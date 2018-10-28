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

#ifndef GARGOYLE_DRAW_H
#define GARGOYLE_DRAW_H

#include "graphics/screen.h"
#include "gargoyle/fonts.h"

namespace Gargoyle {

class Screen : public Graphics::Screen, Fonts {
public:
	/**
	 * Fills the screen with a given rgb color
	 */
	void fill(const byte *rgb);

	/**
	 * Fill a given area of the screen with an rgb color
	 */
	void fillRect(uint x, uint y, uint w, uint h, const byte *rgb);

	int drawString(int x, int y, int fidx, const byte *rgb, const char *s, int n, int spw);

	int drawStringUni(int x, int y, int fidx, const byte *rgb, const uint32 *s, int n, int spw);

	int stringWidth(int fidx, const char *s, int n, int spw);

	int stringWidthUni(int fidx, const uint32 *s, int n, int spw);

	void drawCaret(const Common::Point &pos);
};

} // End of namespace Gargoyle

#endif
