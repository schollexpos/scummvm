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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// Disable symbol overrides so that we can use system headers.
#define FORBIDDEN_SYMBOL_ALLOW_ALL

// HACK to allow building with the SDL backend on MinGW
// see bug #3412 "TOOLS: MinGW tools building broken"
#ifdef main
#undef main
#endif // main

#include "common/language.h"
#include "constants.h"

#include "en_constants.h"
#include "ru_constants.h"

void LangConstants::writeConstants(Common::String num, CCArchive &cc) {
	Common::MemFile file;
	file.syncString(CLOUDS_CREDITS());
	file.syncString(DARK_SIDE_CREDITS());
	file.syncString(SWORDS_CREDITS1());
	file.syncString(SWORDS_CREDITS2());
	file.syncString(OPTIONS_MENU());
	file.syncStrings(GAME_NAMES(), 3);
	delete[] _gameNames;
	_gameNames = NULL;
	file.syncString(THE_PARTY_NEEDS_REST());
	file.syncString(WHO_WILL());
	file.syncString(HOW_MUCH());
	file.syncString(WHATS_THE_PASSWORD());
	file.syncString(PASSWORD_INCORRECT());
	file.syncString(IN_NO_CONDITION());
	file.syncString(NOTHING_HERE());
	file.syncStrings(TERRAIN_TYPES, 6);
	file.syncStrings(OUTDOORS_WALL_TYPES, 16);
	file.syncStrings(SURFACE_NAMES, 16);
	file.syncStrings(WHO_ACTIONS(), 32);
	delete[] _whoActions;
	_whoActions = NULL;
	file.syncStrings(WHO_WILL_ACTIONS(), 4);
	delete[] _whoWillActions;
	_whoWillActions = NULL;
	file.syncBytes2D((const byte *)SYMBOLS, 20, 64);
	file.syncBytes2D((const byte *)TEXT_COLORS, 40, 4);
	file.syncBytes2D((const byte *)TEXT_COLORS_STARTUP, 40, 4);
	file.syncStrings(DIRECTION_TEXT_UPPER(), 4);
	delete[] _directionTextUpper;
	_directionTextUpper = NULL;
	file.syncStrings(DIRECTION_TEXT(), 4);
	delete[] _directionText;
	_directionText = NULL;
	file.syncStrings(RACE_NAMES(), 5);
	delete[] _raceNames;
	_raceNames = NULL;
	file.syncNumbers(RACE_HP_BONUSES, 5);
	file.syncNumbers2D((const int *)RACE_SP_BONUSES, 5, 2);
	file.syncStrings(CLASS_NAMES(), 11);
	delete[] _classNames;
	_classNames = NULL;
	file.syncNumbers(CLASS_EXP_LEVELS, 10);
	file.syncStrings(ALIGNMENT_NAMES(), 3);
	delete[] _alignmentNames;
	_alignmentNames = NULL;
	file.syncStrings(SEX_NAMES(), 2);
	delete[] _sexNames;
	_sexNames = NULL;
	file.syncStrings(SKILL_NAMES(), 18);
	delete[] _skillNames;
	_skillNames = NULL;
	file.syncStrings(CONDITION_NAMES_M(), 17);
	delete[] _conditionNamesM;
	_conditionNamesM = NULL;
	file.syncStrings(CONDITION_NAMES_F(), 17);
	delete[] _conditionNamesF;
	_conditionNamesF = NULL;
	file.syncNumbers(CONDITION_COLORS, 17);
	file.syncString(GOOD());
	file.syncString(BLESSED());
	file.syncString(POWER_SHIELD());
	file.syncString(HOLY_BONUS());
	file.syncString(HEROISM());
	file.syncString(IN_PARTY());
	file.syncString(PARTY_DETAILS());
	file.syncString(PARTY_DIALOG_TEXT());
	file.syncNumbers(FACE_CONDITION_FRAMES, 17);
	file.syncNumbers(CHAR_FACES_X, 6);
	file.syncNumbers(HP_BARS_X, 6);
	file.syncString(NO_ONE_TO_ADVENTURE_WITH());
	file.syncBytes2D((const byte *)DARKNESS_XLAT, 3, 256);
	file.syncString(YOUR_ROSTER_IS_FULL());
	file.syncString(PLEASE_WAIT());
	file.syncString(OOPS());
	file.syncNumbers2D((const int *)SCREEN_POSITIONING_X, 4, 48);
	file.syncNumbers2D((const int *)SCREEN_POSITIONING_Y, 4, 48);
	file.syncNumbers(MONSTER_GRID_BITMASK, 12);
	file.syncNumbers2D((const int *)INDOOR_OBJECT_X, 2, 12);
	file.syncNumbers2D((const int *)MAP_OBJECT_Y, 2, 12);
	file.syncNumbers(INDOOR_MONSTERS_Y, 4);
	file.syncNumbers2D((const int *)OUTDOOR_OBJECT_X, 2, 12);
	file.syncNumbers(OUTDOOR_MONSTER_INDEXES, 26);
	file.syncNumbers(OUTDOOR_MONSTERS_Y, 26);
	file.syncNumbers2D((const int *)DIRECTION_ANIM_POSITIONS, 4, 4);
	file.syncBytes2D((const byte *)WALL_SHIFTS, 4, 48);
	file.syncNumbers(DRAW_NUMBERS, 25);
	file.syncNumbers2D((const int *)DRAW_FRAMES, 25, 2);
	file.syncNumbers(COMBAT_FLOAT_X, 8);
	file.syncNumbers(COMBAT_FLOAT_Y, 8);
	file.syncNumbers2D((const int *)MONSTER_EFFECT_FLAGS, 15, 8);
	file.syncNumbers2D((const int *)SPELLS_ALLOWED, 3, 40);
	file.syncNumbers(BASE_HP_BY_CLASS, 10);
	file.syncNumbers(AGE_RANGES, 10);
	file.syncNumbers2D((const int *)AGE_RANGES_ADJUST, 2, 10);
	file.syncNumbers(STAT_VALUES, 24);
	file.syncNumbers(STAT_BONUSES, 24);
	file.syncNumbers(ELEMENTAL_CATEGORIES, 6);
	file.syncNumbers(ATTRIBUTE_CATEGORIES, 10);
	file.syncNumbers(ATTRIBUTE_BONUSES, 72);
	file.syncNumbers(ELEMENTAL_RESISTENCES, 37);
	file.syncNumbers(ELEMENTAL_DAMAGE, 37);
	file.syncNumbers(WEAPON_DAMAGE_BASE, 35);
	file.syncNumbers(WEAPON_DAMAGE_MULTIPLIER, 35);
	file.syncNumbers(METAL_DAMAGE, 22);
	file.syncNumbers(METAL_DAMAGE_PERCENT, 22);
	file.syncNumbers(METAL_LAC, 22);
	file.syncNumbers(ARMOR_STRENGTHS, 14);
	file.syncNumbers(MAKE_ITEM_ARR1, 6);
	file.syncNumbers3D((const int *)MAKE_ITEM_ARR2, 6, 7, 2);
	file.syncNumbers3D((const int *)MAKE_ITEM_ARR3, 10, 7, 2);
	file.syncNumbers3D((const int *)MAKE_ITEM_ARR4, 2, 7, 2);
	file.syncNumbers2D((const int *)MAKE_ITEM_ARR5, 8, 2);
	file.syncNumbers(OUTDOOR_DRAWSTRUCT_INDEXES, 44);
	file.syncNumbers2D((const int *)TOWN_MAXES, 2, 11);
	file.syncStrings2D((const char *const *)TOWN_ACTION_MUSIC, 2, 7);
	file.syncStrings(TOWN_ACTION_SHAPES, 7);
	file.syncNumbers2D((const int *)TOWN_ACTION_FILES, 2, 7);
	file.syncString(BANK_TEXT());
	file.syncString(BLACKSMITH_TEXT());
	file.syncString(GUILD_NOT_MEMBER_TEXT());
	file.syncString(GUILD_TEXT());
	file.syncString(TAVERN_TEXT());
	file.syncString(GOOD_STUFF());
	file.syncString(HAVE_A_DRINK());
	file.syncString(YOURE_DRUNK());
	file.syncNumbers4D((const int *)TAVERN_EXIT_LIST, 2, 6, 5, 2);
	file.syncString(FOOD_AND_DRINK());
	file.syncString(TEMPLE_TEXT());
	file.syncString(EXPERIENCE_FOR_LEVEL());
	file.syncString(TRAINING_LEARNED_ALL());
	file.syncString(ELIGIBLE_FOR_LEVEL());
	file.syncString(TRAINING_TEXT());
	file.syncString(GOLD_GEMS());
	file.syncString(GOLD_GEMS_2());
	file.syncStrings(DEPOSIT_WITHDRAWL(), 2);
	delete[] _depositWithdrawl;
	_depositWithdrawl = NULL;
	file.syncString(NOT_ENOUGH_X_IN_THE_Y());
	file.syncString(NO_X_IN_THE_Y());
	file.syncStrings(STAT_NAMES(), 16);
	delete[] _statNames;
	_statNames = NULL;
	file.syncStrings(CONSUMABLE_NAMES(), 4);
	delete[] _consumableNames;
	_consumableNames = NULL;
	file.syncStrings(CONSUMABLE_GOLD_FORMS(), 1);
	delete[] _consumableGoldForms;
	_consumableGoldForms = NULL;
	file.syncStrings(CONSUMABLE_GEM_FORMS(), 1);
	delete[] _consumableGemForms;
	_consumableGemForms = NULL;
	file.syncStrings(WHERE_NAMES(), 2);
	delete[] _whereNames;
	_whereNames = NULL;
	file.syncString(AMOUNT());
	file.syncString(FOOD_PACKS_FULL());
	file.syncString(BUY_SPELLS());
	file.syncString(GUILD_OPTIONS());
	file.syncNumbers((const int *)MISC_SPELL_INDEX, 74);
	file.syncNumbers((const int *)SPELL_COSTS, 77);
	file.syncNumbers2D((const int *)CLOUDS_GUILD_SPELLS, 5, 20);
	file.syncNumbers2D((const int *)DARK_SPELL_OFFSETS, 3, 39);
	file.syncNumbers2D((const int *)DARK_SPELL_RANGES, 12, 2);
	file.syncNumbers2D((const int *)SWORDS_SPELL_RANGES, 12, 2);
	file.syncNumbers((const int *)SPELL_GEM_COST, 77);
	file.syncString(NOT_A_SPELL_CASTER());
	file.syncString(SPELLS_LEARNED_ALL());
	file.syncString(SPELLS_FOR());
	file.syncString(SPELL_LINES_0_TO_9());
	file.syncString(SPELLS_DIALOG_SPELLS());
	file.syncString(SPELL_PTS());
	file.syncString(GOLD());
	file.syncString(SPELL_INFO());
	file.syncString(SPELL_PURCHASE());
	file.syncString(MAP_TEXT());
	file.syncString(LIGHT_COUNT_TEXT());
	file.syncString(FIRE_RESISTENCE_TEXT());
	file.syncString(ELECRICITY_RESISTENCE_TEXT());
	file.syncString(COLD_RESISTENCE_TEXT());
	file.syncString(POISON_RESISTENCE_TEXT());
	file.syncString(CLAIRVOYANCE_TEXT());
	file.syncString(LEVITATE_TEXT());
	file.syncString(WALK_ON_WATER_TEXT());
	file.syncString(GAME_INFORMATION());
	file.syncString(WORLD_GAME_TEXT());
	file.syncString(DARKSIDE_GAME_TEXT());
	file.syncString(CLOUDS_GAME_TEXT());
	file.syncString(SWORDS_GAME_TEXT());
	file.syncStrings(WEEK_DAY_STRINGS(), 10);
	delete[] _weekDayStrings;
	_weekDayStrings = NULL;
	file.syncString(CHARACTER_DETAILS());
	file.syncStrings(DAYS(), 3);
	file.syncString(PARTY_GOLD());
	file.syncString(PLUS_14());
	file.syncString(CHARACTER_TEMPLATE());
	file.syncString(EXCHANGING_IN_COMBAT());
	file.syncString(CURRENT_MAXIMUM_RATING_TEXT());
	file.syncString(CURRENT_MAXIMUM_TEXT());
	file.syncStrings(RATING_TEXT(), 24);
	delete[] _ratingText;
	_ratingText = NULL;
	file.syncStrings(BORN(), 2);
	delete[] _born;
	_born = NULL;
	file.syncString(AGE_TEXT());
	file.syncString(LEVEL_TEXT());
	file.syncString(RESISTENCES_TEXT());
	file.syncString(NONE());
	file.syncString(EXPERIENCE_TEXT());
	file.syncString(ELIGIBLE());
	file.syncString(IN_PARTY_IN_BANK());
	file.syncStrings(FOOD_ON_HAND(), 3);
	delete[] _onHand;
	_onHand = NULL;
	file.syncString(FOOD_TEXT());
	file.syncString(EXCHANGE_WITH_WHOM());
	file.syncString(QUICK_REF_LINE());
	file.syncString(QUICK_REFERENCE());
	file.syncNumbers2D((const int *)BLACKSMITH_MAP_IDS, 2, 4);
	file.syncString(ITEMS_DIALOG_TEXT1());
	file.syncString(ITEMS_DIALOG_TEXT2());
	file.syncString(ITEMS_DIALOG_LINE1());
	file.syncString(ITEMS_DIALOG_LINE2());
	file.syncString(BTN_BUY());
	file.syncString(BTN_SELL());
	file.syncString(BTN_IDENTIFY());
	file.syncString(BTN_FIX());
	file.syncString(BTN_USE());
	file.syncString(BTN_EQUIP());
	file.syncString(BTN_REMOVE());
	file.syncString(BTN_DISCARD());
	file.syncString(BTN_QUEST());
	file.syncString(BTN_ENCHANT());
	file.syncString(BTN_RECHARGE());
	file.syncString(BTN_GOLD());
	file.syncString(ITEM_BROKEN());
	file.syncString(ITEM_CURSED());
	file.syncString(ITEM_OF());
	file.syncStrings(BONUS_NAMES(), 7);
	delete[] _bonusNames;
	_bonusNames = NULL;
	file.syncStrings(WEAPON_NAMES(), 41);
	delete[] _weaponNames;
	_weaponNames = NULL;
	file.syncStrings(ARMOR_NAMES(), 14);
	delete[] _armorNames;
	_armorNames = NULL;
	file.syncStrings(ACCESSORY_NAMES(), 11);
	delete[] _accessoryNames;
	_accessoryNames = NULL;
	file.syncStrings(MISC_NAMES(), 22);
	delete[] _miscNames;
	_miscNames = NULL;
	file.syncStrings(SPECIAL_NAMES(), 74);
	delete[] _specialNames;
	_specialNames = NULL;
	file.syncStrings(ELEMENTAL_NAMES(), 6);
	delete[] _elementalNames;
	_elementalNames = NULL;
	file.syncStrings(ATTRIBUTE_NAMES(), 10);
	delete[] _attributeNames;
	_attributeNames = NULL;
	file.syncStrings(EFFECTIVENESS_NAMES(), 7);
	delete[] _effectivenessNames;
	_effectivenessNames = NULL;
	file.syncStrings(QUEST_ITEM_NAMES(), 85);
	delete[] _questItemNames;
	_questItemNames = NULL;
	file.syncStrings(QUEST_ITEM_NAMES_SWORDS(), 51);
	delete[] _questItemNamesSwords;
	_questItemNamesSwords = NULL;
	file.syncNumbers((const int *)WEAPON_BASE_COSTS, 35);
	file.syncNumbers((const int *)ARMOR_BASE_COSTS, 14);
	file.syncNumbers((const int *)ACCESSORY_BASE_COSTS, 11);
	file.syncNumbers((const int *)MISC_MATERIAL_COSTS, 22);
	file.syncNumbers((const int *)MISC_BASE_COSTS, 76);
	file.syncNumbers((const int *)METAL_BASE_MULTIPLIERS, 22);
	file.syncNumbers((const int *)ITEM_SKILL_DIVISORS, 4);
	file.syncNumbers((const int *)RESTRICTION_OFFSETS, 4);
	file.syncNumbers((const int *)ITEM_RESTRICTIONS, 86);
	file.syncString(NOT_PROFICIENT());
	file.syncString(NO_ITEMS_AVAILABLE());
	file.syncStrings(CATEGORY_NAMES(), 4);
	delete[] _categoryNames;
	_categoryNames = NULL;
	file.syncString(X_FOR_THE_Y());
	file.syncString(X_FOR_Y());
	file.syncString(X_FOR_Y_GOLD());
	file.syncString(FMT_CHARGES());
	file.syncString(AVAILABLE_GOLD_COST());
	file.syncString(CHARGES());
	file.syncString(COST());
	file.syncStrings(ITEM_ACTIONS(), 7);
	delete[] _itemActions;
	_itemActions = NULL;
	file.syncString(WHICH_ITEM());
	file.syncString(WHATS_YOUR_HURRY());
	file.syncString(USE_ITEM_IN_COMBAT());
	file.syncString(NO_SPECIAL_ABILITIES());
	file.syncString(CANT_CAST_WHILE_ENGAGED());
	file.syncString(EQUIPPED_ALL_YOU_CAN());
	file.syncString(REMOVE_X_TO_EQUIP_Y());
	file.syncString(RING());
	file.syncString(MEDAL());
	file.syncString(CANNOT_REMOVE_CURSED_ITEM());
	file.syncString(CANNOT_DISCARD_CURSED_ITEM());
	file.syncString(PERMANENTLY_DISCARD());
	file.syncString(BACKPACK_IS_FULL());
	file.syncStrings(CATEGORY_BACKPACK_IS_FULL(), 4);
	file.syncString(BUY_X_FOR_Y_GOLD());
	file.syncString(SELL_X_FOR_Y_GOLD());
	file.syncStrings(GOLDS(), 2);
	delete[] _sellXForYGoldEndings;
	_sellXForYGoldEndings = NULL;
	file.syncString(NO_NEED_OF_THIS());
	file.syncString(NOT_RECHARGABLE());
	file.syncString(SPELL_FAILED());
	file.syncString(NOT_ENCHANTABLE());
	file.syncString(ITEM_NOT_BROKEN());
	file.syncStrings(FIX_IDENTIFY(), 2);
	delete[] _fixIdentify;
	_fixIdentify = NULL;
	file.syncString(FIX_IDENTIFY_GOLD());
	file.syncString(IDENTIFY_ITEM_MSG());
	file.syncString(ITEM_DETAILS());
	file.syncString(ALL());
	file.syncString(FIELD_NONE());
	file.syncString(DAMAGE_X_TO_Y());
	file.syncString(ELEMENTAL_XY_DAMAGE());
	file.syncString(ATTR_XY_BONUS());
	file.syncString(EFFECTIVE_AGAINST());
	file.syncString(QUESTS_DIALOG_TEXT());
	file.syncString(CLOUDS_OF_XEEN_LINE());
	file.syncString(DARKSIDE_OF_XEEN_LINE());
	file.syncString(SWORDS_OF_XEEN_LINE());
	file.syncString(NO_QUEST_ITEMS());
	file.syncString(NO_CURRENT_QUESTS());
	file.syncString(NO_AUTO_NOTES());
	file.syncString(QUEST_ITEMS_DATA());
	file.syncString(CURRENT_QUESTS_DATA());
	file.syncString(AUTO_NOTES_DATA());
	file.syncString(REST_COMPLETE());
	file.syncString(PARTY_IS_STARVING());
	file.syncString(HIT_SPELL_POINTS_RESTORED());
	file.syncString(TOO_DANGEROUS_TO_REST());
	file.syncString(SOME_CHARS_MAY_DIE());
	file.syncString(DISMISS_WHOM());
	file.syncString(CANT_DISMISS_LAST_CHAR());
	file.syncString(DELETE_CHAR_WITH_ELDER_WEAPON());
	file.syncStrings(REMOVE_DELETE(), 2);
	delete[] _removeDelete;
	_removeDelete = NULL;
	file.syncString(REMOVE_OR_DELETE_WHICH());
	file.syncString(YOUR_PARTY_IS_FULL());
	file.syncString(HAS_SLAYER_SWORD());
	file.syncString(SURE_TO_DELETE_CHAR());
	file.syncString(CREATE_CHAR_DETAILS());
	file.syncString(NEW_CHAR_STATS());
	file.syncString(NAME_FOR_NEW_CHARACTER());
	file.syncString(SELECT_CLASS_BEFORE_SAVING());
	file.syncString(EXCHANGE_ATTR_WITH());
	file.syncNumbers((const int *)NEW_CHAR_SKILLS, 10);
	file.syncNumbers((const int *)NEW_CHAR_SKILLS_OFFSET(), 10);
	file.syncNumbers((const int *)NEW_CHAR_SKILLS_LEN, 10);
	file.syncNumbers((const int *)NEW_CHAR_RACE_SKILLS, 10);
	file.syncNumbers((const int *)RACE_MAGIC_RESISTENCES, 5);
	file.syncNumbers((const int *)RACE_FIRE_RESISTENCES, 5);
	file.syncNumbers((const int *)RACE_ELECTRIC_RESISTENCES, 5);
	file.syncNumbers((const int *)RACE_COLD_RESISTENCES, 5);
	file.syncNumbers((const int *)RACE_ENERGY_RESISTENCES, 5);
	file.syncNumbers((const int *)RACE_POISON_RESISTENCES, 5);
	file.syncNumbers2D((const int *)NEW_CHARACTER_SPELLS, 10, 4);
	file.syncString(COMBAT_DETAILS());
	file.syncString(NOT_ENOUGH_TO_CAST());
	file.syncStrings(SPELL_CAST_COMPONENTS(), 2);
	delete[] _spellCastComponents;
	_spellCastComponents = NULL;
	file.syncString(CAST_SPELL_DETAILS());
	file.syncString(PARTY_FOUND());
	file.syncString(BACKPACKS_FULL_PRESS_KEY());
	file.syncString(HIT_A_KEY());
	file.syncString(GIVE_TREASURE_FORMATTING());
	file.syncStrings(FOUND(), 2);
	delete[] _found;
	_found = NULL;
	file.syncString(X_FOUND_Y());
	file.syncString(ON_WHO());
	file.syncString(WHICH_ELEMENT1());
	file.syncString(WHICH_ELEMENT2());
	file.syncString(DETECT_MONSTERS());
	file.syncString(LLOYDS_BEACON());
	file.syncString(HOW_MANY_SQUARES());
	file.syncString(TOWN_PORTAL());
	file.syncString(TOWN_PORTAL_SWORDS());
	file.syncNumbers2D((const int *)TOWN_MAP_NUMBERS, 3, 5);
	file.syncString(MONSTER_DETAILS());
	file.syncStrings(MONSTER_SPECIAL_ATTACKS(), 23);
	delete[] _monsterSpecialAttacks;
	_monsterSpecialAttacks = NULL;
	file.syncString(IDENTIFY_MONSTERS());
	file.syncStrings(EVENT_SAMPLES, 6);
	file.syncString(MOONS_NOT_ALIGNED());
	file.syncString(AWARDS_FOR());
	file.syncString(AWARDS_TEXT());
	file.syncString(NO_AWARDS());
	file.syncString(WARZONE_BATTLE_MASTER());
	file.syncString(WARZONE_MAXED());
	file.syncString(WARZONE_LEVEL());
	file.syncString(WARZONE_HOW_MANY());
	file.syncString(PICKS_THE_LOCK());
	file.syncStrings(PICK_FORM(), 2);
	delete[] _pickForm;
	_pickForm = NULL;
	file.syncString(UNABLE_TO_PICK_LOCK());
	file.syncStrings(UNABLE_TO_PICK_FORM(), 2);
	delete[] _unableToPickForm;
	_unableToPickForm = NULL;
	file.syncString(CONTROL_PANEL_TEXT());
	file.syncString(CONTROL_PANEL_BUTTONS());
	file.syncString(ON());
	file.syncString(OFF());
	file.syncString(CONFIRM_QUIT());
	file.syncString(MR_WIZARD());
	file.syncString(NO_LOADING_IN_COMBAT());
	file.syncString(NO_SAVING_IN_COMBAT());
	file.syncString(QUICK_FIGHT_TEXT());
	file.syncStrings(QUICK_FIGHT_OPTIONS(), 4);
	delete[] _quickFightOptions;
	_quickFightOptions = NULL;
	file.syncStrings(WORLD_END_TEXT(), 9);
	delete[] _worldEndText;
	_worldEndText = NULL;
	file.syncString(WORLD_CONGRATULATIONS());
	file.syncString(WORLD_CONGRATULATIONS2());
	file.syncString(CLOUDS_CONGRATULATIONS1());
	file.syncString(CLOUDS_CONGRATULATIONS2());
	file.syncStrings(GOOBER(), 3);
	delete[] _goober;
	_goober = NULL;
	file.syncStrings(MUSIC_FILES1, 5);
	file.syncStrings2D((const char *const *)MUSIC_FILES2, 6, 7);
	file.syncString(DIFFICULTY_TEXT());
	file.syncString(SAVE_OFF_LIMITS());
	file.syncString(CLOUDS_INTRO1());
	file.syncString(DARKSIDE_ENDING1());
	file.syncString(DARKSIDE_ENDING2());
	file.syncString(PHAROAH_ENDING_TEXT1());
	file.syncString(PHAROAH_ENDING_TEXT2());

	cc.add("CONSTANTS" + num, file);

	Common::MemFile keys;
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CHAR_INFO()->KEY_ITEM());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CHAR_INFO()->KEY_QUICK());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CHAR_INFO()->KEY_EXCHANGE());

	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CONTROL_PANEL()->KEY_FXON());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CONTROL_PANEL()->KEY_MUSICON());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CONTROL_PANEL()->KEY_LOAD());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CONTROL_PANEL()->KEY_SAVE());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CONTROL_PANEL()->KEY_QUIT());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CONTROL_PANEL()->KEY_MRWIZARD());

	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_ROLL());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_CREATE());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_MGT());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_INT());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_PER());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_END());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_SPD());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_ACY());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_CREATE_CHAR()->KEY_LCK());

	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_DIFFICULTY()->KEY_ADVENTURER());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_DIFFICULTY()->KEY_WARRIOR());

	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_WEAPONS());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_ARMOR());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_ACCESSORY());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_MISC());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_ENCHANT());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_USE());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_BUY());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_SELL());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_IDENTIFY());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_FIX());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_EQUIP());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_REM());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_DISC());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_QUEST());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_RECHRG());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_ITEMS()->KEY_GOLD());

	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_PARTY()->KEY_DELETE());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_PARTY()->KEY_REMOVE());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_PARTY()->KEY_CREATE());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_PARTY()->KEY_EXIT());

	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_QUESTS()->KEY_QUEST_ITEMS());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_QUESTS()->KEY_CURRENT_QUESTS());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_QUESTS()->KEY_QUEST_ITEMS());

	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_QUICK_FIGHT()->KEY_NEXT());

	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_SPELLS()->KEY_CAST());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_SPELLS()->KEY_NEW());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_SPELLS()->KEY_FIRE());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_SPELLS()->KEY_ELEC());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_SPELLS()->KEY_COLD());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_SPELLS()->KEY_ACID());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_SPELLS()->KEY_SET());
	keys.syncNumber(KEY_CONSTANTS()->DIALOGS_SPELLS()->KEY_RETURN());

	keys.syncNumber(KEY_CONSTANTS()->LOCATIONS()->KEY_DEP());
	keys.syncNumber(KEY_CONSTANTS()->LOCATIONS()->KEY_WITH());
	keys.syncNumber(KEY_CONSTANTS()->LOCATIONS()->KEY_GOLD());
	keys.syncNumber(KEY_CONSTANTS()->LOCATIONS()->KEY_GEMS());

	cc.add("CONSTKEYS" + num, keys);
}

void writeConstants(CCArchive &cc) {
	EN eng;
	eng.writeConstants(Common::String::format("_%i", Common::Language::EN_ANY), cc);
	RU ru;
	ru.writeConstants(Common::String::format("_%i", Common::Language::RU_RUS), cc);
}
