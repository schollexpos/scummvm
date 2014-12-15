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

#include "common/scummsys.h"

#include "zvision/scripting/controls/safe_control.h"

#include "zvision/zvision.h"
#include "zvision/scripting/script_manager.h"
#include "zvision/graphics/render_manager.h"
#include "zvision/graphics/cursors/cursor_manager.h"
#include "zvision/core/utility.h"

#include "common/stream.h"
#include "common/file.h"
#include "common/tokenizer.h"
#include "common/system.h"
#include "graphics/surface.h"
#include "video/video_decoder.h"

namespace ZVision {

SafeControl::SafeControl(ZVision *engine, uint32 key, Common::SeekableReadStream &stream)
	: Control(engine, key, CONTROL_SAFE) {
	_statesCount = 0;
	_curState = 0;
	_animation = NULL;
	_innerRaduis = 0;
	_innerRadiusSqr = 0;
	_outerRadius = 0;
	_outerRadiusSqr = 0;
	_zeroPointer = 0;
	_startPointer = 0;
	_curFrame = -1;
	_targetFrame = 0;
	_frameTime = 0;
	_lastRenderedFrame = -1;

	// Loop until we find the closing brace
	Common::String line = stream.readLine();
	trimCommentsAndWhiteSpace(&line);
	Common::String param;
	Common::String values;
	getParams(line, param, values);

	while (!stream.eos() && !line.contains('}')) {
		if (param.matchString("animation", true)) {
			_animation = _engine->loadAnimation(values);
		} else if (param.matchString("rectangle", true)) {
			int x;
			int y;
			int width;
			int height;

			sscanf(values.c_str(), "%d %d %d %d", &x, &y, &width, &height);

			_rectangle = Common::Rect(x, y, width, height);
		} else if (param.matchString("num_states", true)) {
			_statesCount = atoi(values.c_str());
		} else if (param.matchString("center", true)) {
			int x;
			int y;

			sscanf(values.c_str(), "%d %d", &x, &y);
			_center = Common::Point(x, y);
		} else if (param.matchString("dial_inner_radius", true)) {
			_innerRaduis = atoi(values.c_str());
			_innerRadiusSqr = _innerRaduis * _innerRaduis;
		} else if (param.matchString("radius", true)) {
			_outerRadius = atoi(values.c_str());
			_outerRadiusSqr = _outerRadius * _outerRadius;
		} else if (param.matchString("zero_radians_offset", true)) {
			_zeroPointer = atoi(values.c_str());
		} else if (param.matchString("pointer_offset", true)) {
			_startPointer = atoi(values.c_str());
			_curState = _startPointer;
		} else if (param.matchString("cursor", true)) {
			// Not used
		} else if (param.matchString("mirrored", true)) {
			// Not used
		} else if (param.matchString("venus_id", true)) {
			_venusId = atoi(values.c_str());
		}

		line = stream.readLine();
		trimCommentsAndWhiteSpace(&line);
		getParams(line, param, values);
	}
	renderFrame(_curState);
}

SafeControl::~SafeControl() {
	if (_animation)
		delete _animation;

}

void SafeControl::renderFrame(uint frameNumber) {
	if (frameNumber == 0) {
		_lastRenderedFrame = frameNumber;
	} else if ((int16)frameNumber < _lastRenderedFrame) {
		_lastRenderedFrame = frameNumber;
		frameNumber = (_statesCount * 2) - frameNumber;
	} else {
		_lastRenderedFrame = frameNumber;
	}

	const Graphics::Surface *frameData;
	int x = _rectangle.left;
	int y = _rectangle.top;

	_animation->seekToFrame(frameNumber);
	frameData = _animation->decodeNextFrame();
	if (frameData)
		_engine->getRenderManager()->blitSurfaceToBkg(*frameData, x, y);
}

bool SafeControl::process(uint32 deltaTimeInMillis) {
	if (_engine->getScriptManager()->getStateFlag(_key) & Puzzle::DISABLED)
		return false;

	if (_curFrame != _targetFrame) {
		_frameTime -= deltaTimeInMillis;

		if (_frameTime <= 0) {
			if (_curFrame < _targetFrame) {
				_curFrame++;
				renderFrame(_curFrame);
			} else if (_curFrame > _targetFrame) {
				_curFrame--;
				renderFrame(_curFrame);
			}
			_frameTime = 1000.0 / _animation->getDuration().framerate();
		}
	}
	return false;
}

bool SafeControl::onMouseMove(const Common::Point &screenSpacePos, const Common::Point &backgroundImageSpacePos) {
	if (_engine->getScriptManager()->getStateFlag(_key) & Puzzle::DISABLED)
		return false;

	if (_rectangle.contains(backgroundImageSpacePos)) {
		int32 mR = backgroundImageSpacePos.sqrDist(_center);
		if (mR <= _outerRadiusSqr && mR >= _innerRadiusSqr) {
			_engine->getCursorManager()->changeCursor(CursorIndex_Active);
			return true;
		}
	}
	return false;
}

bool SafeControl::onMouseUp(const Common::Point &screenSpacePos, const Common::Point &backgroundImageSpacePos) {
	if (_engine->getScriptManager()->getStateFlag(_key) & Puzzle::DISABLED)
		return false;

	if (_rectangle.contains(backgroundImageSpacePos)) {
		int32 mR = backgroundImageSpacePos.sqrDist(_center);
		if (mR <= _outerRadiusSqr && mR >= _innerRadiusSqr) {
			setVenus();

			Common::Point tmp = backgroundImageSpacePos - _center;

			float dd = atan2((float)tmp.x, (float)tmp.y) * 57.29578;

			int16 dp_state = 360 / _statesCount;

			int16 m_state = (_statesCount - ((((int16)dd + 540) % 360) / dp_state)) % _statesCount;

			int16 tmp2 = (m_state + _curState - _zeroPointer + _statesCount - 1) % _statesCount;

			_curFrame = (_curState + _statesCount - _startPointer) % _statesCount;

			_curState = (_statesCount * 2 + tmp2) % _statesCount;

			_targetFrame = (_curState + _statesCount - _startPointer) % _statesCount;

			_engine->getScriptManager()->setStateValue(_key, _curState);
			return true;
		}
	}
	return false;
}

} // End of namespace ZVision
