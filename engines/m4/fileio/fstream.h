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

#ifndef M4_FILEIO_FSTREAM_H
#define M4_FILEIO_FSTREAM_H

#include "common/file.h"
#include "m4/m4_types.h"

namespace M4 {

class StreamFile {
private:
	Common::File _file;
public:
	StreamFile(const Common::String &filename);
	~StreamFile() {}

	int32 read(Handle bufferHandle, int32 n);
	bool seek(uint32 n);
	bool seek_ahead(uint32 n);
	uint32 get_pos();
};

struct strmRequest {
	strmRequest *next;
	strmRequest *prev;
	StreamFile *srcFile;
	int32 strmSize;
	Handle strmHandle;
	uint8 *strmBuff;
	uint8 *endStrmBuff;
	uint8 *strmHead;
	uint8 *strmTail;
	uint8 *strmWrap;
	uint8 *strmLastRead;
	int32 nextReadSize;
	int32 numBlocksToRead;
	int32 *blockSizeArray;
	bool wrapStream;
};

extern bool f_stream_Init();

} // namespace M4

#endif
