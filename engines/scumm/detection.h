/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SCUMM_DETECTION_H
#define SCUMM_DETECTION_H

#include "common/language.h"
#include "common/platform.h"

namespace Scumm {


// GUI-options, primarily used by detection_tables.h
#define GUIO_TRIM_FMTOWNS_TO_200_PIXELS                GUIO_GAMEOPTIONS1
#define GUIO_ENHANCEMENTS                              GUIO_GAMEOPTIONS2
#define GUIO_AUDIO_OVERRIDE                            GUIO_GAMEOPTIONS3
#define GUIO_ORIGINALGUI                               GUIO_GAMEOPTIONS4

/**
 * Descriptor of a specific SCUMM game. Used internally to store
 * information about the tons of game variants that exist.
 */
struct GameSettings {
	/**
	 * The gameid of this game.
	 */
	const char *gameid;

	/**
	 * An identifier which can be used to distinguish game variants.
	 * This string is also used to augment the description string
	 * generated by the detector, and to search the gameFilenamesTable.
	 * It is also used to search the MD5 table (it matches the "extra"
	 * data in scumm-md5.txt).
	 *
	 * Equal to 0 (zero) *if and only if* the game has precisely one
	 * variant. Failing to obey this rule can lead to odd bugs.
	 */
	const char *variant;

	/**
	 * An optional string that will be added to the 'preferredtarget'
	 * computed by the detector.
	 */
	const char *preferredTag;

	/**
	 * The numerical gameid of this game.
	 * This is not in one-to-one correspondence with the gameid above.
	 * But if two games settings have the same id (except for GID_HEGAME),
	 * then they also have the same gameid ; the converse does not hold
	 * in general.
	 */
	byte id;

	/** The SCUMM version. */
	byte version;

	/** The HE subversion. */
	byte heversion;

	/** MidiDriverFlags values */
	int midi;

	/**
	 * Bitmask obtained by ORing various GameFeatures enums, and used
	 * to en-/disable certain features of this game variant.
	 */
	uint32 features;

	/**
	 * Platform indicator, this is set to a value different from
	 * kPlatformUnknown if this game variant only existed for this
	 * specific platform.
	 */
	Common::Platform platform;

	/**
	 * Game GUI options. Used to enable/disable certain GUI widgets
	 */
	const char *guioptions;
};

enum FilenameGenMethod {
	kGenDiskNum,
	kGenDiskNumSteam,
	kGenRoomNum,
	kGenRoomNumSteam,
	kGenHEMac,
	kGenHEMacNoParens,
	kGenHEPC,
	kGenHEIOS,
	kGenUnchanged
};

struct FilenamePattern {
	const char *pattern;
	FilenameGenMethod genMethod;
};

struct GameFilenamePattern {
	const char *gameid;
	const char *pattern;
	FilenameGenMethod genMethod;
	Common::Language language;
	Common::Platform platform;
	const char *variant;
};

struct DetectorResult {
	FilenamePattern fp;
	GameSettings game;
	Common::Language language;
	Common::String md5;
	const char *extra;
};

/**
 * SCUMM feature flags define for every game which specific set of engine
 * features are used by that game.
 * Note that some of them could be replaced by checks for the SCUMM version.
 */
enum GameFeatures {
	/** A demo, not a full blown game. */
	GF_DEMO = 1 << 0,

	/** Games with the AKOS costume system (ScummEngine_v7 and subclasses, HE games). */
	GF_NEW_COSTUMES = 1 << 2,

	/** Games using XOR encrypted data files. */
	GF_USE_KEY = 1 << 4,

	/** Small header games (ScummEngine_v4 and subclasses). */
	GF_SMALL_HEADER = 1 << 5,

	/** Old bundle games (ScummEngine_v3old and subclasses). */
	GF_OLD_BUNDLE = 1 << 6,

	/** EGA games. */
	GF_16COLOR = 1 << 7,

	/** VGA versions of V3 games.  Equivalent to (version == 3 && not GF_16COLOR) */
	GF_OLD256 = 1 << 8,

	/** Games which have Audio CD tracks. */
	GF_AUDIOTRACKS = 1 << 9,

	/**
	 * Games using only very few local variables in scripts.
	 * Apparently that is only the case for 256 color version of Indy3.
	 */
	GF_FEW_LOCALS = 1 << 11,

	/** HE games for which localized versions exist */
	GF_HE_LOCALIZED = 1 << 13,

	/**
	 *  HE games with more global scripts and different sprite handling
	 *  i.e. read it as HE version 9.85. Used for HE98 only.
	 */
	GF_HE_985 = 1 << 14,

	/** HE games with 16 bit color */
	GF_16BIT_COLOR = 1 << 15,

	/**
	 * SCUMM v5-v7 Mac games stored in a container file
	 * Used to differentiate between m68k and PPC versions of Indy4
	 */
	GF_MAC_CONTAINER = 1 << 16,

	/**
	 * SCUMM HE Official Hebrew translations were audio only
	 * but used reversed string for credits etc.
	 * Used to disable BiDi in those games.
	 */
	GF_HE_NO_BIDI = 1 << 17
};

enum ScummGameId {
	GID_CMI,
	GID_DIG,
	GID_FT,
	GID_INDY3,
	GID_INDY4,
	GID_LOOM,
	GID_MANIAC,
	GID_MONKEY_EGA,
	GID_MONKEY_VGA,
	GID_MONKEY,
	GID_MONKEY2,
	GID_PASS,
	GID_SAMNMAX,
	GID_TENTACLE,
	GID_ZAK,

	GID_HEGAME,      // Generic name for all HE games with default behavior
	GID_PUTTDEMO,
	GID_FBEAR,
	GID_PUTTMOON,
	GID_FUNPACK,
	GID_PUTTZOO,
	GID_FREDDI,
	GID_FREDDI3,
	GID_FREDDI4,
	GID_BIRTHDAYRED,
	GID_BIRTHDAYYELLOW,
	GID_TREASUREHUNT,
	GID_PUTTRACE,
	GID_FUNSHOP,	// Used for all three funshops
	GID_FOOTBALL,
	GID_FOOTBALL2002,
	GID_SOCCER,
	GID_SOCCERMLS,
	GID_SOCCER2004,
	GID_BASEBALL2001,
	GID_BASEBALL2003,
	GID_BASKETBALL,
	GID_MOONBASE,
	GID_PJGAMES,
	GID_HECUP		// CUP demos
};

} // End of namespace Scumm


#endif
