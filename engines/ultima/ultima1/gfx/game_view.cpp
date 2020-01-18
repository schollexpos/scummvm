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

#include "ultima/ultima1/gfx/game_view.h"
#include "ultima/shared/gfx/info.h"
#include "ultima/shared/gfx/status.h"
#include "ultima/shared/gfx/viewport_dungeon.h"
#include "ultima/shared/gfx/viewport_map.h"
#include "ultima/ultima1/game.h"
#include "ultima/ultima1/gfx/drawing_support.h"

namespace Ultima {
namespace Ultima1 {

GameView::GameView(TreeItem *parent) : Shared::Gfx::VisualContainer("GameView", Rect(0, 0, 320, 200), parent) {
	_info = new Shared::Info(this);
	_status = new Shared::Status(this);
	_viewportDungeon = new Shared::ViewportDungeon(this);
	_viewportMap = new Shared::ViewportMap(this);
}

GameView::~GameView() {
	delete _info;
	delete _status;
	delete _viewportDungeon;
	delete _viewportMap;
}

void GameView::draw() {
	drawFrame();
	Shared::Gfx::VisualContainer::draw();
}

void GameView::drawFrame() {
	DrawingSupport ds(getSurface());
	ds.drawGameFrame();

	Shared::Gfx::VisualSurface vs = getSurface();
	vs.writeString("Testing", Point(100, 80), 1);
}

} // End of namespace Shared
} // End of namespace Ultima
