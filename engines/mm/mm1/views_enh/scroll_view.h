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

#ifndef MM1_VIEWS_ENH_SCROLL_VIEW_H
#define MM1_VIEWS_ENH_SCROLL_VIEW_H

#include "mm/mm1/views_enh/text_view.h"

namespace MM {
namespace MM1 {
namespace ViewsEnh {

#define FRAME_BORDER_SIZE 8

class ScrollView : public TextView {
protected:
	Common::Point _symbolPos;
protected:
	/**
	 * Draw the scroll frame
	 */
	void frame();

	/**
	 * Fill the scroll background
	 */
	void fill();

	/**
	 * Draw a special symbol
	 */
	void writeSymbol(int symbolId);
public:
	ScrollView(const Common::String &name);
	ScrollView(const Common::String &name, UIElement *owner);
	virtual ~ScrollView() {}

	/**
	 * Draw the view
	 */
	void draw() override;
};

} // namespace ViewsEnh
} // namespace MM1
} // namespace MM

#endif
