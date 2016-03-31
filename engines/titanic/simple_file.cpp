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

#include "common/util.h"
#include "titanic/simple_file.h"

namespace Titanic {

bool File::open(const Common::String &name) {
	if (!Common::File::open(name))
		error("Could not open file - %s", name.c_str());
	return true;
}

/*------------------------------------------------------------------------*/

SimpleFile::SimpleFile(): _inStream(nullptr), _outStream(nullptr), _lineCount(1) {
}

SimpleFile::~SimpleFile() {
	close();
}

void SimpleFile::open(Common::SeekableReadStream *stream) {
	close();	
	_inStream = stream;
}

void SimpleFile::open(Common::OutSaveFile *stream) {
	close();
	_outStream = stream;
}

void SimpleFile::close() {
	if (_outStream) {
		_outStream->finalize();
		delete _outStream;
		_outStream = nullptr;
	}

	if (_inStream) {
		delete _inStream;
		_inStream = nullptr;
	}
}

void SimpleFile::safeRead(void *dst, size_t count) {
	if (unsafeRead(dst, count) != count)
		error("Could not read %d bytes", (int)count);
}

size_t SimpleFile::unsafeRead(void *dst, size_t count) {
	assert(_inStream);
	return _inStream->read(dst, count);
}

size_t SimpleFile::write(const void *src, size_t count) {
	assert(_outStream);
	return _outStream->write(src, count);
}

CString SimpleFile::readString() {
	char c;
	CString result;
	bool backslashFlag = false;

	// First skip any spaces
	do {
		safeRead(&c, 1);
	} while (Common::isSpace(c));

	// Ensure we've found a starting quote for the string
	if (c != '"')
		error("Could not find starting quote");

	bool endFlag = false;
	while (!endFlag) {
		// Read the next character
		safeRead(&c, 1);

		if (backslashFlag) {
			backslashFlag = false;
			switch (c) {
			case 'n':
				result += '\n';
				break;
			case 'r':
				result += '\r';
				break;
			case '\t':
				result += '\t';
				break;
			default:
				result += c;
				break;
			}
		} else {
			switch (c) {
			case '"':
				endFlag = true;
				break;
			case '\\':
				backslashFlag = true;
				break;
			default:
				result += c;
				break;
			}
		}
	}

	// Return the string
	return result;
}

int SimpleFile::readNumber() {
	char c;
	int result = 0;
	bool minusFlag = false;

	// First skip any spaces
	do {
		safeRead(&c, 1);
	} while (Common::isSpace(c));

	// Check for prefix sign
	if (c == '+' || c == '-') {
		minusFlag = c == '-';
		safeRead(&c, 1);
	}

	// Read in the number
	if (!Common::isDigit(c))
		error("Invalid number");

	while (Common::isDigit(c)) {
		result = result * 10 + (c - '0');
		safeRead(&c, 1);
	}

	// Finally, if it's a minus value, then negate it
	if (minusFlag)
		result = -result;

	return result;
}

double SimpleFile::readFloat() {
	char c;
	Common::String result;

	// First skip any spaces
	do {
		safeRead(&c, 1);
	} while (Common::isSpace(c));

	// Check for prefix sign
	if (c == '+' || c == '-') {
		result += c;
		safeRead(&c, 1);
	}

	// Read in the number
	if (!Common::isDigit(c))
		error("Invalid number");

	while (Common::isDigit(c) || c == '.') {
		result += c;
		safeRead(&c, 1);
	}

	// Convert to a float and return it
	float floatValue;
	sscanf(result.c_str(), "%f", &floatValue);

	return floatValue;
}

Point SimpleFile::readPoint() {
	Point pt;
	pt.x = readNumber();
	pt.y = readNumber();

	return pt;
}

Rect SimpleFile::readRect() {
	Rect r;
	r.left = readNumber();
	r.top = readNumber();
	r.right = readNumber();
	r.bottom = readNumber();

	return r;
}

void SimpleFile::readBuffer(char *buffer, size_t count) {
	CString tempString = readString();
	if (buffer) {
		strncpy(buffer, tempString.c_str(), count);
		buffer[count - 1] = '\0';
	}
}

void SimpleFile::writeLine(const CString &str) {
	write(str.c_str(), str.size());
	write("\r\n", 2);
}

void SimpleFile::writeString(const CString &str) {
	if (str.empty())
		return;

	const char *msgP = str.c_str();
	char c;
	
	while ((c = *msgP++) != '\0') {
		switch (c) {
		case '\r':
			write("\\r", 2);
			break;
		case '\n':
			write("\\n", 2);
			break;
		case '\t':
			write("\\t", 2);
			break;
		case '\"':
			write("\\\"", 2);
			break;
		case '\\':
			write("\\\\", 2);
			break;
		case '{':
			write("\\{", 2);
			break;
		case '}':
			write("\\}", 2);
			break;
		default:
			write(&c, 1);
			break;
		}
	}
}

void SimpleFile::writeQuotedString(const CString &str) {
	write("\"", 1);
	writeString(str);
	write("\" ", 2);
}

void SimpleFile::writeQuotedLine(const CString &str, int indent) {
	writeIndent(indent);
	writeQuotedString(str);
	write("\n", 1);
}

void SimpleFile::writeNumber(int val) {
	CString str = CString::format("%d ", val);
	write(str.c_str(), str.size());
}

void SimpleFile::writeNumberLine(int val, int indent) {
	writeIndent(indent);
	writeNumber(val);
	write("\n", 1);
}

void SimpleFile::writeFloat(double val) {
	Common::String valStr = Common::String::format("%f ", val);
	write(valStr.c_str(), valStr.size());
}

void SimpleFile::writeFloatLine(double val, int indent) {
	writeIndent(indent);
	writeFloat(val);
	write("\n", 1);
}

void SimpleFile::writePoint(const Point &pt, int indent) {
	writeIndent(indent);
	writeNumber(pt.x);
	writeNumber(pt.y);
	write("\n", 1);
}

void SimpleFile::writeRect(const Rect &r, int indent) {
	writePoint(Point(r.left, r.top), indent);
	writePoint(Point(r.right, r.bottom), indent);
}

void SimpleFile::writeIndent(uint indent) {
	for (uint idx = 0; idx < indent; ++idx)
		write("\t", 1);
}

bool SimpleFile::IsClassStart() {
	char c;

	do {
		safeRead(&c, 1);
	} while (Common::isSpace(c));

	assert(c == '{' || c == '}');
	return c == '{';
}

void SimpleFile::writeClassStart(const CString &classStr, int indent) {
	write("\n", 1);
	writeIndent(indent);
	write("{\n", 2);
	writeIndent(indent + 1);
	writeQuotedString(classStr);
	write("\n", 1);
}

void SimpleFile::writeClassEnd(int indent) {
	writeIndent(indent);
	write("}\n", 2);
}

/*------------------------------------------------------------------------*/

void StdCWadFile::open(const CString &name) {
	File f;

	// Check for whether it is indeed a file/resource pair
	int idx = name.indexOf('#');

	if (idx < 0) {
		// Nope, so open up file for standard reading
		assert(!name.empty());
		f.open(name);

		SimpleFile::open(f.readStream(f.size()));
		return;
	}

	// Split up the name and resource, and get the resource index
	CString filename = name.left(idx) + ".st";
	int extPos = name.lastIndexOf('.');
	CString resStr = name.mid(idx + 1, extPos - idx - 1);
	int resIndex = resStr.readInt();

	// Open up the index for access 
	f.open(filename);
	int indexSize = f.readUint32LE() / 4;
	assert(resIndex < indexSize);

	// Get the specific resource's offset, and size by also
	// getting the offset of the following resource
	f.seek(resIndex * 4);
	uint resOffset = f.readUint32LE();
	uint nextOffset = (resIndex == (indexSize - 1)) ? f.size() :
		f.readUint32LE();

	// Read in the resource
	f.seek(resOffset);
	Common::SeekableReadStream *stream = f.readStream(nextOffset - resOffset);
	SimpleFile::open(stream);

	f.close();
}

} // End of namespace Titanic
