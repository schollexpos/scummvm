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
 */

#include "gnap/gnap.h"
#include "gnap/gamesys.h"
#include "gnap/resource.h"
#include "gnap/scenes/scene05.h"

namespace Gnap {

enum {
	kHSPlatypus		= 0,
	kHSHaystack		= 1,
	kHSPadlock		= 2,
	kHSLadder		= 3,
	kHSExitHouse	= 4,
	kHSChicken		= 5,
	kHSDevice		= 6,
	kHSWalkArea1	= 7,
	kHSWalkArea2	= 8,
	kHSWalkArea3	= 9
};

enum {
	kASPlatSearchHaystack		= 0,
	kASTryPickPadlock			= 1,
	kASPickPadlock				= 2,
	kASTalkChicken				= 3,
	kASGrabChicken				= 4,
	kASGrabLadder				= 5,
	kASEnterBarn				= 6,
	kASUseTwigWithChicken		= 11,
	kASLeaveScene				= 12
};

Scene05::Scene05(GnapEngine *vm) : Scene(vm) {
	_s05_nextChickenSequenceId = -1;
	_s05_currChickenSequenceId = -1;
}

int Scene05::init() {
	_vm->_gameSys->setAnimation(0, 0, 0);
	_vm->_gameSys->setAnimation(0, 0, 1);
	_vm->_gameSys->setAnimation(0, 0, 3);
	return _vm->isFlag(kGFBarnPadlockOpen) ? 0x151 : 0x150;
}

void Scene05::updateHotspots() {
	_vm->setHotspot(kHSPlatypus, 0, 0, 0, 0, SF_DISABLED | SF_WALKABLE | SF_TALK_CURSOR | SF_GRAB_CURSOR | SF_LOOK_CURSOR);
	_vm->setHotspot(kHSHaystack, 236, 366, 372, 442, SF_PLAT_CURSOR | SF_TALK_CURSOR | SF_GRAB_CURSOR | SF_LOOK_CURSOR, 5, 7);
	_vm->setHotspot(kHSPadlock, 386, 230, 626, 481, SF_PLAT_CURSOR | SF_TALK_CURSOR | SF_GRAB_CURSOR | SF_LOOK_CURSOR, 5, 7);
	_vm->setHotspot(kHSLadder, 108, 222, 207, 444, SF_PLAT_CURSOR | SF_TALK_CURSOR | SF_GRAB_CURSOR | SF_LOOK_CURSOR, 4, 7);
	_vm->setHotspot(kHSExitHouse, 0, 395, 20, 600, SF_EXIT_L_CURSOR | SF_WALKABLE, 0, 8);
	_vm->setHotspot(kHSChicken, 612, 462, 722, 564, SF_PLAT_CURSOR | SF_TALK_CURSOR | SF_GRAB_CURSOR | SF_LOOK_CURSOR, 7, 8);
	_vm->setHotspot(kHSWalkArea1, 104, 0, 421, 480);
	_vm->setHotspot(kHSWalkArea2, 422, 0, 800, 487);
	_vm->setHotspot(kHSWalkArea3, 0, 0, 104, 499);
	_vm->setDeviceHotspot(kHSDevice, -1, -1, -1, -1);
	if (_vm->isFlag(kGFPlatypus))
		_vm->_hotspots[kHSPlatypus]._flags = SF_WALKABLE | SF_TALK_CURSOR | SF_GRAB_CURSOR | SF_LOOK_CURSOR;
	if (_vm->isFlag(kGFBarnPadlockOpen))
		_vm->_hotspots[kHSPadlock]._flags = SF_EXIT_U_CURSOR;
	_vm->_hotspotsCount = 10;
}

void Scene05::run() {
	_vm->playSound(0x1091C, true);
	_vm->startSoundTimerC(7);
	
	_s05_currChickenSequenceId = 0x142;
	_vm->_gameSys->setAnimation(0x142, 100, 3);
	_vm->_gameSys->insertSequence(0x142, 100, 0, 0, kSeqNone, 0, 0, 0);
	
	_s05_nextChickenSequenceId = -1;

	_vm->_timers[5] = _vm->getRandom(10) + 30;
	_vm->_timers[6] = _vm->getRandom(150) + 300;

	if (_vm->isFlag(kGFBarnPadlockOpen))
		_vm->_gameSys->insertSequence(0x14A, 141, 0, 0, kSeqNone, 0, 0, 0);
	
	_vm->queueInsertDeviceIcon();
	
	if (_vm->_prevSceneNum != 6 && _vm->_prevSceneNum != 36) {
		_vm->initGnapPos(-1, 8, kDirBottomRight);
		if (_vm->isFlag(kGFPlatypus))
			_vm->initPlatypusPos(-1, 9, kDirNone);
		_vm->endSceneInit();
		if (_vm->isFlag(kGFPlatypus))
			_vm->platypusWalkTo(2, 8, -1, 0x107C2, 1);
		_vm->gnapWalkTo(2, 9, -1, 0x107B9, 1);
	} else {
		_vm->initGnapPos(6, 8, kDirBottomRight);
		if (_vm->isFlag(kGFPlatypus))
			_vm->initPlatypusPos(7, 9, kDirNone);
		_vm->endSceneInit();
	}
	
	while (!_vm->_sceneDone) {
		_vm->updateMouseCursor();
		_vm->updateCursorByHotspot();
	
		_vm->testWalk(0, 12, -1, -1, -1, -1);
	
		_vm->_sceneClickedHotspot = _vm->getClickedHotspotId();
		_vm->updateGrabCursorSprite(0, 0);
	
		switch (_vm->_sceneClickedHotspot) {
		case kHSDevice:
			if (_vm->_gnapActionStatus < 0) {
				_vm->runMenu();
				updateHotspots();
			}
			break;

		case kHSPlatypus:
			if (_vm->_gnapActionStatus < 0 && _vm->isFlag(kGFPlatypus)) {
				if (_vm->_grabCursorSpriteIndex == kItemDisguise) {
					_vm->gnapUseDisguiseOnPlatypus();
				} else if (_vm->_grabCursorSpriteIndex >= 0) {
					_vm->playGnapImpossible(0, 0);
				} else {
					switch (_vm->_verbCursor) {
					case LOOK_CURSOR:
						if (_vm->isFlag(kGFKeysTaken))
							_vm->playGnapMoan1(_vm->_platX, _vm->_platY);
						else
							_vm->playGnapScratchingHead(_vm->_platX, _vm->_platY);
						break;
					case GRAB_CURSOR:
						_vm->gnapKissPlatypus(0);
						break;
					case TALK_CURSOR:
						_vm->playGnapBrainPulsating(_vm->_platX, _vm->_platY);
						_vm->playPlatypusSequence(_vm->getPlatypusSequenceId());
						break;
					case PLAT_CURSOR:
						_vm->playGnapImpossible(0, 0);
						break;
					}
				}
			}
			break;

		case kHSHaystack:
			if (_vm->_gnapActionStatus < 0 && _vm->_platypusActionStatus < 0) {
				if (_vm->_grabCursorSpriteIndex >= 0) {
					_vm->playGnapShowCurrItem(_vm->_hotspotsWalkPos[1].x - 2, _vm->_hotspotsWalkPos[1].y, 4, 5);
				} else if (_vm->isFlag(kGFNeedleTaken)) {
					_vm->playGnapImpossible(0, 0);
				} else {
					switch (_vm->_verbCursor) {
					case LOOK_CURSOR:
						_vm->playGnapScratchingHead(_vm->_hotspotsWalkPos[1].x, _vm->_hotspotsWalkPos[1].y - 1);
						break;
					case GRAB_CURSOR:
					case TALK_CURSOR:
						_vm->playGnapImpossible(0, 0);
						break;
					case PLAT_CURSOR:
						if (_vm->isFlag(kGFPlatypus)) {
							_vm->gnapUseDeviceOnPlatypuss();
							if (_vm->platypusWalkTo(_vm->_hotspotsWalkPos[1].x, _vm->_hotspotsWalkPos[1].y, 1, 0x107C2, 1)) {
								_vm->_platypusActionStatus = kASPlatSearchHaystack;
								_vm->_platypusFacing = kDirUnk4;
							}
							if (_vm->_gnapX == 4 && (_vm->_gnapY == 8 || _vm->_gnapY == 7))
								_vm->gnapWalkStep();
							_vm->playGnapIdle(_vm->_platX, _vm->_platY);
						}
						break;
					}
				}
			}
			break;

		case kHSChicken:
			if (_vm->_gnapActionStatus < 0) {
				if (_vm->_grabCursorSpriteIndex == kItemTwig) {
					_vm->_gnapIdleFacing = kDirUpRight;
					_vm->gnapWalkTo(_vm->_hotspotsWalkPos[_vm->_sceneClickedHotspot].x, _vm->_hotspotsWalkPos[_vm->_sceneClickedHotspot].y + 1,
						0, _vm->getGnapSequenceId(gskIdle, 0, 0) | 0x10000, 1);
					_vm->_gnapActionStatus = kASUseTwigWithChicken;
				} else if (_vm->_grabCursorSpriteIndex >= 0) {
					_vm->playGnapShowCurrItem(_vm->_hotspotsWalkPos[_vm->_sceneClickedHotspot].x, _vm->_hotspotsWalkPos[_vm->_sceneClickedHotspot].y + 1, 9, 7);
				} else {
					switch (_vm->_verbCursor) {
					case LOOK_CURSOR:
						_vm->playGnapMoan2(9, 7);
						break;
					case GRAB_CURSOR:
						_vm->_gnapIdleFacing = kDirBottomRight;
						_vm->gnapWalkTo(_vm->_hotspotsWalkPos[5].x, _vm->_hotspotsWalkPos[5].y, 0, _vm->getGnapSequenceId(gskIdle, 0, 0) | 0x10000, 1);
						_vm->_gnapActionStatus = kASGrabChicken;
						break;
					case TALK_CURSOR:
						_vm->_gnapIdleFacing = kDirBottomRight;
						_vm->gnapWalkTo(_vm->_hotspotsWalkPos[5].x, _vm->_hotspotsWalkPos[5].y, 0, _vm->getGnapSequenceId(gskBrainPulsating, 0, 0) | 0x10000, 1);
						_vm->_gnapActionStatus = kASTalkChicken;
						break;
					case PLAT_CURSOR:
						_vm->playGnapImpossible(0, 0);
						break;
					}
				}
			}
			break;

		case kHSLadder:
			if (_vm->_gnapActionStatus < 0) {
				if (_vm->_grabCursorSpriteIndex >= 0) {
					_vm->playGnapShowCurrItem(_vm->_hotspotsWalkPos[_vm->_sceneClickedHotspot].x, _vm->_hotspotsWalkPos[_vm->_sceneClickedHotspot].y, 2, 5);
				} else {
					switch (_vm->_verbCursor) {
					case LOOK_CURSOR:
						_vm->playGnapMoan2(2, 4);
						break;
					case GRAB_CURSOR:
						_vm->_gnapIdleFacing = kDirBottomLeft;
						_vm->gnapWalkTo(_vm->_hotspotsWalkPos[3].x, _vm->_hotspotsWalkPos[3].y, 0, _vm->getGnapSequenceId(gskIdle, 0, 0) | 0x10000, 1);
						_vm->_gnapActionStatus = kASGrabLadder;
						break;
					case TALK_CURSOR:
					case PLAT_CURSOR:
						_vm->playGnapImpossible(0, 0);
						break;
					}
				}
			}
			break;

		case kHSPadlock:
			if (_vm->isFlag(kGFBarnPadlockOpen)) {
				_vm->_isLeavingScene = true;
				_vm->gnapWalkTo(_vm->_hotspotsWalkPos[2].x - 1, _vm->_hotspotsWalkPos[2].y + 1, 0, -1, 1);
				_vm->_gnapActionStatus = kASEnterBarn;
				if (_vm->_cursorValue == 1)
					_vm->_newSceneNum = 6;
				else
					_vm->_newSceneNum = 36;
			} else if (_vm->_gnapActionStatus < 0) {
				if (_vm->_grabCursorSpriteIndex == kItemNeedle) {
					if (_vm->gnapWalkTo(_vm->_hotspotsWalkPos[2].x, _vm->_hotspotsWalkPos[2].y, 0,
						_vm->getGnapSequenceId(gskIdle, _vm->_hotspotsWalkPos[2].x, _vm->_hotspotsWalkPos[2].y) | 0x10000, 1))
						_vm->_gnapActionStatus = kASPickPadlock;
				} else if (_vm->_grabCursorSpriteIndex >= 0) {
					_vm->playGnapShowCurrItem(_vm->_hotspotsWalkPos[2].x, _vm->_hotspotsWalkPos[2].y, 7, 4);
				} else {
					switch (_vm->_verbCursor) {
					case LOOK_CURSOR:
						_vm->playGnapScratchingHead(7, 4);
						break;
					case GRAB_CURSOR:
						_vm->_gnapIdleFacing = kDirUpRight;
						_vm->gnapWalkTo(_vm->_hotspotsWalkPos[_vm->_sceneClickedHotspot].x, _vm->_hotspotsWalkPos[_vm->_sceneClickedHotspot].y,
							0, _vm->getGnapSequenceId(gskIdle, 0, 0) | 0x10000, 1);
						_vm->_gnapActionStatus = kASTryPickPadlock;
						break;
					case TALK_CURSOR:
					case PLAT_CURSOR:
						_vm->playGnapImpossible(0, 0);
						break;
					}
				}
			}
			break;

		case kHSExitHouse:
			if (_vm->_gnapActionStatus < 0) {
				_vm->_isLeavingScene = true;
				_vm->gnapWalkTo(_vm->_hotspotsWalkPos[4].x, _vm->_hotspotsWalkPos[4].y, 0, 0x107AF, 1);
				_vm->_gnapActionStatus = kASLeaveScene;
				if (_vm->isFlag(kGFPlatypus))
					_vm->platypusWalkTo(_vm->_hotspotsWalkPos[4].x, _vm->_hotspotsWalkPos[4].y + 1, -1, 0x107C7, 1);
				if (_vm->_cursorValue == 1)
					_vm->_newSceneNum = 4;
				else
					_vm->_newSceneNum = 37;
			}
			break;

		case kHSWalkArea1:
		case kHSWalkArea2:
			if (_vm->_gnapActionStatus < 0)
				_vm->gnapWalkTo(-1, -1, -1, -1, 1);
			break;
			
		case kHSWalkArea3:
			// Nothing
			break;

		default:
			if (_vm->_mouseClickState._left && _vm->_gnapActionStatus < 0) {
				_vm->gnapWalkTo(-1, -1, -1, -1, 1);
				_vm->_mouseClickState._left = false;
			}
			break;
		
		}
	
		updateAnimations();
	
		if (!_vm->isSoundPlaying(0x1091C))
			_vm->playSound(0x1091C, true);
	
		if (!_vm->_isLeavingScene) {
			if (_vm->isFlag(kGFPlatypus))
				_vm->updatePlatypusIdleSequence();
			_vm->updateGnapIdleSequence();
			if (!_vm->_timers[5]) {
				_vm->_timers[5] = _vm->getRandom(20) + 30;
				if (_vm->_gnapActionStatus != kASTalkChicken && _s05_nextChickenSequenceId == -1) {
					if (_vm->getRandom(4) != 0)
						_s05_nextChickenSequenceId = 0x142;
					else
						_s05_nextChickenSequenceId = 0x143;
				}
			}
			if (!_vm->_timers[6]) {
				_vm->_timers[6] = _vm->getRandom(150) + 300;
				if (_vm->_gnapActionStatus < 0)
					_vm->_gameSys->insertSequence(0x149, 39, 0, 0, kSeqNone, 0, 0, 0);
			}
			_vm->playSoundC();
		}
	
		_vm->checkGameKeys();
	
		if (_vm->isKeyStatus1(8)) {
			_vm->clearKeyStatus1(8);
			_vm->runMenu();
			updateHotspots();
			_vm->_timers[5] = _vm->getRandom(20) + 30;
		}
		
		_vm->gameUpdateTick();
	}
}

void Scene05::updateAnimations() {
	if (_vm->_gameSys->getAnimationStatus(0) == 2) {
		_vm->_gameSys->setAnimation(0, 0, 0);
		switch (_vm->_gnapActionStatus) {
		case kASLeaveScene:
			_vm->_sceneDone = true;
			_vm->_gnapActionStatus = -1;
			break;
		case kASTryPickPadlock:
			_vm->_gameSys->insertSequence(0x148, _vm->_gnapId, makeRid(_vm->_gnapSequenceDatNum, _vm->_gnapSequenceId), _vm->_gnapId, kSeqSyncWait, 0, 0, 0);
			_vm->_gnapSequenceId = 0x148;
			_vm->_gnapSequenceDatNum = 0;
			_vm->_gnapActionStatus = -1;
			break;
		case kASPickPadlock:
			_vm->_gameSys->setAnimation(0x147, _vm->_gnapId, 0);
			_vm->_gameSys->insertSequence(0x147, _vm->_gnapId, makeRid(_vm->_gnapSequenceDatNum, _vm->_gnapSequenceId), _vm->_gnapId, kSeqSyncWait, 0, 0, 0);
			_vm->_gnapSequenceId = 0x147;
			_vm->_gnapSequenceDatNum = 0;
			_vm->setFlag(kGFBarnPadlockOpen);
			_vm->setFlag(kGFSceneFlag1);
			_vm->setGrabCursorSprite(-1);
			_vm->_newSceneNum = 6;
			_vm->_timers[2] = 100;
			_vm->invRemove(kItemNeedle);
			_vm->_gnapActionStatus = kASLeaveScene;
			break;
		case kASTalkChicken:
			_s05_nextChickenSequenceId = 0x144;
			_vm->_gnapActionStatus = -1;
			break;
		case kASGrabChicken:
			_s05_nextChickenSequenceId = 0x14B;
			break;
		case kASGrabLadder:
			while (_vm->_gameSys->isSequenceActive(0x149, 39))
				_vm->gameUpdateTick();
			_vm->_gameSys->insertSequence(0x14E, _vm->_gnapId + 1, 0, 0, kSeqNone, 0, 0, 0);
			_vm->_gameSys->insertSequence(0x14D, _vm->_gnapId, makeRid(_vm->_gnapSequenceDatNum, _vm->_gnapSequenceId), _vm->_gnapId, kSeqSyncWait, 0, 0, 0);
			_vm->_gnapSequenceId = 0x14D;
			_vm->_gnapSequenceDatNum = 0;
			_vm->_timers[2] = 200;
			_vm->_timers[6] = 300;
			_vm->_gnapActionStatus = -1;
			break;
		case kASEnterBarn:
			_vm->_gameSys->insertSequence(0x107B1, 1,
				makeRid(_vm->_gnapSequenceDatNum, _vm->_gnapSequenceId), _vm->_gnapId,
				kSeqSyncWait, 0, 75 * _vm->_gnapX - _vm->_gnapGridX, 48 * _vm->_gnapY - _vm->_gnapGridY);
			_vm->_gameSys->setAnimation(0x107B1, 1, 0);
			_vm->_gnapActionStatus = kASLeaveScene;
			break;
		case kASUseTwigWithChicken:
			_vm->playGnapShowItem(5, 0, 0);
			_s05_nextChickenSequenceId = 0x14F;
			_vm->_gnapActionStatus = -1;
			break;
		}
	}
	
	if (_vm->_gameSys->getAnimationStatus(1) == 2) {
		if (_vm->_platypusSequenceId == 0x146) {
			_vm->_platX = 4;
			_vm->_platY = 8;
			_vm->_gameSys->insertSequence(0x107C1, 160, 0x146, 256, kSeqSyncWait, 0, 300 - _vm->_platGridX, 384 - _vm->_platGridY);
			_vm->_platypusSequenceId = 0x7C1;
			_vm->_platypusSequenceDatNum = 1;
			_vm->_platypusId = 20 * _vm->_platY;
			_vm->invAdd(kItemNeedle);
			_vm->setFlag(kGFNeedleTaken);
			_vm->setGrabCursorSprite(kItemNeedle);
			_vm->showCursor();
			_vm->_timers[1] = 30;
			_vm->_platypusActionStatus = -1;
		}
		if (_vm->_platypusActionStatus == kASPlatSearchHaystack) {
			_vm->_gameSys->setAnimation(0, 0, 1);
			_vm->_gameSys->insertSequence(0x145, _vm->_platypusId, _vm->_platypusSequenceId | (_vm->_platypusSequenceDatNum << 16), _vm->_platypusId, kSeqSyncWait, 0, 0, 0);
			_vm->_gameSys->insertSequence(0x146, 256, 0x145, _vm->_platypusId, kSeqSyncWait, 0, 0, 0);
			_vm->hideCursor();
			_vm->setGrabCursorSprite(-1);
			_vm->_platypusSequenceId = 0x146;
			_vm->_platypusSequenceDatNum = 0;
			_vm->_gameSys->setAnimation(0x146, 256, 1);
			_vm->_timers[1] = 300;
		}
	}
	
	if (_vm->_gameSys->getAnimationStatus(3) == 2) {
		if (_s05_nextChickenSequenceId == 0x14B) {
			_vm->_gameSys->setAnimation(_s05_nextChickenSequenceId, 100, 3);
			_vm->_gameSys->insertSequence(_s05_nextChickenSequenceId, 100, _s05_currChickenSequenceId, 100, kSeqSyncWait, 0, 0, 0);
			_vm->_gameSys->insertSequence(0x14C, _vm->_gnapId, makeRid(_vm->_gnapSequenceDatNum, _vm->_gnapSequenceId), _vm->_gnapId, kSeqSyncWait, 0, 0, 0);
			_vm->_gnapSequenceDatNum = 0;
			_vm->_gnapSequenceId = 0x14C;
			_s05_currChickenSequenceId = _s05_nextChickenSequenceId;
			_s05_nextChickenSequenceId = -1;
			_vm->_gnapActionStatus = -1;
		} else if (_s05_nextChickenSequenceId != -1) {
			_vm->_gameSys->setAnimation(_s05_nextChickenSequenceId, 100, 3);
			_vm->_gameSys->insertSequence(_s05_nextChickenSequenceId, 100, _s05_currChickenSequenceId, 100, kSeqSyncWait, 0, 0, 0);
			_s05_currChickenSequenceId = _s05_nextChickenSequenceId;
			_s05_nextChickenSequenceId = -1;
		}
	}
}

} // End of namespace Gnap
