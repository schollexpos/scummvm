/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "common/debug.h"
#include "common/stack.h"

#include "sci/scicore/resource.h"
#include "sci/engine/state.h"
#include "sci/scicore/versions.h"
#include "sci/engine/intmap.h"
#include "sci/engine/kdebug.h"
#include "sci/engine/kernel.h"
#include "sci/engine/kernel_types.h"
#include "sci/engine/seg_manager.h"
#include "sci/engine/gc.h"
#include "sci/sfx/player.h"

namespace Sci {

reg_t NULL_REG = NULL_REG_INITIALIZER;

//#define VM_DEBUG_SEND
#undef STRICT_SEND // Disallows variable sends with more than one parameter
#undef STRICT_READ // Disallows reading from out-of-bounds parameters and locals


int script_abort_flag = 0; // Set to 1 to abort execution
int script_error_flag = 0; // Set to 1 if an error occured, reset each round by the VM
int script_checkloads_flag = 0; // Print info when scripts get (un)loaded
int script_step_counter = 0; // Counts the number of steps executed
int script_gc_interval = GC_INTERVAL; // Number of steps in between gcs

extern int _debug_step_running;
extern int _debug_seeking;
extern int _weak_validations;


static bool breakpointFlag = false;
static reg_t _dummy_register;

// validation functionality

#ifndef DISABLE_VALIDATIONS

static reg_t &validate_property(Object *obj, int index) {
	if (!obj) {
		if (sci_debug_flags & 4)
			sciprintf("[VM] Sending to disposed object!\n");
		_dummy_register = NULL_REG;
		return _dummy_register;
	}

	if (index < 0 || index >= obj->variables_nr) {
		if (sci_debug_flags & 4)
			sciprintf("[VM] Invalid property #%d (out of [0..%d]) requested!\n", index,
			          obj->variables_nr);

		_dummy_register = NULL_REG;
		return _dummy_register;
	}

	return obj->variables[index];
}

static StackPtr validate_stack_addr(EngineState *s, StackPtr sp) {
	if (sp >= s->stack_base && sp < s->stack_top)
		return sp;

	script_debug_flag = script_error_flag = 1;
	if (sci_debug_flags & 4)
		sciprintf("[VM] Stack index %d out of valid range [%d..%d]\n", (int)(sp - s->stack_base), 0, (int)(s->stack_top - s->stack_base - 1));
	return 0;
}

static int validate_arithmetic(reg_t reg) {
	if (reg.segment) {
		if (!_weak_validations)
			script_debug_flag = script_error_flag = 1;
		if (sci_debug_flags & 4)
			sciprintf("[VM] Attempt to read arithmetic value from non-zero segment [%04x]\n", reg.segment);
		return 0;
	}

	return reg.offset;
}

static int signed_validate_arithmetic(reg_t reg) {
	if (reg.segment) {
		if (!_weak_validations)
			script_debug_flag = script_error_flag = 1;
		if (sci_debug_flags & 4)
			sciprintf("[VM] Attempt to read arithmetic value from non-zero segment [%04x]\n", reg.segment);
		return 0;
	}

	if (reg.offset&0x8000)
		return (signed)(reg.offset) - 65536;
	else
		return reg.offset;
}

static int validate_variable(reg_t *r, reg_t *stack_base, int type, int max, int index, int line) {
	const char *names[4] = {"global", "local", "temp", "param"};

	if (index < 0 || index >= max) {
		sciprintf("[VM] Attempt to use invalid %s variable %04x ", names[type], index);
		if (max == 0)
			sciprintf("(variable type invalid)");
		else
			sciprintf("(out of range [%d..%d])", 0, max - 1);
		sciprintf(" in %s, line %d\n", __FILE__, line);
		if (!_weak_validations)
			script_debug_flag = script_error_flag = 1;

#ifdef STRICT_READ
		return 1;
#else // !STRICT_READ
		if (type == VAR_PARAM || type == VAR_TEMP) {
			int total_offset = r - stack_base;
			if (total_offset < 0 || total_offset >= VM_STACK_SIZE) {
				sciprintf("[VM] Access would be outside even of the stack (%d); access denied\n", total_offset);
				return 1;
			} else {
				sciprintf("[VM] Access within stack boundaries; access granted.\n");
				return 0;
			}
		};
#endif
	}

	return 0;
}

static reg_t validate_read_var(reg_t *r, reg_t *stack_base, int type, int max, int index, int line, reg_t default_value) {
	if (!validate_variable(r, stack_base, type, max, index, line))
		return r[index];
	else
		return default_value;
}

static void validate_write_var(reg_t *r, reg_t *stack_base, int type, int max, int index, int line, reg_t value) {
	if (!validate_variable(r, stack_base, type, max, index, line))
		r[index] = value;
}

#  define ASSERT_ARITHMETIC(v) validate_arithmetic(v)

#else
// Non-validating alternatives

#  define validate_stack_addr(s, sp) sp
#  define validate_arithmetic(r) ((r).offset)
#  define signed_validate_arithmetic(r) ((int) ((r).offset) & 0x8000 ? (signed) ((r).offset) - 65536 : ((r).offset))
#  define validate_variable(r, sb, t, m, i, l)
#  define validate_read_var(r, sb, t, m, i, l) ((r)[i])
#  define validate_write_var(r, sb, t, m, i, l, v) ((r)[i] = (v))
#  define validate_property(o, p) ((o)->variables[p])
#  define ASSERT_ARITHMETIC(v) (v).offset

#endif

#define READ_VAR(type, index, def) validate_read_var(variables[type], s->stack_base, type, variables_max[type], index, __LINE__, def)
#define WRITE_VAR(type, index, value) validate_write_var(variables[type], s->stack_base, type, variables_max[type], index, __LINE__, value)
#define WRITE_VAR16(type, index, value) WRITE_VAR(type, index, make_reg(0, value));

#define ACC_ARITHMETIC_L(op) make_reg(0, (op validate_arithmetic(s->r_acc)))
#define ACC_AUX_LOAD() aux_acc = signed_validate_arithmetic(s->r_acc)
#define ACC_AUX_STORE() s->r_acc = make_reg(0, aux_acc)

#define OBJ_PROPERTY(o, p) (validate_property(o, p))

int script_error(EngineState *s, const char *file, int line, const char *reason) {
	sciprintf("Script error in file %s, line %d: %s\n", file, line, reason);
	script_debug_flag = script_error_flag = 1;
	return 0;
}
#define CORE_ERROR(area, msg) script_error(s, "[" area "] " __FILE__, __LINE__, msg)

reg_t get_class_address(EngineState *s, int classnr, int lock, reg_t caller) {
	Class *the_class = s->classtable + classnr;

	if (NULL == s) {
		sciprintf("vm.c: get_class_address(): NULL passed for \"s\"\n");
		return NULL_REG;
	}

	if (classnr < 0 || s->classtable_size <= classnr || the_class->script < 0) {
		sciprintf("[VM] Attempt to dereference class %x, which doesn't exist (max %x)\n", classnr, s->classtable_size);
		script_error_flag = script_debug_flag = 1;
		return NULL_REG;
	} else {
		if (!the_class->reg.segment) {
			script_get_segment(s, the_class->script, lock);

			if (!the_class->reg.segment) {
				sciprintf("[VM] Trying to instantiate class %x by instantiating script 0x%x (%03d) failed;"
				          " Entering debugger.\n", classnr, the_class->script, the_class->script);
				script_error_flag = script_debug_flag = 1;
				return NULL_REG;
			}
		} else
			if (caller.segment != the_class->reg.segment)
				s->seg_manager->incrementLockers(the_class->reg.segment, SEG_ID);

		return the_class->reg;
	}
}

// Operating on the stack
// 16 bit:
#define PUSH(v) PUSH32(make_reg(0, v))
#define POP() (validate_arithmetic(POP32()))
// 32 bit:
#define PUSH32(a) (*(validate_stack_addr(s, (xs->sp)++)) = (a))
#define POP32() (*(validate_stack_addr(s, --(xs->sp))))

// Getting instruction parameters
#define GET_OP_BYTE() ((uint8)code_buf[(xs->addr.pc.offset)++])
#define GET_OP_WORD() (READ_LE_UINT16(code_buf + ((xs->addr.pc.offset) += 2) - 2))
#define GET_OP_FLEX() ((opcode & 1)? GET_OP_BYTE() : GET_OP_WORD())
#define GET_OP_SIGNED_BYTE() ((int8)(code_buf[(xs->addr.pc.offset)++]))
#define GET_OP_SIGNED_WORD() (((int16)READ_LE_UINT16(code_buf + ((xs->addr.pc.offset) += 2) - 2)))
#define GET_OP_SIGNED_FLEX() ((opcode & 1)? GET_OP_SIGNED_BYTE() : GET_OP_SIGNED_WORD())

#define SEG_GET_HEAP(s, reg) s->seg_manager->getHeap(reg)
#define OBJ_SPECIES(s, reg) SEG_GET_HEAP(s, make_reg(reg.segment, reg.offset + SCRIPT_SPECIES_OFFSET))
// Returns an object's species

#define OBJ_SUPERCLASS(s, reg) SEG_GET_HEAP(s, make_reg(reg.segment, reg.offset + SCRIPT_SUPERCLASS_OFFSET))
// Returns an object's superclass

ExecStack *execute_method(EngineState *s, uint16 script, uint16 pubfunct, StackPtr sp, reg_t calling_obj, uint16 argc, StackPtr argp) {
	int seg;
	uint16 temp;

	if (!s->seg_manager->scriptIsLoaded(script, SCRIPT_ID))  // Script not present yet?
		script_instantiate(s, script);
	else
		s->seg_manager->unmarkScriptDeleted(script);

	seg = s->seg_manager->segGet(script);

	temp = s->seg_manager->validateExportFunc(pubfunct, seg);
	VERIFY(temp, "Invalid pubfunct in export table");
	if (!temp) {
		sciprintf("Request for invalid exported function 0x%x of script 0x%x\n", pubfunct, script);
		script_error_flag = script_debug_flag = 1;
		return NULL;
	}

	// Check if a breakpoint is set on this method
	if (s->have_bp & BREAK_EXPORT) {
		Breakpoint *bp;
		uint32 bpaddress;

		bpaddress = (script << 16 | pubfunct);

		bp = s->bp_list;
		while (bp) {
			if (bp->type == BREAK_EXPORT && bp->data.address == bpaddress) {
				sciprintf("Break on script %d, export %d\n", script, pubfunct);
				script_debug_flag = 1;
				breakpointFlag = true;
				break;
			}
			bp = bp->next;
		}
	}

	return add_exec_stack_entry(s, make_reg(seg, temp), sp, calling_obj, argc, argp, -1, calling_obj, s->execution_stack_pos, seg);
}


static void _exec_varselectors(EngineState *s) {
	// Executes all varselector read/write ops on the TOS
	// Now check the TOS to execute all varselector entries
	if (s->execution_stack_pos >= 0)
		while (s->execution_stack[s->execution_stack_pos].type == EXEC_STACK_TYPE_VARSELECTOR) {
			// varselector access?
			if (s->execution_stack[s->execution_stack_pos].argc) { // write?
				reg_t temp = s->execution_stack[s->execution_stack_pos].variables_argp[1];
				*(s->execution_stack[s->execution_stack_pos].addr.varp) = temp;

			} else // No, read
				s->r_acc = *(s->execution_stack[s->execution_stack_pos].addr.varp);

			--(s->execution_stack_pos);
		}
}

ExecStack *send_selector(EngineState *s, reg_t send_obj, reg_t work_obj, StackPtr sp, int framesize, StackPtr argp) {
// send_obj and work_obj are equal for anything but 'super'
// Returns a pointer to the TOS exec_stack element
#ifdef VM_DEBUG_SEND
	int i;
#endif
	reg_t *varp;
	reg_t funcp;
	int selector;
	int argc;
	int origin = s->execution_stack_pos; // Origin: Used for debugging
	ExecStack *retval = s->execution_stack + s->execution_stack_pos;
	int print_send_action = 0;
	// We return a pointer to the new active ExecStack

	// The selector calls we catch are stored below:
	Common::Stack<CallsStruct> sendCalls;

	if (NULL == s) {
		sciprintf("vm.c: ExecStack(): NULL passed for \"s\"\n");
		return NULL;
	}

	while (framesize > 0) {
		selector = validate_arithmetic(*argp++);
		argc = validate_arithmetic(*argp);

		if (argc > 0x800) { // More arguments than the stack could possibly accomodate for
			CORE_ERROR("SEND", "More than 0x800 arguments to function call\n");
			return NULL;
		}

		// Check if a breakpoint is set on this method
		if (s->have_bp & BREAK_SELECTOR) {
			Breakpoint *bp;
			char method_name [256];

			sprintf(method_name, "%s::%s", obj_get_name(s, send_obj), s->_selectorNames[selector].c_str());

			bp = s->bp_list;
			while (bp) {
				int cmplen = strlen(bp->data.name);
				if (bp->data.name[cmplen - 1] != ':')
					cmplen = 256;

				if (bp->type == BREAK_SELECTOR && !strncmp(bp->data.name, method_name, cmplen)) {
					sciprintf("Break on %s (in ["PREG"])\n", method_name, PRINT_REG(send_obj));
					script_debug_flag = print_send_action = 1;
					breakpointFlag = true;
					break;
				}
				bp = bp->next;
			}
		}

#ifdef VM_DEBUG_SEND
		sciprintf("Send to "PREG", selector %04x (%s):", PRINT_REG(send_obj), selector, s->_selectorNames[selector].c_str());
#endif // VM_DEBUG_SEND

		switch (lookup_selector(s, send_obj, selector, &varp, &funcp)) {
		case kSelectorNone:
			sciprintf("Send to invalid selector 0x%x of object at "PREG"\n", 0xffff & selector, PRINT_REG(send_obj));
			script_error_flag = script_debug_flag = 1;
			break;

		case kSelectorVariable:

#ifdef VM_DEBUG_SEND
			sciprintf("Varselector: ");
			if (argc)
				sciprintf("Write "PREG"\n", PRINT_REG(argp[1]));
			else
				sciprintf("Read\n");
#endif // VM_DEBUG_SEND

			switch (argc) {
			case 0:   // Read selector
				if (print_send_action) {
					sciprintf("[read selector]\n");
					print_send_action = 0;
				}
				// fallthrough
			case 1:
#ifndef STRICT_SEND
			default:
#endif
				{ // Argument is supplied -> Selector should be set
					if (print_send_action) {
						reg_t oldReg = *varp;
						reg_t newReg = argp[1];

						sciprintf("[write to selector: change "PREG" to "PREG"]\n", PRINT_REG(oldReg), PRINT_REG(newReg));
						print_send_action = 0;
					}
					CallsStruct call;
					call.address.var = varp; // register the call
					call.argp = argp;
					call.argc = argc;
					call.selector = selector;
					call.type = EXEC_STACK_TYPE_VARSELECTOR; // Register as a varselector
					sendCalls.push(call);
				}
				break;
#ifdef STRICT_SEND
			default:
				sciprintf("Send error: Variable selector %04x in "PREG" called with %04x params\n", selector, PRINT_REG(send_obj), argc);
				script_debug_flag = 1; // Enter debug mode
				_debug_seeking = _debug_step_running = 0;
#endif
			}
			break;

		case kSelectorMethod:

#ifdef VM_DEBUG_SEND
			sciprintf("Funcselector(");
			for (i = 0; i < argc; i++) {
				sciprintf(PREG, PRINT_REG(argp[i+1]));
				if (i + 1 < argc)
					sciprintf(", ");
			}
			sciprintf(") at "PREG"\n", PRINT_REG(funcp));
#endif // VM_DEBUG_SEND
			if (print_send_action) {
				sciprintf("[invoke selector]\n");
				print_send_action = 0;
			}

			CallsStruct call;
			call.address.func = funcp; // register call
			call.argp = argp;
			call.argc = argc;
			call.selector = selector;
			call.type = EXEC_STACK_TYPE_CALL;
			call.sp = sp;
			sp = CALL_SP_CARRY; // Destroy sp, as it will be carried over
			sendCalls.push(call);

			break;
		} // switch(lookup_selector())

		framesize -= (2 + argc);
		argp += argc + 1;
	}

	// Iterate over all registered calls in the reverse order. This way, the first call is
	// placed on the TOS; as soon as it returns, it will cause the second call to be executed.
	while (!sendCalls.empty()) {
		CallsStruct call = sendCalls.pop();
		if (call.type == EXEC_STACK_TYPE_VARSELECTOR) // Write/read variable?
			retval = add_exec_stack_varselector(s, work_obj, call.argc, call.argp,
			                                    call.selector, call.address.var, origin);
		else
			retval = add_exec_stack_entry(s, call.address.func, call.sp, work_obj,
			                         call.argc, call.argp,
			                         call.selector, send_obj, origin, SCI_XS_CALLEE_LOCALS);
	}

	_exec_varselectors(s);

	retval = s->execution_stack + s->execution_stack_pos;
	return retval;
}

ExecStack *add_exec_stack_varselector(EngineState *s, reg_t objp, int argc, StackPtr argp, Selector selector, reg_t *address, int origin) {
	ExecStack *xstack = add_exec_stack_entry(s, NULL_REG, address, objp, argc, argp, selector, objp, origin, SCI_XS_CALLEE_LOCALS);
	// Store selector address in sp

	xstack->addr.varp = address;
	xstack->type = EXEC_STACK_TYPE_VARSELECTOR;

	return xstack;
}

ExecStack *add_exec_stack_entry(EngineState *s, reg_t pc, StackPtr sp, reg_t objp, int argc,
								   StackPtr argp, Selector selector, reg_t sendp, int origin, SegmentId locals_segment) {
	// Returns new TOS element for the execution stack
	// locals_segment may be -1 if derived from the called object
	ExecStack *xstack = NULL;

	if (!s->execution_stack)
		s->execution_stack = (ExecStack *)sci_malloc(sizeof(ExecStack) * (s->execution_stack_size = 16));

	if (++(s->execution_stack_pos) == s->execution_stack_size) // Out of stack space?
		s->execution_stack = (ExecStack*)sci_realloc(s->execution_stack, sizeof(ExecStack) * (s->execution_stack_size += 8));

	//sciprintf("Exec stack: [%d/%d], origin %d, at %p\n", s->execution_stack_pos, s->execution_stack_size, origin, s->execution_stack);

	xstack = s->execution_stack + s->execution_stack_pos;

	xstack->objp = objp;
	if (locals_segment != SCI_XS_CALLEE_LOCALS)
		xstack->local_segment = locals_segment;
	else
		xstack->local_segment = pc.segment;

	xstack->sendp = sendp;
	xstack->addr.pc = pc;
	xstack->fp = xstack->sp = sp;
	xstack->argc = argc;

	xstack->variables_argp = argp; // Parameters

	*argp = make_reg(0, argc);  // SCI code relies on the zeroeth argument to equal argc

	// Additional debug information
	xstack->selector = selector;
	xstack->origin = origin;

	xstack->type = EXEC_STACK_TYPE_CALL; // Normal call

	return xstack;
}

#ifdef DISABLE_VALIDATONS
#  define kernel_matches_signature(a, b, c, d) 1
#endif

void vm_handle_fatal_error(EngineState *s, int line, const char *file) {
	fprintf(stderr, "Fatal VM error in %s, L%d; aborting...\n", file, line);
	error("Could not recover, exitting...\n");
}

static Script *script_locate_by_segment(EngineState *s, SegmentId seg) {
	MemObject *memobj = GET_SEGMENT(*s->seg_manager, seg, MEM_OBJ_SCRIPT);
	if (memobj)
		return &(memobj->data.script);

	return NULL;
}

static reg_t pointer_add(EngineState *s, reg_t base, int offset) {
	MemObject *mobj = GET_SEGMENT_ANY(*s->seg_manager, base.segment);

	if (!mobj) {
		script_debug_flag = script_error_flag = 1;
		sciprintf("[VM] Error: Attempt to add %d to invalid pointer "PREG"!", offset, PRINT_REG(base));
		return NULL_REG;
	}

	switch (mobj->type) {

	case MEM_OBJ_LOCALS:
		base.offset += 2 * offset;
		return base;

	case MEM_OBJ_SCRIPT:
	case MEM_OBJ_STACK:
	case MEM_OBJ_DYNMEM:
		base.offset += offset;
		return base;
		break;

	default:
		sciprintf("[VM] Error: Attempt to add %d to pointer "PREG": Pointer arithmetics of this type unsupported!", offset, PRINT_REG(base));
		return NULL_REG;

	}
}

static void gc_countdown(EngineState *s) {
	if (s->gc_countdown-- <= 0) {
		s->gc_countdown = script_gc_interval;
		run_gc(s);
	}
}

static const byte _fake_return_buffer[2] = {op_ret << 1, op_ret << 1};

void run_vm(EngineState *s, int restoring) {
	reg_t *variables[4]; // global, local, temp, param, as immediate pointers
	reg_t *variables_base[4]; // Used for referencing VM ops
	SegmentId variables_seg[4]; // Same as above, contains segment IDs
#ifndef DISABLE_VALIDATIONS
	int variables_max[4]; // Max. values for all variables
	unsigned int code_buf_size = 0 ; // (Avoid spurious warning)
#endif
	int temp;
	int16 aux_acc; // Auxiliary 16 bit accumulator
	reg_t r_temp; // Temporary register
	StackPtr s_temp; // Temporary stack pointer
	int16 opparams[4]; // opcode parameters

	int restadjust = s->r_amp_rest;
	// &rest adjusts the parameter count by this value
	// Current execution data:
	ExecStack *xs = s->execution_stack + s->execution_stack_pos;
	ExecStack *xs_new = NULL;
	Object *obj = obj_get(s, xs->objp);
	Script *local_script = script_locate_by_segment(s, xs->local_segment);
	int old_execution_stack_base = s->execution_stack_base;
	// Used to detect the stack bottom, for "physical" returns
	const byte *code_buf = NULL; // (Avoid spurious warning)

	if (!local_script) {
		script_error(s, __FILE__, __LINE__, "Program Counter gone astray");
		return;
	}

	if (NULL == s) {
		sciprintf("vm.c: run_vm(): NULL passed for \"s\"\n");
		return;
	}

	if (!restoring)
		s->execution_stack_base = s->execution_stack_pos;

#ifndef DISABLE_VALIDATIONS
	// Initialize maximum variable count
	if (s->script_000->locals_block)
		variables_max[VAR_GLOBAL] = s->script_000->locals_block->nr;
	else
		variables_max[VAR_GLOBAL] = 0;
#endif

	variables_seg[VAR_GLOBAL] = s->script_000->locals_segment;
	variables_seg[VAR_TEMP] = variables_seg[VAR_PARAM] = s->stack_segment;
	variables_base[VAR_TEMP] = variables_base[VAR_PARAM] = s->stack_base;

	// SCI code reads the zeroeth argument to determine argc
	if (s->script_000->locals_block)
		variables_base[VAR_GLOBAL] = variables[VAR_GLOBAL] = s->script_000->locals_block->locals;
	else
		variables_base[VAR_GLOBAL] = variables[VAR_GLOBAL] = NULL;



	s->execution_stack_pos_changed = 1; // Force initialization

	while (1) {
		byte opcode;
		int old_pc_offset;
		StackPtr old_sp = xs->sp;
		byte opnumber;
		int var_type; // See description below
		int var_number;

		old_pc_offset = xs->addr.pc.offset;

		if (s->execution_stack_pos_changed) {
			Script *scr;
			xs = s->execution_stack + s->execution_stack_pos;
			s->execution_stack_pos_changed = 0;

			scr = script_locate_by_segment(s, xs->addr.pc.segment);
			if (!scr) {
				// No script? Implicit return via fake instruction buffer
				warning("Running on non-existant script in segment %x!", xs->addr.pc.segment);
				code_buf = _fake_return_buffer;
#ifndef DISABLE_VALIDATIONS
				code_buf_size = 2;
#endif
				xs->addr.pc.offset = 1;

				scr = NULL;
				obj = NULL;
			} else {
				obj = obj_get(s, xs->objp);
				code_buf = scr->buf;
#ifndef DISABLE_VALIDATIONS
				code_buf_size = scr->buf_size;
#endif
				/*if (!obj) {
					SCIkdebug(SCIkWARNING, "Running with non-existant self= "PREG"\n", PRINT_REG(xs->objp));
				}*/

				local_script = script_locate_by_segment(s, xs->local_segment);
				if (!local_script) {
					warning("Could not find local script from segment %x", xs->local_segment);
					local_script = NULL;
					variables_base[VAR_LOCAL] = variables[VAR_LOCAL] = NULL;
#ifndef DISABLE_VALIDATIONS
					variables_max[VAR_LOCAL] = 0;
#endif
				} else {

					variables_seg[VAR_LOCAL] = local_script->locals_segment;
					if (local_script->locals_block)
						variables_base[VAR_LOCAL] = variables[VAR_LOCAL] = local_script->locals_block->locals;
					else
						variables_base[VAR_LOCAL] = variables[VAR_LOCAL] = NULL;
#ifndef DISABLE_VALIDATIONS
					if (local_script->locals_block)
						variables_max[VAR_LOCAL] = local_script->locals_block->nr;
					else
						variables_max[VAR_LOCAL] = 0;
					variables_max[VAR_TEMP] = xs->sp - xs->fp;
					variables_max[VAR_PARAM] = xs->argc + 1;
#endif
				}
				variables[VAR_TEMP] = xs->fp;
				variables[VAR_PARAM] = xs->variables_argp;
			}

		}

		script_error_flag = 0; // Set error condition to false

		if (script_abort_flag)
			return; // Emergency

		// Debug if this has been requested:
		if (script_debug_flag || sci_debug_flags) {
			script_debug(s, &(xs->addr.pc), &(xs->sp), &(xs->fp), &(xs->objp), &restadjust, variables_seg, variables, variables_base,
#ifdef DISABLE_VALIDATIONS
			             NULL,
#else
			             variables_max,
#endif
			             breakpointFlag);
			breakpointFlag = false;
		}

#ifndef DISABLE_VALIDATIONS
		if (xs->sp < xs->fp)
			script_error(s, "[VM] "__FILE__, __LINE__, "Stack underflow");

		variables_max[VAR_TEMP] = xs->sp - xs->fp;

		if (xs->addr.pc.offset >= code_buf_size)
			script_error(s, "[VM] "__FILE__, __LINE__, "Program Counter gone astray");
#endif

		opcode = GET_OP_BYTE(); // Get opcode

		opnumber = opcode >> 1;

		for (temp = 0; formats[opnumber][temp]; temp++)
			switch (formats[opnumber][temp]) {

			case Script_Byte:
				opparams[temp] = GET_OP_BYTE();
				break;
			case Script_SByte:
				opparams[temp] = GET_OP_SIGNED_BYTE();
				break;

			case Script_Word:
				opparams[temp] = GET_OP_WORD();
				break;
			case Script_SWord:
				opparams[temp] = GET_OP_SIGNED_WORD();
				break;

			case Script_Variable:
			case Script_Property:

			case Script_Local:
			case Script_Temp:
			case Script_Global:
			case Script_Param:
				opparams[temp] = GET_OP_FLEX();
				break;

			case Script_SVariable:
			case Script_SRelative:
				opparams[temp] = GET_OP_SIGNED_FLEX();
				break;

			case Script_Offset:
				opparams[temp] = GET_OP_FLEX();
				break;

			case Script_None:
			case Script_End:
				break;

			case Script_Invalid:
			default:
				sciprintf("opcode %02x: Invalid!", opcode);
				script_debug_flag = script_error_flag = 1;
			}

		// TODO: Replace the following by an opcode table, and several methods for
		// each opcode.
		switch (opnumber) {

		case 0x00: // bnot
			s->r_acc = ACC_ARITHMETIC_L(0xffff ^ /*acc*/);
			break;

		case 0x01: // add
			r_temp = POP32();
			if (r_temp.segment || s->r_acc.segment) {
				reg_t r_ptr;
				int offset;
				// Pointer arithmetics!
				if (s->r_acc.segment) {
					if (r_temp.segment) {
						sciprintf("Error: Attempt to add two pointers, stack="PREG" and acc="PREG"!\n",
						          PRINT_REG(r_temp), PRINT_REG(s->r_acc));
						script_debug_flag = script_error_flag = 1;
						offset = 0;
					} else {
						r_ptr = s->r_acc;
						offset = r_temp.offset;
					}
				} else {
					r_ptr = r_temp;
					offset = s->r_acc.offset;
				}

				s->r_acc = pointer_add(s, r_ptr, offset);

			} else
				s->r_acc = make_reg(0, r_temp.offset + s->r_acc.offset);
			break;

		case 0x02: // sub
			r_temp = POP32();
			if (r_temp.segment || s->r_acc.segment) {
				reg_t r_ptr;
				int offset;
				// Pointer arithmetics!
				if (s->r_acc.segment) {
					if (r_temp.segment) {
						sciprintf("Error: Attempt to subtract two pointers, stack="PREG" and acc="PREG"!\n",
						          PRINT_REG(r_temp), PRINT_REG(s->r_acc));
						script_debug_flag = script_error_flag = 1;
						offset = 0;
					} else {
						r_ptr = s->r_acc;
						offset = r_temp.offset;
					}
				} else {
					r_ptr = r_temp;
					offset = s->r_acc.offset;
				}

				s->r_acc = pointer_add(s, r_ptr, -offset);

			} else
				s->r_acc = make_reg(0, r_temp.offset - s->r_acc.offset);
			break;

		case 0x03: // mul
			s->r_acc = ACC_ARITHMETIC_L(((int16)POP()) * (int16)/*acc*/);
			break;

		case 0x04: // div
			ACC_AUX_LOAD();
			aux_acc = aux_acc != 0 ? ((int16)POP()) / aux_acc : 0;
			ACC_AUX_STORE();
			break;

		case 0x05: // mod
			ACC_AUX_LOAD();
			aux_acc = aux_acc != 0 ? ((int16)POP()) % aux_acc : 0;
			ACC_AUX_STORE();
			break;

		case 0x06: // shr
			s->r_acc = ACC_ARITHMETIC_L(((uint16) POP()) >> /*acc*/);
			break;

		case 0x07: // shl
			s->r_acc = ACC_ARITHMETIC_L(((uint16)POP()) << /*acc*/);
			break;

		case 0x08: // xor
			s->r_acc = ACC_ARITHMETIC_L(POP() ^ /*acc*/);
			break;

		case 0x09: // and
			s->r_acc = ACC_ARITHMETIC_L(POP() & /*acc*/);
			break;

		case 0x0a: // or
			s->r_acc = ACC_ARITHMETIC_L(POP() | /*acc*/);
			break;

		case 0x0b: // neg
			s->r_acc = ACC_ARITHMETIC_L(-/*acc*/);
			break;

		case 0x0c: // not
			s->r_acc = make_reg(0, !(s->r_acc.offset || s->r_acc.segment));
			// Must allow pointers to be negated, as this is used for checking whether objects exist
			break;

		case 0x0d: // eq?
			s->r_prev = s->r_acc;
			r_temp = POP32();
			s->r_acc = make_reg(0, REG_EQ(r_temp, s->r_acc));
			// Explicitly allow pointers to be compared
			break;

		case 0x0e: // ne?
			s->r_prev = s->r_acc;
			r_temp = POP32();
			s->r_acc = make_reg(0, !REG_EQ(r_temp, s->r_acc));
			// Explicitly allow pointers to be compared
			break;

		case 0x0f: // gt?
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L((int16)POP() > (int16)/*acc*/);
			break;

		case 0x10: // ge?
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L((int16)POP() >= (int16)/*acc*/);
			break;

		case 0x11: // lt?
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L((int16)POP() < (int16)/*acc*/);
			break;

		case 0x12: // le?
			s->r_prev = s->r_acc;
			s->r_acc = ACC_ARITHMETIC_L((int16)POP() <= (int16)/*acc*/);
			break;

		case 0x13: // ugt?
			s->r_prev = s->r_acc;
			r_temp = POP32();
			s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset > s->r_acc.offset);
			break;

		case 0x14: // uge?
			s->r_prev = s->r_acc;
			r_temp = POP32();
			s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset >= s->r_acc.offset);
			break;

		case 0x15: // ult?
			s->r_prev = s->r_acc;
			r_temp = POP32();
			s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset < s->r_acc.offset);
			break;

		case 0x16: // ule?
			s->r_prev = s->r_acc;
			r_temp = POP32();
			s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset <= s->r_acc.offset);
			break;

		case 0x17: // bt
			if (s->r_acc.offset || s->r_acc.segment)
				xs->addr.pc.offset += opparams[0];
			break;

		case 0x18: // bnt
			if (!(s->r_acc.offset || s->r_acc.segment))
				xs->addr.pc.offset += opparams[0];
			break;

		case 0x19: // jmp
			xs->addr.pc.offset += opparams[0];
			break;

		case 0x1a: // ldi
			s->r_acc = make_reg(0, opparams[0]);
			break;

		case 0x1b: // push
			PUSH32(s->r_acc);
			break;

		case 0x1c: // pushi
			PUSH(opparams[0]);
			break;

		case 0x1d: // toss
			xs->sp--;
			break;

		case 0x1e: // dup
			r_temp = xs->sp[-1];
			PUSH32(r_temp);
			break;

		case 0x1f: { // link
			int i;
			for (i = 0; i < opparams[0]; i++)
				xs->sp[i] = NULL_REG;
			xs->sp += opparams[0];
			break;
		}

		case 0x20: { // call
			int argc = (opparams[1] >> 1) // Given as offset, but we need count
			           + 1 + restadjust;
			StackPtr call_base = xs->sp - argc;

			xs->sp[1].offset += restadjust;
			xs_new = add_exec_stack_entry(s, make_reg(xs->addr.pc.segment, xs->addr.pc.offset + opparams[0]),
			                              xs->sp, xs->objp, (validate_arithmetic(*call_base)) + restadjust,
			                              call_base, NULL_SELECTOR, xs->objp, s->execution_stack_pos, xs->local_segment);
			restadjust = 0; // Used up the &rest adjustment
			xs->sp = call_base;

			s->execution_stack_pos_changed = 1;
			break;
		}

		case 0x21: // callk
			gc_countdown(s);

			xs->sp -= (opparams[1] >> 1) + 1;
			if (s->version >= SCI_VERSION_FTU_NEW_SCRIPT_HEADER) {
				xs->sp -= restadjust;
				s->r_amp_rest = 0; // We just used up the restadjust, remember?
			}

			if (opparams[0] >= s->kfunct_nr) {
				sciprintf("Invalid kernel function 0x%x requested\n", opparams[0]);
				script_debug_flag = script_error_flag = 1;
			} else {
				int argc = ASSERT_ARITHMETIC(xs->sp[0]);

				if (s->version >= SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
					argc += restadjust;

				if (s->kfunct_table[opparams[0]].signature
				        && !kernel_matches_signature(s, s->kfunct_table[opparams[0]].signature, argc, xs->sp + 1)) {
					sciprintf("[VM] Invalid arguments to kernel call %x\n", opparams[0]);
					script_debug_flag = script_error_flag = 1;
				} else {
					s->r_acc = s->kfunct_table[opparams[0]].fun(s, opparams[0], argc, xs->sp + 1);
				}
				// Call kernel function

				// Calculate xs again: The kernel function might
				// have spawned a new VM

				xs_new = s->execution_stack + s->execution_stack_pos;
				s->execution_stack_pos_changed = 1;

				if (s->version >= SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
					restadjust = s->r_amp_rest;

			}
			break;

		case 0x22: // callb
			temp = ((opparams[1] >> 1) + restadjust + 1);
			s_temp = xs->sp;
			xs->sp -= temp;

			xs->sp[0].offset += restadjust;
			xs_new = execute_method(s, 0, opparams[0], s_temp, xs->objp, xs->sp[0].offset, xs->sp);
			restadjust = 0; // Used up the &rest adjustment
			if (xs_new)    // in case of error, keep old stack
				s->execution_stack_pos_changed = 1;
			break;

		case 0x23: // calle
			temp = ((opparams[2] >> 1) + restadjust + 1);
			s_temp = xs->sp;
			xs->sp -= temp;

			xs->sp[0].offset += restadjust;
			xs_new = execute_method(s, opparams[0], opparams[1], s_temp, xs->objp, xs->sp[0].offset, xs->sp);
			restadjust = 0; // Used up the &rest adjustment

			if (xs_new)  // in case of error, keep old stack
				s->execution_stack_pos_changed = 1;
			break;

		case 0x24: // ret
			do {
				StackPtr old_sp2 = xs->sp;
				StackPtr old_fp = xs->fp;
				ExecStack *old_xs = s->execution_stack + s->execution_stack_pos;

				if (s->execution_stack_pos == s->execution_stack_base) { // Have we reached the base?
					s->execution_stack_base = old_execution_stack_base; // Restore stack base

					--(s->execution_stack_pos);

					s->execution_stack_pos_changed = 1;
					s->r_amp_rest = restadjust; // Update &rest
					return; // "Hard" return
				}

				if (old_xs->type == EXEC_STACK_TYPE_VARSELECTOR) {
					// varselector access?
					if (old_xs->argc) // write?
						*(old_xs->addr.varp) = old_xs->variables_argp[1];
					else // No, read
						s->r_acc = *(old_xs->addr.varp);
				}

				// Not reached the base, so let's do a soft return
				--(s->execution_stack_pos);
				xs = old_xs - 1;
				s->execution_stack_pos_changed = 1;
				xs = s->execution_stack + s->execution_stack_pos;

				if (xs->sp == CALL_SP_CARRY // Used in sends to 'carry' the stack pointer
				        || xs->type != EXEC_STACK_TYPE_CALL) {
					xs->sp = old_sp2;
					xs->fp = old_fp;
				}

			} while (xs->type == EXEC_STACK_TYPE_VARSELECTOR);
			// Iterate over all varselector accesses
			s->execution_stack_pos_changed = 1;
			xs_new = xs;

			break;

		case 0x25: // send
			s_temp = xs->sp;
			xs->sp -= ((opparams[0] >> 1) + restadjust); // Adjust stack

			xs->sp[1].offset += restadjust;
			xs_new = send_selector(s, s->r_acc, s->r_acc, s_temp, (int)(opparams[0] >> 1) + (uint16)restadjust, xs->sp);

			if (xs_new && xs_new != xs)
				s->execution_stack_pos_changed = 1;

			restadjust = 0;

			break;

		case 0x28: // class
			s->r_acc = get_class_address(s, (unsigned)opparams[0], SCRIPT_GET_LOCK, xs->addr.pc);
			break;

		case 0x2a: // self
			s_temp = xs->sp;
			xs->sp -= ((opparams[0] >> 1) + restadjust); // Adjust stack

			xs->sp[1].offset += restadjust;
			xs_new = send_selector(s, xs->objp, xs->objp, s_temp, (int)(opparams[0] >> 1) + (uint16)restadjust, xs->sp);

			if (xs_new && xs_new != xs)
				s->execution_stack_pos_changed = 1;

			restadjust = 0;
			break;

		case 0x2b: // super
			r_temp = get_class_address(s, opparams[0], SCRIPT_GET_LOAD, xs->addr.pc);

			if (!r_temp.segment)
				CORE_ERROR("VM", "Invalid superclass in object");
			else {
				s_temp = xs->sp;
				xs->sp -= ((opparams[1] >> 1) + restadjust); // Adjust stack

				xs->sp[1].offset += restadjust;
				xs_new = send_selector(s, r_temp, xs->objp, s_temp, (int)(opparams[1] >> 1) + (uint16)restadjust, xs->sp);

				if (xs_new && xs_new != xs)
					s->execution_stack_pos_changed = 1;

				restadjust = 0;
			}

			break;

		case 0x2c: // &rest
			temp = (uint16) opparams[0]; // First argument
			restadjust = xs->argc - temp + 1; // +1 because temp counts the paramcount while argc doesn't
			if (restadjust < 0)
				restadjust = 0;

			for (; temp <= xs->argc; temp++)
				PUSH32(xs->variables_argp[temp]);

			break;

		case 0x2d: // lea
			temp = (uint16) opparams[0] >> 1;
			var_number = temp & 0x03; // Get variable type

			// Get variable block offset
			r_temp.segment = variables_seg[var_number];
			r_temp.offset = variables[var_number] - variables_base[var_number];

			if (temp & 0x08)  // Add accumulator offset if requested
				r_temp.offset += signed_validate_arithmetic(s->r_acc);

			r_temp.offset += opparams[1];  // Add index
			r_temp.offset *= sizeof(reg_t);
			// That's the immediate address now
			s->r_acc = r_temp;
			break;


		case 0x2e: // selfID
			s->r_acc = xs->objp;
			break;

		case 0x30: // pprev
			PUSH32(s->r_prev);
			break;

		case 0x31: // pToa
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1));
			break;

		case 0x32: // aTop
			OBJ_PROPERTY(obj, (opparams[0] >> 1)) = s->r_acc;
			break;

		case 0x33: // pTos
			PUSH32(OBJ_PROPERTY(obj, opparams[0] >> 1));
			break;

		case 0x34: // sTop
			OBJ_PROPERTY(obj, (opparams[0] >> 1)) = POP32();
			break;

		case 0x35: // ipToa
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1));
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1)) = ACC_ARITHMETIC_L(1 + /*acc*/);
			break;

		case 0x36: // dpToa
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1));
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1)) = ACC_ARITHMETIC_L(-1 + /*acc*/);
			break;

		case 0x37: // ipTos
			ASSERT_ARITHMETIC(OBJ_PROPERTY(obj, (opparams[0] >> 1)));
			temp = ++OBJ_PROPERTY(obj, (opparams[0] >> 1)).offset;
			PUSH(temp);
			break;

		case 0x38: // dpTos
			ASSERT_ARITHMETIC(OBJ_PROPERTY(obj, (opparams[0] >> 1)));
			temp = --OBJ_PROPERTY(obj, (opparams[0] >> 1)).offset;
			PUSH(temp);
			break;


		case 0x39: // lofsa
			s->r_acc.segment = xs->addr.pc.segment;

			if (s->version >= SCI_VERSION(1, 001, 000))
				s->r_acc.offset = opparams[0] + local_script->script_size;
			else
				if (s->version >= SCI_VERSION_FTU_LOFS_ABSOLUTE)
					s->r_acc.offset = opparams[0];
				else
					s->r_acc.offset = xs->addr.pc.offset + opparams[0];
#ifndef DISABLE_VALIDATIONS
			if (s->r_acc.offset >= code_buf_size) {
				sciprintf("VM: lofsa operation overflowed: "PREG" beyond end"
				          " of script (at %04x)\n", PRINT_REG(s->r_acc), code_buf_size);
				script_error_flag = script_debug_flag = 1;
			}
#endif
			break;

		case 0x3a: // lofss
			r_temp.segment = xs->addr.pc.segment;

			if (s->version >= SCI_VERSION_FTU_LOFS_ABSOLUTE)
				r_temp.offset = opparams[0];
			else
				r_temp.offset = xs->addr.pc.offset + opparams[0];
#ifndef DISABLE_VALIDATIONS
			if (r_temp.offset >= code_buf_size) {
				sciprintf("VM: lofss operation overflowed: "PREG" beyond end"
				          " of script (at %04x)\n", PRINT_REG(r_temp), code_buf_size);
				script_error_flag = script_debug_flag = 1;
			}
#endif
			PUSH32(r_temp);
			break;

		case 0x3b: // push0
			PUSH(0);
			break;

		case 0x3c: // push1
			PUSH(1);
			break;

		case 0x3d: // push2
			PUSH(2);
			break;

		case 0x3e: // pushSelf
			PUSH32(xs->objp);
			break;

		case 0x40: // lag
		case 0x41: // lal
		case 0x42: // lat
		case 0x43: // lap
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			s->r_acc = READ_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x44: // lsg
		case 0x45: // lsl
		case 0x46: // lst
		case 0x47: // lsp
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			PUSH32(READ_VAR(var_type, var_number, s->r_acc));
			break;

		case 0x48: // lagi
		case 0x49: // lali
		case 0x4a: // lati
		case 0x4b: // lapi
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			s->r_acc = READ_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x4c: // lsgi
		case 0x4d: // lsli
		case 0x4e: // lsti
		case 0x4f: // lspi
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			PUSH32(READ_VAR(var_type, var_number, s->r_acc));
			break;

		case 0x50: // sag
		case 0x51: // sal
		case 0x52: // sat
		case 0x53: // sap
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x54: // ssg
		case 0x55: // ssl
		case 0x56: // sst
		case 0x57: // ssp
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			WRITE_VAR(var_type, var_number, POP32());
			break;

		case 0x58: // sagi
		case 0x59: // sali
		case 0x5a: // sati
		case 0x5b: // sapi
			// Special semantics because it wouldn't really make a whole lot
			// of sense otherwise, with acc being used for two things
			// simultaneously...
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			WRITE_VAR(var_type, var_number, s->r_acc = POP32());
			break;

		case 0x5c: // ssgi
		case 0x5d: // ssli
		case 0x5e: // ssti
		case 0x5f: // sspi
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			WRITE_VAR(var_type, var_number, POP32());
			break;

		case 0x60: // +ag
		case 0x61: // +al
		case 0x62: // +at
		case 0x63: // +ap
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			s->r_acc = make_reg(0, 1 + validate_arithmetic(READ_VAR(var_type, var_number, s->r_acc)));
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x64: // +sg
		case 0x65: // +sl
		case 0x66: // +st
		case 0x67: // +sp
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			r_temp = make_reg(0, 1 + validate_arithmetic(READ_VAR(var_type, var_number, s->r_acc)));
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case 0x68: // +agi
		case 0x69: // +ali
		case 0x6a: // +ati
		case 0x6b: // +api
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			s->r_acc = make_reg(0, 1 + validate_arithmetic(READ_VAR(var_type, var_number, s->r_acc)));
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x6c: // +sgi
		case 0x6d: // +sli
		case 0x6e: // +sti
		case 0x6f: // +spi
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			r_temp = make_reg(0, 1 + validate_arithmetic(READ_VAR(var_type, var_number, s->r_acc)));
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case 0x70: // -ag
		case 0x71: // -al
		case 0x72: // -at
		case 0x73: // -ap
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			s->r_acc = make_reg(0, -1 + validate_arithmetic(READ_VAR(var_type, var_number, s->r_acc)));
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x74: // -sg
		case 0x75: // -sl
		case 0x76: // -st
		case 0x77: // -sp
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			r_temp = make_reg(0, -1 + validate_arithmetic(READ_VAR(var_type, var_number, s->r_acc)));
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case 0x78: // -agi
		case 0x79: // -ali
		case 0x7a: // -ati
		case 0x7b: // -api
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			s->r_acc = make_reg(0, -1 + validate_arithmetic(READ_VAR(var_type, var_number, s->r_acc)));
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case 0x7c: // -sgi
		case 0x7d: // -sli
		case 0x7e: // -sti
		case 0x7f: // -spi
			var_type = (opcode >> 1) & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			r_temp = make_reg(0, -1 + validate_arithmetic(READ_VAR(var_type, var_number, s->r_acc)));
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		default:
			script_error(s, __FILE__, __LINE__, "Illegal opcode");

		} // switch(opcode >> 1)

		if (s->execution_stack_pos_changed) // Force initialization
			xs = xs_new;

#ifndef DISABLE_VALIDATIONS
		if (xs != s->execution_stack + s->execution_stack_pos) {
			sciprintf("Error: xs is stale (%d vs %d); last command was %02x\n", (int)(xs - s->execution_stack), s->execution_stack_pos, opnumber);
		}
#endif
		if (script_error_flag) {
			_debug_step_running = 0; // Stop multiple execution
			_debug_seeking = 0; // Stop special seeks
			xs->addr.pc.offset = old_pc_offset;
			xs->sp = old_sp;
		} else
			++script_step_counter;
	}
}

static int _obj_locate_varselector(EngineState *s, Object *obj, Selector slc) {
	// Determines if obj explicitly defines slc as a varselector
	// Returns -1 if not found

	if (s->version < SCI_VERSION(1, 001, 000)) {
		int varnum = obj->variable_names_nr;
		int selector_name_offset = varnum * 2 + SCRIPT_SELECTOR_OFFSET;
		int i;
		byte *buf = obj->base_obj + selector_name_offset;

		obj->base_vars = (uint16 *) buf;

		for (i = 0; i < varnum; i++)
			if (READ_LE_UINT16(buf + (i << 1)) == slc) // Found it?
				return i; // report success

		return -1; // Failed
	} else {
		byte *buf = (byte *) obj->base_vars;
		int i;
		int varnum = obj->variables[1].offset;

		if (!(obj->variables[SCRIPT_INFO_SELECTOR].offset & SCRIPT_INFO_CLASS))
			buf = ((byte *) obj_get(s, obj->variables[SCRIPT_SUPERCLASS_SELECTOR])->base_vars);

		for (i = 0; i < varnum; i++)
			if (READ_LE_UINT16(buf + (i << 1)) == slc) // Found it?
				return i; // report success

		return -1; // Failed
	}
}

static int _class_locate_funcselector(EngineState *s, Object *obj, Selector slc) {
	// Determines if obj is a class and explicitly defines slc as a funcselector
	// Does NOT say anything about obj's superclasses, i.e. failure may be
	// returned even if one of the superclasses defines the funcselector.
	int funcnum = obj->methods_nr;
	int i;

	for (i = 0; i < funcnum; i++)
		if (VM_OBJECT_GET_FUNCSELECTOR(obj, i) == slc) // Found it?
			return i; // report success

	return -1; // Failed
}

static SelectorType _lookup_selector_function(EngineState *s, int seg_id, Object *obj, Selector selector_id, reg_t *fptr) {
	int index;

	// "recursive" lookup

	while (obj) {
		index = _class_locate_funcselector(s, obj, selector_id);

		if (index >= 0) {
			if (fptr) {
				if (s->version < SCI_VERSION(1, 001, 000))
					*fptr = make_reg(obj->pos.segment, READ_LE_UINT16((byte *)(obj->base_method + index + obj->methods_nr + 1)));
				else
					*fptr = make_reg(obj->pos.segment, READ_LE_UINT16((byte *)(obj->base_method + index * 2 + 2)));
			}

			return kSelectorMethod;
		} else {
			seg_id = obj->variables[SCRIPT_SUPERCLASS_SELECTOR].segment;
			obj = obj_get(s, obj->variables[SCRIPT_SUPERCLASS_SELECTOR]);
		}
	}

	return kSelectorNone;
}

SelectorType lookup_selector(EngineState *s, reg_t obj_location, Selector selector_id, reg_t **vptr, reg_t *fptr) {
	Object *obj = obj_get(s, obj_location);
	Object *species;
	int index;

	// Early SCI versions used the LSB in the selector ID as a read/write
	// toggle, meaning that we must remove it for selector lookup.
	if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER)
		selector_id &= ~1;

	if (!obj) {
		CORE_ERROR("SLC-LU", "Attempt to send to non-object or invalid script");
		sciprintf("Address was "PREG"\n", PRINT_REG(obj_location));
		return kSelectorNone;
	}

	if (IS_CLASS(obj))
		species = obj;
	else
		species = obj_get(s, obj->variables[SCRIPT_SPECIES_SELECTOR]);


	if (!obj) {
		CORE_ERROR("SLC-LU", "Error while looking up Species class");
		sciprintf("Original address was "PREG"\n", PRINT_REG(obj_location));
		sciprintf("Species address was "PREG"\n", PRINT_REG(obj->variables[SCRIPT_SPECIES_SELECTOR]));
		return kSelectorNone;
	}

	index = _obj_locate_varselector(s, obj, selector_id);

	if (index >= 0) {
		// Found it as a variable
		if (vptr)
			*vptr = obj->variables + index;
		return kSelectorVariable;
	}

	return _lookup_selector_function(s, obj_location.segment, obj, selector_id, fptr);
}

// Detects SCI versions by their different script header
void script_detect_versions(EngineState *s) {
	int c;
	Resource *script = {0};

	if (s->resmgr->findResource(kResourceTypeHeap, 0, 0)) {
		version_require_later_than(s, SCI_VERSION(1, 001, 000));
		return;
	}

	for (c = 0; c < 1000; c++) {
		if ((script = s->resmgr->findResource(kResourceTypeScript, c, 0))) {

			int id = (int16)READ_LE_UINT16(script->data);

			if (id > 15) {
				version_require_earlier_than(s, SCI_VERSION_FTU_NEW_SCRIPT_HEADER);
				return;
			}
		}
	}
}

SegmentId script_get_segment(EngineState *s, int script_nr, int load) {
	SegmentId segment;

	if ((load & SCRIPT_GET_LOAD) == SCRIPT_GET_LOAD)
		script_instantiate(s, script_nr);

	segment = s->seg_manager->segGet(script_nr);

	if (segment > 0) {
		if ((load & SCRIPT_GET_LOCK) == SCRIPT_GET_LOCK)
			s->seg_manager->incrementLockers(segment, SEG_ID);

		return segment;
	} else
		return 0;
}

reg_t script_lookup_export(EngineState *s, int script_nr, int export_index) {
	SegmentId seg = script_get_segment(s, script_nr, SCRIPT_GET_DONT_LOAD);
	MemObject *memobj;
	Script *script = NULL;

#ifndef DISABLE_VALIDATIONS
	if (!seg) {
		CORE_ERROR("EXPORTS", "Script invalid or not loaded");
		sciprintf("Script was script.%03d (0x%x)\n",
		          script_nr, script_nr);
		return NULL_REG;
	}
#endif

	memobj = GET_SEGMENT(*s->seg_manager, seg, MEM_OBJ_SCRIPT);

	if (memobj)
		script = &(memobj->data.script);

#ifndef DISABLE_VALIDATIONS
	if (script
	        && export_index < script->exports_nr
	        && export_index >= 0)
#endif
		return make_reg(seg, READ_LE_UINT16((byte *)(script->export_table + export_index)));
#ifndef DISABLE_VALIDATIONS
	else {
		CORE_ERROR("EXPORTS", "Export invalid or script missing ");
		if (!script)
			sciprintf("(script.%03d missing)\n", script_nr);
		else
			sciprintf("(script.%03d: Sought export %d/%d)\n",
			          script_nr, export_index, script->exports_nr);
		return NULL_REG;
	}
#endif
}

#define INST_LOOKUP_CLASS(id) ((id == 0xffff)? NULL_REG : get_class_address(s, id, SCRIPT_GET_LOCK, reg))

int script_instantiate_common(EngineState *s, int script_nr, Resource **script, Resource **heap, int *was_new) {
	int seg;
	int seg_id;
	int marked_for_deletion;
	MemObject *mem;
	reg_t reg;

	*was_new = 1;

	*script = s->resmgr->findResource(kResourceTypeScript, script_nr, 0);
	if (s->version >= SCI_VERSION(1, 001, 000))
		*heap = s->resmgr->findResource(kResourceTypeHeap, script_nr, 0);

	if (!*script || (s->version >= SCI_VERSION(1, 001, 000) && !heap)) {
		sciprintf("Script 0x%x requested but not found\n", script_nr);
		//script_debug_flag = script_error_flag = 1;
		if (s->version >= SCI_VERSION(1, 001, 000)) {
			if (*heap)
				sciprintf("Inconsistency: heap resource WAS found\n");
			else if (*script)
				sciprintf("Inconsistency: script resource WAS found\n");
		}
		return 0;
	}

	if (NULL == s) {
		sciprintf("vm.c: script_instantiate(): NULL passed for \"s\"\n");
		return 0;
	}

	seg = s->seg_manager->segGet(script_nr);
	if (s->seg_manager->scriptIsLoaded(script_nr, SCRIPT_ID)) {
		marked_for_deletion = s->seg_manager->scriptMarkedDeleted(script_nr);
		if (!marked_for_deletion) {
			s->seg_manager->incrementLockers(seg, SEG_ID);
			return seg;
		} else {
			seg_id = seg;
			mem = s->seg_manager->heap[seg];
			s->seg_manager->freeScript(mem);
		}
	} else if (!(mem = s->seg_manager->allocateScript(s, script_nr, &seg_id))) {  // ALL YOUR SCRIPT BASE ARE BELONG TO US
		sciprintf("Not enough heap space for script size 0x%x of script 0x%x, should this happen?`\n", (*script)->size, script_nr);
		script_debug_flag = script_error_flag = 1;
		return 0;
	}

	s->seg_manager->initialiseScript(mem, s, script_nr);

	reg.segment = seg_id;
	reg.offset = 0;

	// Set heap position (beyond the size word)
	s->seg_manager->setLockers(1, reg.segment, SEG_ID);
	s->seg_manager->setExportTableOffset(0, reg.segment, SEG_ID);
	s->seg_manager->setSynonymsOffset(0, reg.segment, SEG_ID);
	s->seg_manager->setSynonymsNr(0, reg.segment, SEG_ID);

	*was_new = 0;

	return seg_id;
}

int script_instantiate_sci0(EngineState *s, int script_nr) {
	int objtype;
	unsigned int objlength;
	reg_t reg, reg_tmp;
	int seg_id;
	int relocation = -1;
	int magic_pos_adder; // Usually 0; 2 for older SCI versions
	Resource *script;
	int was_new;

	seg_id = script_instantiate_common(s, script_nr, &script, NULL, &was_new);

	if (was_new)
		return seg_id;

	reg.segment = seg_id;
	reg.offset = 0;

	if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER) {
		//
		int locals_nr = READ_LE_UINT16(script->data);

		// Old script block
		// There won't be a localvar block in this case
		// Instead, the script starts with a 16 bit int specifying the
		// number of locals we need; these are then allocated and zeroed.

		s->seg_manager->mcpyInOut(0, script->data, script->size, reg.segment, SEG_ID);
		magic_pos_adder = 2;  // Step over the funny prefix

		if (locals_nr)
			s->seg_manager->scriptInitialiseLocalsZero(reg.segment, locals_nr);

	} else {
		s->seg_manager->mcpyInOut(0, script->data, script->size, reg.segment, SEG_ID);
		magic_pos_adder = 0;
	}

	// Now do a first pass through the script objects to find the
	// export table and local variable block

	objlength = 0;
	reg_tmp = reg;
	reg.offset = magic_pos_adder;

	do {
		reg_t data_base;
		reg_t addr;
		reg.offset += objlength; // Step over the last checked object
		objtype = SEG_GET_HEAP(s, reg);
		if (!objtype) break;

		objlength = SEG_GET_HEAP(s, make_reg(reg.segment, reg.offset + 2));

		data_base = reg;
		data_base.offset += 4;

		addr = data_base;

		switch (objtype) {
		case sci_obj_exports: {
			s->seg_manager->setExportTableOffset(data_base.offset, reg.segment, SEG_ID);
		}
		break;

		case sci_obj_synonyms:
			s->seg_manager->setSynonymsOffset(addr.offset, reg.segment, SEG_ID);   // +4 is to step over the header
			s->seg_manager->setSynonymsNr((objlength) / 4, reg.segment, SEG_ID);
			break;

		case sci_obj_localvars:
			s->seg_manager->scriptInitialiseLocals(data_base);
			break;

		case sci_obj_class: {
			int classpos = addr.offset - SCRIPT_OBJECT_MAGIC_OFFSET;
			int species;
			reg_tmp.offset = addr.offset - SCRIPT_OBJECT_MAGIC_OFFSET;
			species = OBJ_SPECIES(s, reg_tmp);
			if (species < 0 || species >= s->classtable_size) {
				sciprintf("Invalid species %d(0x%x) not in interval "
				          "[0,%d) while instantiating script %d\n",
				          species, species, s->classtable_size,
				          script_nr);
				script_debug_flag = script_error_flag = 1;
				return 1;
			}

			s->classtable[species].script = script_nr;
			s->classtable[species].reg = addr;
			s->classtable[species].reg.offset = classpos;
			// Set technical class position-- into the block allocated for it
		}
		break;

		default:
			break;
		}
	} while (objtype != 0);
	// And now a second pass to adjust objects and class pointers, and the general pointers

	objlength = 0;
	reg.offset = magic_pos_adder; // Reset counter

	do {
		reg_t addr;
		reg.offset += objlength; // Step over the last checked object
		objtype = SEG_GET_HEAP(s, reg);
		if (!objtype) break;
		objlength = SEG_GET_HEAP(s, make_reg(reg.segment, reg.offset + 2));
		reg.offset += 4; // Step over header

		addr = reg;

		switch (objtype) {
		case sci_obj_code:
			s->seg_manager->scriptAddCodeBlock(addr);
			break;
		case sci_obj_object:
		case sci_obj_class: { // object or class?
			Object *obj = s->seg_manager->scriptObjInit(s, addr);
			Object *base_obj;

			// Instantiate the superclass, if neccessary
			obj->variables[SCRIPT_SPECIES_SELECTOR] = INST_LOOKUP_CLASS(obj->variables[SCRIPT_SPECIES_SELECTOR].offset);

			base_obj = obj_get(s, obj->variables[SCRIPT_SPECIES_SELECTOR]);
			obj->variable_names_nr = base_obj->variables_nr;
			obj->base_obj = base_obj->base_obj;
			// Copy base from species class, as we need its selector IDs

			obj->variables[SCRIPT_SUPERCLASS_SELECTOR] = INST_LOOKUP_CLASS(obj->variables[SCRIPT_SUPERCLASS_SELECTOR].offset);
		} // if object or class
		break;
		case sci_obj_pointers: // A relocation table
			relocation = addr.offset;
			break;

		default:
			break;
		}

		reg.offset -= 4; // Step back on header

	} while ((objtype != 0) && (((unsigned)reg.offset) < script->size - 2));

	if (relocation >= 0)
		s->seg_manager->scriptRelocate(make_reg(reg.segment, relocation));

	s->seg_manager->scriptFreeUnusedObjects(reg.segment);

	return reg.segment;		// instantiation successful
}

int script_instantiate_sci11(EngineState *s, int script_nr) {
	Resource *script, *heap;
	int seg_id;
	int heap_start;
	reg_t reg;
	int was_new;

	seg_id = script_instantiate_common(s, script_nr, &script, &heap, &was_new);

	if (was_new)
		return seg_id;

	heap_start = script->size;
	if (script->size & 2)
		heap_start ++;

	s->seg_manager->mcpyInOut(0, script->data, script->size, seg_id, SEG_ID);
	s->seg_manager->mcpyInOut(heap_start, heap->data, heap->size, seg_id, SEG_ID);

	if (READ_LE_UINT16(script->data + 6) > 0)
		s->seg_manager->setExportTableOffset(6, seg_id, SEG_ID);

	reg.segment = seg_id;
	reg.offset = heap_start + 4;
	s->seg_manager->scriptInitialiseLocals(reg);

	s->seg_manager->scriptRelocateExportsSci11(seg_id);
	s->seg_manager->scriptInitialiseObjectsSci11(s, seg_id);

	reg.offset = READ_LE_UINT16(heap->data);
	s->seg_manager->heapRelocate(s, reg);

	return seg_id;
}

int script_instantiate(EngineState *s, int script_nr) {
	if (s->version >= SCI_VERSION(1, 001, 000))
		return script_instantiate_sci11(s, script_nr);
	else
		return script_instantiate_sci0(s, script_nr);
}

void script_uninstantiate_sci0(EngineState *s, int script_nr, SegmentId seg) {
	reg_t reg = make_reg(seg, (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER) ? 2 : 0);
	int objtype, objlength;

	// Make a pass over the object in order uninstantiate all superclasses
	objlength = 0;

	do {
		reg.offset += objlength; // Step over the last checked object

		objtype = SEG_GET_HEAP(s, reg);
		if (!objtype) break;
		objlength = SEG_GET_HEAP(s, make_reg(reg.segment, reg.offset + 2));  // use SEG_UGET_HEAP ??

		reg.offset += 4; // Step over header

		if ((objtype == sci_obj_object) || (objtype == sci_obj_class)) { // object or class?
			int superclass;

			reg.offset -= SCRIPT_OBJECT_MAGIC_OFFSET;

			superclass = OBJ_SUPERCLASS(s, reg); // Get superclass...

			if (superclass >= 0) {
				int superclass_script = s->classtable[superclass].script;

				if (superclass_script == script_nr) {
					if (s->seg_manager->getLockers(reg.segment, SEG_ID))
						s->seg_manager->decrementLockers(reg.segment, SEG_ID);  // Decrease lockers if this is us ourselves
				} else
					script_uninstantiate(s, superclass_script);
				// Recurse to assure that the superclass lockers number gets decreased
			}

			reg.offset += SCRIPT_OBJECT_MAGIC_OFFSET;
		} // if object or class

		reg.offset -= 4; // Step back on header

	} while (objtype != 0);
}

void script_uninstantiate(EngineState *s, int script_nr) {
	reg_t reg = make_reg(0, (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER) ? 2 : 0);
	int i;

	reg.segment = s->seg_manager->segGet(script_nr);

	if (!s->seg_manager->scriptIsLoaded(script_nr, SCRIPT_ID) || reg.segment <= 0) {   // Is it already loaded?
		//warning("unloading script 0x%x requested although not loaded", script_nr);
		// This is perfectly valid SCI behaviour
		return;
	}

	s->seg_manager->decrementLockers(reg.segment, SEG_ID);   // One less locker

	if (s->seg_manager->getLockers(reg.segment, SEG_ID) > 0)
		return;

	// Free all classtable references to this script
	for (i = 0; i < s->classtable_size; i++)
		if (s->classtable[i].reg.segment == reg.segment)
			s->classtable[i].reg = NULL_REG;

	if (s->version < SCI_VERSION(1, 001, 000))
		script_uninstantiate_sci0(s, script_nr, reg.segment);
	else
		sciprintf("FIXME: Add proper script uninstantiation for SCI 1.1\n");

	if (s->seg_manager->getLockers(reg.segment, SEG_ID))
		return; // if xxx.lockers > 0

	// Otherwise unload it completely
	// Explanation: I'm starting to believe that this work is done by SCI itself.
	s->seg_manager->markScriptDeleted(script_nr);

	if (script_checkloads_flag)
		sciprintf("Unloaded script 0x%x.\n", script_nr);

	return;
}

static void _init_stack_base_with_selector(EngineState *s, Selector selector) {
	s->stack_base[0] = make_reg(0, (uint16)selector);
	s->stack_base[1] = NULL_REG;
}

static EngineState *_game_run(EngineState *s, int restoring) {
	EngineState *successor = NULL;
	int game_is_finished = 0;
	do {
		s->execution_stack_pos_changed = 0;
		run_vm(s, (successor || restoring) ? 1 : 0);
		if (s->restarting_flags & SCI_GAME_IS_RESTARTING_NOW) { // Restart was requested?
			successor = NULL;
			free(s->execution_stack);
			s->execution_stack = NULL;
			s->execution_stack_pos = -1;
			s->execution_stack_pos_changed = 0;

			game_exit(s);
			script_free_engine(s);
			script_init_engine(s, s->version);
			game_init(s);
			sfx_reset_player();
			_init_stack_base_with_selector(s, s->selector_map.play);

			send_selector(s, s->game_obj, s->game_obj, s->stack_base, 2, s->stack_base);

			script_abort_flag = 0;
			s->restarting_flags = SCI_GAME_WAS_RESTARTED | SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE;

		} else {
			successor = s->successor;
			if (successor) {
				game_exit(s);
				script_free_vm_memory(s);
				delete s;
				s = successor;

				if (script_abort_flag == SCRIPT_ABORT_WITH_REPLAY) {
					sciprintf("Restarting with replay()\n");
					s->execution_stack_pos = -1; // Restart with replay

					_init_stack_base_with_selector(s, s->selector_map.replay);

					send_selector(s, s->game_obj, s->game_obj, s->stack_base, 2, s->stack_base);
				}

				script_abort_flag = 0;

			} else
				game_is_finished = 1;
		}
	} while (!game_is_finished);

	return s;
}

int objinfo(EngineState *s, reg_t pos);

int game_run(EngineState **_s) {
	EngineState *s = *_s;

	sciprintf(" Calling %s::play()\n", s->_gameName.c_str());
	_init_stack_base_with_selector(s, s->selector_map.play); // Call the play selector

	// Now: Register the first element on the execution stack-
	if (!send_selector(s, s->game_obj, s->game_obj, s->stack_base, 2, s->stack_base) || script_error_flag) {
		objinfo(s, s->game_obj);
		sciprintf("Failed to run the game! Aborting...\n");
		return 1;
	}
	// and ENGAGE!
	*_s = s = _game_run(s, 0);

	sciprintf(" Game::play() finished.\n");

	return 0;
}

Object *obj_get(EngineState *s, reg_t offset) {
	MemObject *memobj = GET_OBJECT_SEGMENT(*s->seg_manager, offset.segment);
	Object *obj = NULL;
	int idx;

	if (memobj != NULL) {
		if (memobj->type == MEM_OBJ_CLONES && ENTRY_IS_VALID(&memobj->data.clones, offset.offset))
			obj = &(memobj->data.clones.table[offset.offset].entry);
		else if (memobj->type == MEM_OBJ_SCRIPT) {
			if (offset.offset <= memobj->data.script.buf_size && offset.offset >= -SCRIPT_OBJECT_MAGIC_OFFSET
			        && RAW_IS_OBJECT(memobj->data.script.buf + offset.offset)) {
				idx = RAW_GET_CLASS_INDEX(&(memobj->data.script), offset);
				if (idx >= 0 && idx < memobj->data.script.objects_nr)
					obj = memobj->data.script.objects + idx;
			}
		}
	}

	return obj;
}

const char *obj_get_name(EngineState *s, reg_t pos) {
	Object *obj = obj_get(s, pos);

	if (!obj)
		return "<no such object>";

	return (const char *)(obj->base + obj->variables[SCRIPT_NAME_SELECTOR].offset);
}

void quit_vm() {
	script_abort_flag = 1; // Terminate VM
	_debugstate_valid = 0;
	_debug_seeking = 0;
	_debug_step_running = 0;
}

} // End of namespace Sci
