/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "scumm.h"

/* Open a room */
void Scumm::openRoom(int room)
{
	int room_offs, roomlimit;
	char buf[256];

	debug(9, "openRoom(%d)", room);

	/* Don't load the same room again */
	if (_lastLoadedRoom == room)
		return;
	_lastLoadedRoom = room;

	/* Room -1 means close file */
	if (room == -1) {
		_encbyte = 0;
		deleteRoomOffsets();
		fileClose(_fileHandle);
		_fileHandle = NULL;
		return;
	}

	/* Either xxx.lfl or monkey.xxx file name */
	while (!_resFilePrefix) {
#if REAL_CODE
		room_offs = _roomFileOffsets[room];
#else
		if (_features & GF_SMALL_NAMES)
			roomlimit = 98;
		else
			roomlimit = 900;
		if (_features & GF_EXTERNAL_CHARSET && room >= roomlimit)
			room_offs = 0;
		else
			room_offs = room ? _roomFileOffsets[room] : 0;
#endif

		if (room_offs == (int)0xFFFFFFFF)
			break;

		if (room_offs != 0 && room != 0) {
			_fileOffset = _roomFileOffsets[room];
			return;
		}
		if (!(_features & GF_SMALL_HEADER)) {
			if (_features & GF_AFTER_V7)
				sprintf(buf, "%s%s.la%d", _gameDataPath, _exe_name,
								room == 0 ? 0 : res.roomno[rtRoom][room]);
			else
				sprintf(buf, "%s%s.%.3d", _gameDataPath, _exe_name,
								room == 0 ? 0 : res.roomno[rtRoom][room]);
			_encbyte = (_features & GF_USE_KEY) ? 0x69 : 0;
		} else if (!(_features & GF_SMALL_NAMES)) {
			if (room == 0 || room >= 900) {
				sprintf(buf, "%s%.3d.lfl", _gameDataPath, room);
				_encbyte = 0;
				if (openResourceFile(buf)) {
					return;
				}
				askForDisk(buf);

			} else {
				sprintf(buf, "%sdisk%.2d.lec", _gameDataPath,
								res.roomno[rtRoom][room]);
				_encbyte = 0x69;
			}
		} else {
			sprintf(buf, "%s%.2d.lfl", _gameDataPath, room);
			if (_features & GF_OLD_BUNDLE)
				_encbyte = 0xFF;
			else
				_encbyte = 0;
		}

		if (openResourceFile(buf)) {
			if (room == 0)
				return;
			if (_features & GF_EXTERNAL_CHARSET && room >= roomlimit)
				return;
			readRoomsOffsets();
			_fileOffset = _roomFileOffsets[room];

			if (_fileOffset != 8)
				return;

			error("Room %d not in %s", room, buf);
			return;
		}
		askForDisk(buf);
	}

	do {
		sprintf(buf, "%.3d.lfl", room);
		_encbyte = 0;
		if (openResourceFile(buf))
			break;
		askForDisk(buf);
	} while (1);

	deleteRoomOffsets();
	_fileOffset = 0;							/* start of file */
}

/* Delete the currently loaded room offsets */
void Scumm::deleteRoomOffsets()
{
	if (!(_features & GF_SMALL_HEADER) && !_dynamicRoomOffsets)
		return;

	for (int i = 0; i < _maxRooms; i++) {
		if (_roomFileOffsets[i] != 0xFFFFFFFF)
			_roomFileOffsets[i] = 0;
	}
}

/* Read room offsets */
void Scumm::readRoomsOffsets()
{
	int num, room;

	debug(9, "readRoomOffsets()");

	deleteRoomOffsets();
	if (_features & GF_SMALL_NAMES)
		return;

	if (!(_features & GF_SMALL_HEADER)) {
		if (!_dynamicRoomOffsets)
			return;

		fileSeek(_fileHandle, 16, SEEK_SET);
	} else {
		fileSeek(_fileHandle, 12, SEEK_SET);	// Directlry searching for the room offset block would be more generic...
	}

	num = fileReadByte();
	while (num) {
		num--;

		room = fileReadByte();
		if (_roomFileOffsets[room] != 0xFFFFFFFF) {
			_roomFileOffsets[room] = fileReadDwordLE();
		} else {
			fileReadDwordLE();
		}
	}
}

bool Scumm::openResourceFile(const char *filename)
{
	char buf[256];

	debug(9, "openResourceFile(%s)", filename);

	if (_resFilePath) {
#if defined(macintosh)
		sprintf(buf, ":%s.%d:%s", _resFilePath, _resFilePathId, filename);
#else
		sprintf(buf, "%s.%d\\%s", _resFilePath, _resFilePathId, filename);
#endif
	} else if (_resFilePrefix) {
		sprintf(buf, "%s%s", _resFilePrefix, filename);
	} else {
		strcpy(buf, filename);
	}

	if (_fileHandle != NULL) {
		fileClose(_fileHandle);
		_fileHandle = NULL;
	}

	_fileHandle = fileOpen(buf, 1);

	return _fileHandle != NULL;
}

void Scumm::askForDisk(const char *filename)
{
	error("Cannot find '%s'", filename);
}

void Scumm::readIndexFile()
{
	uint32 blocktype, itemsize;
	int numblock = 0;
	int num, i;

	debug(9, "readIndexFile()");

	openRoom(-1);
	openRoom(0);

	if (!(_features & GF_AFTER_V6)) {
		/* Figure out the sizes of various resources */
		while (!fileEof(_fileHandle)) {
			blocktype = fileReadDword();
			itemsize = fileReadDwordBE();
			if (fileReadFailed(_fileHandle))
				break;
			switch (blocktype) {
			case MKID('DOBJ'):
				_numGlobalObjects = fileReadWordLE();
				itemsize -= 2;
				break;
			case MKID('DROO'):
				_numRooms = fileReadWordLE();
				itemsize -= 2;
				break;

			case MKID('DSCR'):
				_numScripts = fileReadWordLE();
				itemsize -= 2;
				break;

			case MKID('DCOS'):
				_numCostumes = fileReadWordLE();
				itemsize -= 2;
				break;

			case MKID('DSOU'):
				_numSounds = fileReadWordLE();
				itemsize -= 2;
				break;
			}
			fileSeek(_fileHandle, itemsize - 8, SEEK_CUR);
		}
		clearFileReadFailed(_fileHandle);
		fileSeek(_fileHandle, 0, SEEK_SET);
	}

	while (1) {
		blocktype = fileReadDword();

		if (fileReadFailed(_fileHandle))
			break;
		itemsize = fileReadDwordBE();

		numblock++;

		switch (blocktype) {
		case MKID('DCHR'):
			readResTypeList(rtCharset, MKID('CHAR'), "charset");
			break;

		case MKID('DOBJ'):
			num = fileReadWordLE();
			assert(num == _numGlobalObjects);

			if (_features & GF_AFTER_V7) {
				fileRead(_fileHandle, _objectStateTable, num);
				fileRead(_fileHandle, _objectRoomTable, num);
				memset(_objectOwnerTable, 0xFF, num);

			} else {
				fileRead(_fileHandle, _objectOwnerTable, num);
				for (i = 0; i < num; i++) {
					_objectStateTable[i] = _objectOwnerTable[i] >> OF_STATE_SHL;
					_objectOwnerTable[i] &= OF_OWNER_MASK;
				}
			}
			fileRead(_fileHandle, _classData, num * sizeof(uint32));

			/* This code should be here. Otherwise the flags will be swapped for big endian computers.
			 * If it doesn't work with this code, something else is wrong */
#if defined(SCUMM_BIG_ENDIAN)
			for (i = 0; i != num; i++) {
				_classData[i] = FROM_LE_32(_classData[i]);
			}
#endif
			break;

		case MKID('RNAM'):
		case MKID('ANAM'):
			fileSeek(_fileHandle, itemsize - 8, SEEK_CUR);
			break;

		case MKID('DROO'):
			readResTypeList(rtRoom, MKID('ROOM'), "room");
			break;

		case MKID('DSCR'):
			readResTypeList(rtScript, MKID('SCRP'), "script");
			break;

		case MKID('DCOS'):
			readResTypeList(rtCostume, MKID('COST'), "costume");
			break;

		case MKID('MAXS'):
			readMAXS();
			break;

		case MKID('DSOU'):
			readResTypeList(rtSound, MKID('SOUN'), "sound");
			break;

		case MKID('AARY'):
			readArrayFromIndexFile();
			break;

		default:
			error("Bad ID %c%c%c%c found in directory!", blocktype & 0xFF,
						blocktype >> 8, blocktype >> 16, blocktype >> 24);
			return;
		}
	}

//  if (numblock!=9)
//    error("Not enough blocks read from directory");

	openRoom(-1);
}

void Scumm::readArrayFromIndexFile()
{
	int num;
	int a, b, c;

	while ((num = fileReadWordLE()) != 0) {
		a = fileReadWordLE();
		b = fileReadWordLE();
		c = fileReadWordLE();
		if (c == 1)
			defineArray(num, 1, a, b);
		else
			defineArray(num, 5, a, b);
	}
}

void Scumm::readResTypeList(int id, uint32 tag, const char *name)
{
	int num;
	int i;

	debug(9, "readResTypeList(%s,%x,%s)", resTypeFromId(id), FROM_LE_32(tag), name);

	num = fileReadWordLE();

	if (1 || _features & GF_AFTER_V6) {
		if (num != res.num[id]) {
			error("Invalid number of %ss (%d) in directory", name, num);
		}
	} else {
		if (num >= 0xFF) {
			error("Too many %ss (%d) in directory", name, num);
		}
		allocResTypeData(id, tag, num, name, 1);
	}

	if (_features & GF_SMALL_HEADER) {
		for (i = 0; i < num; i++) {
			res.roomno[id][i] = fileReadByte();
			res.roomoffs[id][i] = fileReadDword();
		}
	} else {
		fileRead(_fileHandle, res.roomno[id], num * sizeof(uint8));
		fileRead(_fileHandle, res.roomoffs[id], num * sizeof(uint32));
	}

#if defined(SCUMM_BIG_ENDIAN)
	for (i = 0; i < num; i++)
		res.roomoffs[id][i] = FROM_LE_32(res.roomoffs[id][i]);
#endif
}


void Scumm::allocResTypeData(int id, uint32 tag, int num, const char *name,
														 int mode)
{
	debug(9, "allocResTypeData(%s/%s,%x,%d,%d)", resTypeFromId(id), name, FROM_LE_32(tag), num, mode);
	assert(id >= 0 && id < (int)(sizeof(res.mode) / sizeof(res.mode[0])));

	if (num >= 2000) {
		error("Too many %ss (%d) in directory", name, num);
	}

	res.mode[id] = mode;
	res.num[id] = num;
	res.tags[id] = tag;
	res.name[id] = name;
	res.address[id] = (byte **)calloc(num, sizeof(void *));
	res.flags[id] = (byte *)calloc(num, sizeof(byte));

	if (mode) {
		res.roomno[id] = (byte *)calloc(num, sizeof(byte));
		res.roomoffs[id] = (uint32 *)calloc(num, sizeof(uint32));
	}
}

void Scumm::loadCharset(int no)
{
	int i;
	byte *ptr;

	debug(9, "loadCharset(%d)", no);

	/* FIXME - hack around crash in Indy4 (occurs if you try to load after dieing) */
	if (_gameId == GID_INDY4 && no == 0)
		no = 1;

	memset(_charsetData, 0, sizeof(_charsetData));

	assert(no < (int) sizeof(_charsetData)/16);
	checkRange(_maxCharsets - 1, 1, no, "Loading illegal charset %d");

//  ensureResourceLoaded(6, no);
	ptr = getResourceAddress(6, no);

	for (i = 0; i < 15; i++) {
		_charsetData[no][i + 1] = ptr[i + 14];
	}
}

void Scumm::nukeCharset(int i)
{
	checkRange(_maxCharsets - 1, 1, i, "Nuking illegal charset %d");
	nukeResource(rtCharset, i);
}

void Scumm::ensureResourceLoaded(int type, int i)
{
	void *addr;

	debug(9, "ensureResourceLoaded(%s,%d)", resTypeFromId(type), i);

	if (type == rtRoom && i > 127) {
		i = _resourceMapper[i & 127];
	}

	if (i == 0)
		return;

	addr = res.address[type][i];
	if (addr)
		return;

	loadResource(type, i);

	if (!(_features & GF_AFTER_V7))
		if (type == rtRoom && i == _roomResource)
			_vars[VAR_ROOM_FLAG] = 1;
}

int Scumm::loadResource(int type, int idx)
{
	int roomNr, i;
	uint32 fileOffs;
	uint32 size, tag;

    // debug(1, "loadResource(%s,%d)", resTypeFromId(type),idx);

	if (type == rtCharset && (_features & GF_SMALL_HEADER)) {
		loadCharset(idx);
		return (1);
	}

	roomNr = getResourceRoomNr(type, idx);

	if (idx >= res.num[type])
		error("%s %d undefined %d %d", res.name[type], idx, res.num[type], roomNr);

	if (roomNr == 0)
		roomNr = _roomResource;

	if (type == rtRoom) {
		fileOffs = 0;
	} else {
		fileOffs = res.roomoffs[type][idx];
		if (fileOffs == 0xFFFFFFFF)
			return 0;
	}

	do {
		for (i = 0; i < 5; i++) {
			openRoom(roomNr);

			fileSeek(_fileHandle, fileOffs + _fileOffset, SEEK_SET);

			if (_features & GF_SMALL_HEADER) {
				if (!(_features & GF_SMALL_NAMES))
					fileSeek(_fileHandle, 8, SEEK_CUR);
				size = fileReadDwordLE();
				tag = fileReadWordLE();
				fileSeek(_fileHandle, -6, SEEK_CUR);
			} else {
				if (type == rtSound) {
					fileReadDwordLE();
					fileReadDwordLE();
					return readSoundResource(type, idx);
				}

				tag = fileReadDword();

				if (tag != res.tags[type]) {
					error("%s %d not in room %d at %d+%d",
								res.name[type], type, roomNr, _fileOffset, fileOffs);
				}

				size = fileReadDwordBE();
				fileSeek(_fileHandle, -8, SEEK_CUR);
			}
			fileRead(_fileHandle, createResource(type, idx, size), size);

			/* dump the resource */
#ifdef DUMP_SCRIPTS
			if (type == rtScript) {
				dumpResource("script-", idx, getResourceAddress(rtScript, idx));
			}
#endif

			if (!fileReadFailed(_fileHandle)) {
				return 1;
			}

			nukeResource(type, idx);
		}

		error("Cannot read resource");
	} while (1);
}

int Scumm::readSoundResource(int type, int idx)
{
	uint32 pos, total_size, size, tag, basetag;
	int pri, best_pri;
	uint32 best_size = 0, best_offs = 0;

	debug(9, "readSoundResource(%s,%d)", resTypeFromId(type), idx);

	pos = 0;

	basetag = fileReadDword();
	total_size = fileReadDwordBE();

	//if (_gameId == GID_SAMNMAX || _features & GF_AFTER_V7) {
	if (basetag == MKID('MIDI') || basetag == MKID('iMUS')) {
		fileSeek(_fileHandle, -8, SEEK_CUR);
		fileRead(_fileHandle, createResource(type, idx, total_size + 8),
						 total_size + 8);
		return 1;
	} else if (basetag == MKID('SOU ')) {
		best_pri = -1;
		while (pos < total_size) {
			tag = fileReadDword();
			size = fileReadDwordBE() + 8;
			pos += size;

			pri = -1;
		
			switch (tag) {
			case MKID('ADL '):
				if (_use_adlib)
					pri = 10;
				break;
			case MKID('ROL '):
				if (!_use_adlib)
					pri = 2;
				break;
			case MKID('GMD '):
				if (!_use_adlib)
					pri = 3;
				break;
			case MKID('MAC '):
				if (!_use_adlib)
					pri = 1;
				break;
			case MKID('SPK '):
				if (!_use_adlib)
					pri = 0;
				break;
			}

			if (pri > best_pri) {
				best_pri = pri;
				best_size = size;
				best_offs = filePos(_fileHandle);
			}

			fileSeek(_fileHandle, size - 8, SEEK_CUR);
		}

		if (best_pri != -1) {
			fileSeek(_fileHandle, best_offs - 8, SEEK_SET);
			fileRead(_fileHandle, createResource(type, idx, best_size), best_size);
			return 1;
		}
	} else if (FROM_LE_32(basetag) == 24) {
		fileSeek(_fileHandle, -12, SEEK_CUR);
		total_size = fileReadDwordBE();
		fileSeek(_fileHandle, -8, SEEK_CUR);
		fileRead(_fileHandle, createResource(type, idx, total_size), total_size);
		return 1;
	} else if (basetag == MKID('Mac0')) {
		debug(1, "Ignoring base tag Mac0 in sound %d, size %d", idx, total_size);
		debug(1, "It was at position %d", filePos(_fileHandle));
		
		/* Offset
		   0x14, 0x1C, 0x20, 0x24 - offsets of channel 1/2/3/4 chunk-
		   Each channel has tag "Chan", followed by its length. At the end
		   of each chan follows either an empty "Done" chunk (length 0) or an
		   empty "Loop" chunk. Maybe "Loop" indicates the song should be
		   played forever?!?.
		   
		   For Mac1, it follows a chunk "Inst" contains the same data as a
		   MacOS 'snd ' resource, i.e. sampled sound data (8bit). This is
		   used for sound effects (like the sea gull in Monkey Island).
		
		   For Mac0, there can be various different subchunks it seems. The
		   following combinations appear in MI
		     ORGA, SHAK, BASS (ID 101)
		     ORGA, MARI, BASS (ID 108)
		     VIBE, WHIS, BASS (ID 104)
		     ORGA, SHAK, VIBE (ID 110)
		   Guess is that these are instrument names: Organ, Marimba, Whistle...
		   Maybe there is a mapping table someplace?
		   
		   What follows are four byte "commands" it seems, like this (hex):
		     01 68 4F 49
		     01 68 00 40
		     01 68 4F 49
		     ...
		     01 68 00 40
		     02 1C 5B 40
		     00 B4 00 40
		     ...
		     01 68 37 3C
		     00 18 37 38
		     04 20 3E 34
		     01 68 4A 3C
		   
		   More data:
		     00 09 3E 10
		     01 5F 00 40
		     00 9C 36 40
		     00 CC 00 40
		     00 18 42 49
		     00 18 45 3C
		     01 29 4A 3C
		     00 0F 00 40
		     
		   Maybe I am mistaken when I think it's four byte, some other parts
		   seem to suggest it's 2 byte oriented, or even variable lenght...
		 */
	} else if (basetag == MKID('Mac1')) {
		debug(1, "Ignoring base tag Mac1 in sound %d, size %d", idx, total_size);
		debug(1, "It was at position %d", filePos(_fileHandle));
	} else {
		fprintf(stderr, "WARNING: Unrecognized base tag 0x%08lx in sound %d\n",
					basetag, idx);
	}
	res.roomoffs[type][idx] = 0xFFFFFFFF;
	return 0;
}

int Scumm::getResourceRoomNr(int type, int idx)
{
	if (type == rtRoom)
		return idx;
	return res.roomno[type][idx];
}

byte *Scumm::getResourceAddress(int type, int idx)
{
	byte *ptr;
	

	CHECK_HEAP validateResource("getResourceAddress", type, idx);
	if (!res.address[type]) {
		debug(9, "getResourceAddress(%s,%d) == NULL", resTypeFromId(type), idx);
		return NULL;
	}

	if (res.mode[type] && !res.address[type][idx]) {
		ensureResourceLoaded(type, idx);
	}


	if (!(ptr = (byte *)res.address[type][idx])) {
		debug(9, "getResourceAddress(%s,%d) == NULL", resTypeFromId(type), idx);
		return NULL;
	}

	setResourceCounter(type, idx, 1);

	debug(9, "getResourceAddress(%s,%d) == %ld", resTypeFromId(type), idx, ptr + sizeof(MemBlkHeader));
	return ptr + sizeof(MemBlkHeader);
}

byte *Scumm::getStringAddress(int i)
{
	byte *b = getResourceAddress(rtString, i);
	if (!b)
		return b;

	if (_features & GF_NEW_OPCODES)
		return ((ArrayHeader *)b)->data;
	return b;
}



byte *Scumm::getStringAddressVar(int i) {
	byte *addr;

	addr=getResourceAddress(rtString,_vars[i]);
	if(addr==NULL)
		error("NULL string var %d slot %d",i,_vars[i]);

	if (_features & GF_NEW_OPCODES)
			return ((ArrayHeader *)addr)->data;

	return(addr);
}

void Scumm::setResourceCounter(int type, int idx, byte flag)
{
	res.flags[type][idx] &= ~RF_USAGE;
	res.flags[type][idx] |= flag;
}

/* 2 bytes safety area to make "precaching" of bytes in the gdi drawer easier */
#define SAFETY_AREA 2

byte *Scumm::createResource(int type, int idx, uint32 size)
{
	byte *ptr;

	CHECK_HEAP
	debug(9, "createResource(%s,%d,%d)", resTypeFromId(type), idx, size);

	validateResource("allocating", type, idx);
	nukeResource(type, idx);

	expireResources(size);

	CHECK_HEAP ptr = (byte *)calloc(size + sizeof(MemBlkHeader) + SAFETY_AREA, 1);
	if (ptr == NULL) {
		error("Out of memory while allocating %d", size);
	}

	_allocatedSize += size;

	res.address[type][idx] = ptr;
	((MemBlkHeader *)ptr)->size = size;
	setResourceCounter(type, idx, 1);
	return ptr + sizeof(MemBlkHeader);	/* skip header */
}

void Scumm::validateResource(const char *str, int type, int idx)
{
	if (type < rtFirst || type > rtLast || (uint) idx >= (uint) res.num[type]) {
		warning("%s Illegal Glob type %s (%d) num %d", str, resTypeFromId(type), type, idx);
	}
}

void Scumm::nukeResource(int type, int idx)
{
	byte *ptr;

	debug(9, "nukeResource(%s,%d)", resTypeFromId(type), idx);

	CHECK_HEAP if (!res.address[type])
		return;

	assert(idx >= 0 && idx < res.num[type]);

	if ((ptr = res.address[type][idx]) != NULL) {
		res.address[type][idx] = 0;
		res.flags[type][idx] = 0;
		_allocatedSize -= ((MemBlkHeader *)ptr)->size;
		free(ptr);
	}
}

byte *Scumm::findResourceData(uint32 tag, byte *ptr)
{
	if (_features & GF_SMALL_HEADER) {
		ptr = findResourceSmall(tag, ptr, 0);
		if (ptr == NULL)
			return NULL;
		return ptr + 6;
	}

	ptr = findResource(tag, ptr, 0);
	if (ptr == NULL)
		return NULL;
	return ptr + 8;
}

int Scumm::getResourceDataSize(byte *ptr)
{
	if (ptr == NULL)
		return 0;

	if (_features & GF_SMALL_HEADER)
		return READ_LE_UINT32(ptr) - 6;
	else
		return READ_BE_UINT32_UNALIGNED(ptr - 4) - 8;
}

struct FindResourceState {
	uint32 size, pos;
	byte *ptr;
};

/* just O(N) complexity when iterating with this function */
byte *findResource(uint32 tag, byte *searchin)
{
	uint32 size;
	static FindResourceState frs;
	FindResourceState *f = &frs;	/* easier to make it thread safe like this */

	if (searchin) {
		f->size = READ_BE_UINT32_UNALIGNED(searchin + 4);
		f->pos = 8;
		f->ptr = searchin + 8;
		goto StartScan;
	}

	do {
		size = READ_BE_UINT32_UNALIGNED(f->ptr + 4);
		if ((int32) size <= 0)
			return NULL;

		f->pos += size;
		f->ptr += size;

	StartScan:
		if (f->pos >= f->size)
			return NULL;
	} while (READ_UINT32_UNALIGNED(f->ptr) != tag);

	return f->ptr;
}

byte *findResourceSmall(uint32 tag, byte *searchin)
{
	uint32 size;
	static FindResourceState frs;
	FindResourceState *f = &frs;	/* easier to make it thread safe like this */
	uint16 smallTag;

	smallTag = newTag2Old(tag);

	if (searchin) {
		f->size = READ_LE_UINT32(searchin);
		f->pos = 6;
		f->ptr = searchin + 6;
		goto StartScan;
	}

	do {
		size = READ_LE_UINT32(f->ptr);
		if ((int32) size <= 0)
			return NULL;

		f->pos += size;
		f->ptr += size;

	StartScan:
		if (f->pos >= f->size)
			return NULL;
	} while (READ_LE_UINT16(f->ptr + 4) != smallTag);

	return f->ptr;
}

byte *findResource(uint32 tag, byte *searchin, int idx)
{
	uint32 curpos, totalsize, size;

	assert(searchin);

	searchin += 4;
	totalsize = READ_BE_UINT32_UNALIGNED(searchin);
	curpos = 8;
	searchin += 4;

	while (curpos < totalsize) {
		if (READ_UINT32_UNALIGNED(searchin) == tag && !idx--)
			return searchin;

		size = READ_BE_UINT32_UNALIGNED(searchin + 4);
		if ((int32) size <= 0) {
			error("(%c%c%c%c) Not found in %d... illegal block len %d",
						tag & 0xFF, (tag >> 8) & 0xFF, (tag >> 16) & 0xFF,
						(tag >> 24) & 0xFF, 0, size);
			return NULL;
		}

		curpos += size;
		searchin += size;
	}

	return NULL;
}

byte *findResourceSmall(uint32 tag, byte *searchin, int idx)
{
	uint32 curpos, totalsize, size;
	uint16 smallTag;

	smallTag = newTag2Old(tag);

	assert(searchin);

	totalsize = READ_LE_UINT32(searchin);
	searchin += 6;
	curpos = 6;

	while (curpos < totalsize) {
		size = READ_LE_UINT32(searchin);

		if (READ_LE_UINT16(searchin + 4) == smallTag && !idx--)
			return searchin;

		if ((int32) size <= 0) {
			error("(%c%c%c%c) Not found in %d... illegal block len %d",
						tag & 0xFF, (tag >> 8) & 0xFF, (tag >> 16) & 0xFF,
						(tag >> 24) & 0xFF, 0, size);
			return NULL;
		}

		curpos += size;
		searchin += size;
	}

	return NULL;
}

void Scumm::lock(int type, int i)
{
	validateResource("Locking", type, i);
	res.flags[type][i] |= RF_LOCK;

//  debug(1, "locking %d,%d", type, i);
}

void Scumm::unlock(int type, int i)
{
	validateResource("Unlocking", type, i);
	res.flags[type][i] &= ~RF_LOCK;

//  debug(1, "unlocking %d,%d", type, i);
}

bool Scumm::isResourceInUse(int type, int i)
{
	validateResource("isResourceInUse", type, i);
	switch (type) {
	case rtRoom:
		return _roomResource == (byte)i;
	case rtScript:
		return isScriptInUse(i);
	case rtCostume:
		return isCostumeInUse(i);
	case rtSound:
		return isSoundRunning(i) != 0;
	default:
		return false;
	}
}

void Scumm::increaseResourceCounter()
{
	int i, j;
	byte counter;

	for (i = rtFirst; i <= rtLast; i++) {
		for (j = res.num[i]; --j >= 0;) {
			counter = res.flags[i][j] & RF_USAGE;
			if (counter && counter < RF_USAGE_MAX) {
				setResourceCounter(i, j, counter + 1);
			}
		}
	}
}

void Scumm::expireResources(uint32 size)
{
	int i, j;
	byte flag;
	byte best_counter;
	int best_type, best_res = 0;
	uint32 oldAllocatedSize;

//	return;

	if (_expire_counter != 0xFF) {
		_expire_counter = 0xFF;
		increaseResourceCounter();
	}

	if (size + _allocatedSize < _maxHeapThreshold)
		return;

	oldAllocatedSize = _allocatedSize;

	do {
		best_type = 0;
		best_counter = 2;

		for (i = rtFirst; i <= rtLast; i++)
			if (res.mode[i]) {
				for (j = res.num[i]; --j >= 0;) {
					flag = res.flags[i][j];
					if (!(flag & 0x80) && flag >= best_counter
							&& res.address[i][j] && !isResourceInUse(i, j)) {
						best_counter = flag;
						best_type = i;
						best_res = j;
					}
				}
			}

		if (!best_type)
			break;
		nukeResource(best_type, best_res);
	} while (size + _allocatedSize > _minHeapThreshold);

	increaseResourceCounter();

	debug(5, "Expired resources, mem %d -> %d", oldAllocatedSize,
				_allocatedSize);
}

void Scumm::freeResources()
{
	int i, j;
	for (i = rtFirst; i <= rtLast; i++) {
		for (j = res.num[i]; --j >= 0;) {
			if (isResourceLoaded(i, j))
				nukeResource(i, j);
		}
		free(res.address[i]);
		free(res.flags[i]);
		free(res.roomno[i]);
		free(res.roomoffs[i]);
	}
}

void Scumm::loadPtrToResource(int type, int resindex, byte *source)
{
	byte *alloced;
	int i, len;

	nukeResource(type, resindex);

	len = getStringLen(source);

	if (len <= 0)
		return;

	alloced = createResource(type, resindex, len);

	if (!source) {
		alloced[0] = fetchScriptByte();
		for (i = 1; i < len; i++)
			alloced[i] = *_scriptPointer++;
	} else {
		for (i = 0; i < len; i++)
			alloced[i] = source[i];
	}
}

bool Scumm::isResourceLoaded(int type, int idx)
{
	validateResource("isLoaded", type, idx);
	return res.address[type][idx] != NULL;
}

void Scumm::resourceStats()
{
	int i, j;
	uint32 lockedSize = 0, lockedNum = 0;
	byte flag;

	for (i = rtFirst; i <= rtLast; i++)
		for (j = res.num[i]; --j >= 0;) {
			flag = res.flags[i][j];
			if (flag & 0x80 && res.address[i][j]) {
				lockedSize += ((MemBlkHeader *)res.address[i][j])->size;
				lockedNum++;
			}
		}

	printf("Total allocated size=%ld, locked=%ld(%ld)\n", _allocatedSize,
				 lockedSize, lockedNum);
}

void Scumm::heapClear(int mode)
{
}

void Scumm::unkHeapProc2(int a, int b)
{
}

void Scumm::readMAXS()
{
	if (_features & GF_AFTER_V7) {
		fileSeek(_fileHandle, 50 + 50, SEEK_CUR);
		_numVariables = fileReadWordLE();
		_numBitVariables = fileReadWordLE();
		fileReadWordLE();
		_numGlobalObjects = fileReadWordLE();
		_numLocalObjects = fileReadWordLE();
		_numNewNames = fileReadWordLE();
		_numVerbs = fileReadWordLE();
		_numFlObject = fileReadWordLE();
		_numInventory = fileReadWordLE();
		_numArray = fileReadWordLE();
		_numRooms = fileReadWordLE();
		_numScripts = fileReadWordLE();
		_numSounds = fileReadWordLE();
		_numCharsets = fileReadWordLE();
		_numCostumes = fileReadWordLE();

		_objectRoomTable = (byte *)calloc(_numGlobalObjects, 1);
		_numGlobalScripts = 2000;

		_shadowPaletteSize = NUM_SHADOW_PALETTE * 256;
	} else if (_features & GF_AFTER_V6) {
		_numVariables = fileReadWordLE();
		fileReadWordLE();
		_numBitVariables = fileReadWordLE();
		_numLocalObjects = fileReadWordLE();
		_numArray = fileReadWordLE();
		fileReadWordLE();
		_numVerbs = fileReadWordLE();
		_numFlObject = fileReadWordLE();
		_numInventory = fileReadWordLE();
		_numRooms = fileReadWordLE();
		_numScripts = fileReadWordLE();
		_numSounds = fileReadWordLE();
		_numCharsets = fileReadWordLE();
		_numCostumes = fileReadWordLE();
		_numGlobalObjects = fileReadWordLE();
		_numNewNames = 50;

		_objectRoomTable = NULL;
		_numGlobalScripts = 200;

		_shadowPaletteSize = 256;
	} else {
		_numVariables = fileReadWordLE();	/* 800 */
		fileReadWordLE();						/* 16 */
		_numBitVariables = fileReadWordLE();	/* 2048 */
		_numLocalObjects = fileReadWordLE();	/* 200 */
		_numArray = 50;
		_numVerbs = 100;
		_numNewNames = 0;
		_objectRoomTable = NULL;

		fileReadWordLE();						/* 50 */
		_numCharsets = fileReadWordLE();	/* 9 */
		fileReadWordLE();						/* 100 */
		fileReadWordLE();						/* 50 */
		_numInventory = fileReadWordLE();	/* 80 */
		_numGlobalScripts = 200;

		_shadowPaletteSize = 256;

		_numFlObject = 50;
	}

	if (_shadowPaletteSize)
		_shadowPalette = (byte *)calloc(_shadowPaletteSize, 1);

	allocateArrays();
	_dynamicRoomOffsets = 1;
}

void Scumm::allocateArrays()
{
	// Note: Buffers are now allocated in scummMain to allow for
	//     early GUI init.

	_objectOwnerTable = (byte *)calloc(_numGlobalObjects, 1);
	_objectStateTable = (byte *)calloc(_numGlobalObjects, 1);
	_classData = (uint32 *)calloc(_numGlobalObjects, sizeof(uint32));
	_arrays = (byte *)calloc(_numArray, 1);
	_newNames = (uint16 *)calloc(_numNewNames, sizeof(uint16));

	_inventory = (uint16 *)calloc(_numInventory, sizeof(uint16));
	_verbs = (VerbSlot *)calloc(_numVerbs, sizeof(VerbSlot));
	_objs = (ObjectData *)calloc(_numLocalObjects, sizeof(ObjectData));
	_vars = (int16 *) calloc(_numVariables, sizeof(int16));
	_bitVars = (byte *)calloc(_numBitVariables >> 3, 1);

	allocResTypeData(rtCostume,
									 (_features & GF_NEW_COSTUMES) ? MKID('AKOS') :
									 MKID('COST'), _numCostumes, "costume", 1);

	allocResTypeData(rtRoom, MKID('ROOM'), _numRooms, "room", 1);
	allocResTypeData(rtSound, MKID('SOUN'), _numSounds, "sound", 1);
	allocResTypeData(rtScript, MKID('SCRP'), _numScripts, "script", 1);
	allocResTypeData(rtCharset, MKID('CHAR'), _numCharsets, "charset", 1);
	allocResTypeData(rtObjectName, MKID('NONE'), _numNewNames, "new name", 0);
	allocResTypeData(rtInventory, MKID('NONE'), _numInventory, "inventory", 0);
	allocResTypeData(rtTemp, MKID('NONE'), 10, "temp", 0);
	allocResTypeData(rtScaleTable, MKID('NONE'), 5, "scale table", 0);
	allocResTypeData(rtActorName, MKID('NONE'), NUM_ACTORS, "actor name", 0);
	allocResTypeData(rtVerb, MKID('NONE'), _numVerbs, "verb", 0);
	allocResTypeData(rtString, MKID('NONE'), _numArray, "array", 0);
	allocResTypeData(rtFlObject, MKID('NONE'), _numFlObject, "flobject", 0);
	allocResTypeData(rtMatrix, MKID('NONE'), 10, "boxes", 0);
}




bool Scumm::isGlobInMemory(int type, int idx)
{
	validateResource("isGlobInMemory",type,idx);

	return res.address[type][idx] != NULL;
}


uint16 newTag2Old(uint32 oldTag)
{
	switch (oldTag) {
	case (MKID('RMHD')):
		return (0x4448);
		break;
	case (MKID('IM00')):
		return (0x4D42);
		break;
	case (MKID('EXCD')):
		return (0x5845);
		break;
	case (MKID('ENCD')):
		return (0x4E45);
		break;
	case (MKID('SCAL')):
		return (0x4153);
		break;
	case (MKID('LSCR')):
		return (0x534C);
		break;
	case (MKID('OBCD')):
		return (0x434F);
		break;
	case (MKID('OBIM')):
		return (0x494F);
		break;
	case (MKID('SMAP')):
		return (0x4D42);
		break;
	case (MKID('CLUT')):
		return (0x4150);
		break;
	case (MKID('BOXD')):
		return (0x5842);
		break;
	default:
		return (0);
	}
}


char *Scumm::resTypeFromId(int id) {
	static char buf[100];

 	switch(id) {
		case rtRoom: 	  sprintf(buf, "Room"); break;
		case rtScript: 	  sprintf(buf, "Script"); break;
		case rtCostume:   sprintf(buf, "Costume"); break;
		case rtSound: 	  sprintf(buf, "Sound"); break;
		case rtInventory: sprintf(buf, "Inventory"); break;
		case rtCharset:   sprintf(buf, "Charset"); break;
		case rtString: 	  sprintf(buf, "String"); break;
		case rtVerb:	  sprintf(buf, "Verb"); break;
		case rtActorName: sprintf(buf, "ActorName"); break;
		case rtBuffer: 	  sprintf(buf, "Buffer"); break;
		case rtScaleTable:sprintf(buf, "ScaleTable"); break;
		case rtTemp: 	  sprintf(buf, "Temp"); break;
		case rtFlObject:  sprintf(buf, "FlObject"); break;
		case rtMatrix: 	  sprintf(buf, "Matrix"); break;
		case rtBox: 	  sprintf(buf, "Box"); break;
		case rtLast: 	  sprintf(buf, "Last"); break;
		case rtNumTypes:  sprintf(buf, "NumTypes"); break;
		default: sprintf(buf,"%d", id);
	}
	return buf;
}
