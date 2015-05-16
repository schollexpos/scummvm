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

#include "sherlock/resources.h"
#include "sherlock/screen.h"
#include "sherlock/sherlock.h"
#include "common/debug.h"
#include "common/memstream.h"

namespace Sherlock {

Cache::Cache(SherlockEngine *vm): _vm(vm) {
}

/**
 * Returns true if a given file is currently being cached
 */
bool Cache::isCached(const Common::String &filename) const {
	return _resources.contains(filename);
}

/**
 * Loads a file into the cache if it's not already present, and returns it.
 * If the file is LZW compressed, automatically decompresses it and loads
 * the uncompressed version into memory
 */
void Cache::load(const Common::String &name) {
	// First check if the entry already exists
	if (_resources.contains(name))
		return;

	// Open the file for reading
	Common::File f;
	if (!f.open(name))
		error("Could not read file - %s", name.c_str());

	load(name, f);

	f.close();
}

/**
 * Load a cache entry based on a passed stream
 */
void Cache::load(const Common::String &name, Common::SeekableReadStream &stream) {
	// First check if the entry already exists
	if (_resources.contains(name))
		return;

	int32 signature = stream.readUint32BE();
	stream.seek(0);

	// Allocate a new cache entry
	_resources[name] = CacheEntry();
	CacheEntry &cacheEntry = _resources[name];

	// Check whether the file is compressed
	if (signature == MKTAG('L', 'Z', 'V', 26)) {
		// It's compressed, so decompress the file and store it's data in the cache entry
		Common::SeekableReadStream *decompressed = _vm->_res->decompress(stream);
		cacheEntry.resize(decompressed->size());
		decompressed->read(&cacheEntry[0], decompressed->size());

		delete decompressed;
	} else {
		// It's not, so read the raw data of the file into the cache entry
		cacheEntry.resize(stream.size());
		stream.read(&cacheEntry[0], stream.size());
	}
}

/**
 * Get a file from the cache
 */
Common::SeekableReadStream *Cache::get(const Common::String &filename) const {
	// Return a memory stream that encapsulates the data
	const CacheEntry &cacheEntry = _resources[filename];
	return new Common::MemoryReadStream(&cacheEntry[0], cacheEntry.size());
}

/*----------------------------------------------------------------*/

Resources::Resources(SherlockEngine *vm): _vm(vm), _cache(vm) {
	_resourceIndex = -1;

	addToCache("vgs.lib");
	addToCache("talk.lib");
	addToCache("journal.txt");
}

/**
 * Adds the specified file to the cache. If it's a library file, takes care of
 * loading it's index for future use
 */
void Resources::addToCache(const Common::String &filename) {
	_cache.load(filename);

	// Check to see if the file is a library
	Common::SeekableReadStream *stream = load(filename);
	uint32 header = stream->readUint32BE();

	if (header == MKTAG('L', 'I', 'B', 26))
		loadLibraryIndex(filename, stream, false);
	else if (header == MKTAG('L', 'I', 'C', 26))
		loadLibraryIndex(filename, stream, true);

	delete stream;
}

/**
 * Adds a resource from a library file to the cache
 */
void Resources::addToCache(const Common::String &filename, const Common::String &libFilename) {
	// Get the resource
	Common::SeekableReadStream *stream = load(filename, libFilename);

	_cache.load(filename, *stream);

	delete stream;
}

/**
 * Adds a given stream to the cache under the given name
 */
void Resources::addToCache(const Common::String &filename, Common::SeekableReadStream &stream) {
	_cache.load(filename, stream);
}

/**
 * Returns a stream for a given file
 */
Common::SeekableReadStream *Resources::load(const Common::String &filename) {
	// First check if the file is directly in the cache
	if (_cache.isCached(filename))
		return _cache.get(filename);

	// Secondly, iterate through any loaded library file looking for a resource
	// that has the same name
	LibraryIndexes::iterator i;
	for (i = _indexes.begin(); i != _indexes.end(); ++i) {
		if ((*i)._value.contains(filename)) {
			// Get a stream reference to the given library file
			Common::SeekableReadStream *stream = load((*i)._key);
			LibraryEntry &entry = (*i)._value[filename];
			_resourceIndex = entry._index;

			stream->seek(entry._offset);
			Common::SeekableReadStream *resStream = stream->readStream(entry._size);

			// Check whether the file is compressed
			if (resStream->readUint32BE() == MKTAG('L', 'Z', 'V', 26)) {
				resStream->seek(0);
				// It's compressed, so decompress the sub-file and return it
				Common::SeekableReadStream *decompressed = decompress(*resStream);
				delete stream;
				delete resStream;
				return decompressed;
			} else {
				resStream->seek(0);
				delete stream;
				return resStream;
			}
		}
	}

	// At this point, fall back on a physical file with the given name
	Common::File f;
	if (!f.open(filename))
		error("Could not load file - %s", filename.c_str());

	Common::SeekableReadStream *stream = f.readStream(f.size());
	f.close();

	return stream;
}

/**
 * Loads a specific resource from a given library file
 */
Common::SeekableReadStream *Resources::load(const Common::String &filename, const Common::String &libraryFile) {
	// Open up the library for access
	Common::SeekableReadStream *libStream = load(libraryFile);

	// Check if the library has already had it's index read, and if not, load it
	if (!_indexes.contains(libraryFile))
		loadLibraryIndex(libraryFile, libStream, false);

	// Extract the data for the specified resource and return it
	LibraryEntry &entry = _indexes[libraryFile][filename];
	libStream->seek(entry._offset);
	Common::SeekableReadStream *stream = libStream->readStream(entry._size);

	delete libStream;
	return stream;
}

/**
 * Returns true if the given file exists on disk or in the cache
 */
bool Resources::exists(const Common::String &filename) const {
	Common::File f;
	return f.exists(filename) || _cache.isCached(filename);
}

/**
 * Reads in the index from a library file, and caches it's index for later use
 */
void Resources::loadLibraryIndex(const Common::String &libFilename,
		Common::SeekableReadStream *stream, bool isNewStyle) {
	uint32 offset, nextOffset;

	// Create an index entry
	_indexes[libFilename] = LibraryIndex();
	LibraryIndex &index = _indexes[libFilename];

	// Read in the number of resources
	stream->seek(4);
	int count = stream->readUint16LE();

	if (isNewStyle)
		stream->seek((count + 1) * 8, SEEK_CUR);

	// Loop through reading in the entries
	for (int idx = 0; idx < count; ++idx) {
		// Read the name of the resource
		char resName[13];
		stream->read(resName, 13);
		resName[12] = '\0';

		// Read the offset
		offset = stream->readUint32LE();

		if (idx == (count - 1)) {
			nextOffset = stream->size();
		} else {
			// Read the size by jumping forward to read the next entry's offset
			stream->seek(13, SEEK_CUR);
			nextOffset = stream->readUint32LE();
			stream->seek(-17, SEEK_CUR);
		}

		// Add the entry to the index
		index[resName] = LibraryEntry(idx, offset, nextOffset - offset);
	}
}

/**
 * Returns the index of the last loaded resource in it's given library file.
 * This will be used primarily when loading talk files, so the engine can
 * update the given conversation number in the journal
 */
int Resources::resourceIndex() const {
	return _resourceIndex;
}

/*----------------------------------------------------------------*/

SherlockEngine *ImageFile::_vm;

void ImageFile::setVm(SherlockEngine *vm) {
	_vm = vm;
}

ImageFile::ImageFile(const Common::String &name, bool skipPal, bool animImages) {
	Common::SeekableReadStream *stream = _vm->_res->load(name);

	Common::fill(&_palette[0], &_palette[PALETTE_SIZE], 0);
	load(*stream, skipPal, animImages);

	delete stream;
}

ImageFile::ImageFile(Common::SeekableReadStream &stream, bool skipPal) {
	Common::fill(&_palette[0], &_palette[PALETTE_SIZE], 0);
	load(stream, skipPal, false);
}

ImageFile::~ImageFile() {
	for (uint idx = 0; idx < size(); ++idx)
		(*this)[idx]._frame.free();
}

/**
 * Load the data of the sprite
 */
void ImageFile::load(Common::SeekableReadStream &stream, bool skipPalette, bool animImages) {
	loadPalette(stream);

	int streamSize = stream.size();
	while (stream.pos() < streamSize) {
		ImageFrame frame;
		frame._width = stream.readUint16LE() + 1;
		frame._height = stream.readUint16LE() + 1;
		frame._paletteBase = stream.readByte();

		if (animImages) {
			// Animation cutscene image files use a 16-bit x offset
			frame._offset.x = stream.readUint16LE();
			frame._rleEncoded = (frame._offset.x & 0xff) == 1;
			frame._offset.y = stream.readByte();
		} else {
			// Standard image files have a separate byte for the RLE flag, and an 8-bit X offset
			frame._rleEncoded = stream.readByte() == 1;
			frame._offset.x = stream.readByte();
			frame._offset.y = stream.readByte();
		}

		frame._offset.y = stream.readByte();
		frame._rleEncoded = !skipPalette && frame._rleEncoded;

		if (frame._paletteBase) {
			// Nibble packed frame data
			frame._size = (frame._width * frame._height) / 2;
		} else if (frame._rleEncoded) {
			// This size includes the header size, which we subtract
			frame._size = stream.readUint16LE() - 11;
			frame._rleMarker = stream.readByte();
		} else {
			// Uncompressed data
			frame._size = frame._width * frame._height;
		}

		// Load data for frame and decompress it
		byte *data = new byte[frame._size];
		stream.read(data, frame._size);
		decompressFrame(frame, data);
		delete[] data;

		push_back(frame);
	}
}

/**
 * Gets the palette at the start of the sprite file
 */
void ImageFile::loadPalette(Common::SeekableReadStream &stream) {
	// Check for palette
	int v1 = stream.readUint16LE() + 1;
	int v2 = stream.readUint16LE() + 1;
	stream.skip(1);		// Skip paletteBase byte
	bool rleEncoded = stream.readByte() == 1;
	int palSize = v1 * v2;

	if ((palSize - 12) == PALETTE_SIZE && !rleEncoded) {
		// Found palette, so read it in
		stream.seek(2 + 12, SEEK_CUR);
		for (int idx = 0; idx < PALETTE_SIZE; ++idx)
			_palette[idx] = VGA_COLOR_TRANS(stream.readByte());
	} else {
		// Not a palette, so rewind to start of frame data for normal frame processing
		stream.seek(-6, SEEK_CUR);
	}
}

/**
 * Decompress a single frame for the sprite
 */
void ImageFile::decompressFrame(ImageFrame &frame, const byte *src) {
	frame._frame.create(frame._width, frame._height, Graphics::PixelFormat::createFormatCLUT8());

	if (frame._paletteBase) {
		// Nibble-packed
		byte *pDest = (byte *)frame._frame.getPixels();
		for (uint idx = 0; idx < frame._size; ++idx, ++src) {
			*pDest++ = *src & 0xF;
			*pDest++ = (*src >> 4);
		}
	} else if (frame._rleEncoded) {
		// RLE encoded
		byte *dst = (byte *)frame._frame.getPixels();

		int frameSize = frame._width * frame._height;
		while (frameSize > 0) {
			if (*src == frame._rleMarker) {
				byte rleColor = src[1];
				byte rleCount = src[2];
				src += 3;
				frameSize -= rleCount;
				while (rleCount--)
					*dst++ = rleColor;
			} else {
				*dst++ = *src++;
				--frameSize;
			}
		}
		assert(frameSize == 0);
	} else {
		// Uncompressed frame
		Common::copy(src, src + frame._width * frame._height,
			(byte *)frame._frame.getPixels());
	}
}

/**
 * Decompress an LZW compressed resource
 */
Common::SeekableReadStream *Resources::decompress(Common::SeekableReadStream &source) {
	// This variation can't be used by Rose Tattoo, since compressed resources include the input size,
	// not the output size. Which means their decompression has to be done via passed buffers
	assert(_vm->getGameID() == GType_SerratedScalpel);

	uint32 id = source.readUint32BE();
	assert(id == MKTAG('L', 'Z', 'V', 0x1A));

	uint32 outputSize = source.readUint32LE();
	return decompressLZ(source, outputSize);
}

/**
 * Decompress an LZW compressed resource
 */
void Resources::decompress(Common::SeekableReadStream &source, byte *buffer, uint32 outSize) {
	assert(_vm->getGameID() == GType_RoseTattoo);

	uint32 inputSize = source.readUint32LE();
	decompressLZ(source, buffer, outSize, inputSize);
}

/**
 * Decompresses an LZW block of data with a specified output size
 */
Common::SeekableReadStream *Resources::decompressLZ(Common::SeekableReadStream &source, uint32 outSize) {
	byte *outBuffer = (byte *)malloc(outSize);
	Common::MemoryReadStream *outStream = new Common::MemoryReadStream(outBuffer, outSize, DisposeAfterUse::YES);

	decompressLZ(source, outSize);
	return outStream;
}

void Resources::decompressLZ(Common::SeekableReadStream &source, byte *outBuffer, int32 outSize, int32 inSize) {
	byte lzWindow[4096];
	uint16 lzWindowPos;
	uint16 cmd;
	byte *outBufferEnd = outBuffer + outSize;
	uint32 endPos = source.pos() + inSize;

	memset(lzWindow, 0xFF, 0xFEE);
	lzWindowPos = 0xFEE;
	cmd = 0;

	do {
		cmd >>= 1;
		if (!(cmd & 0x100))
			cmd = source.readByte() | 0xFF00;

		if (cmd & 1) {
			byte literal = source.readByte();
			*outBuffer++ = literal;
			lzWindow[lzWindowPos] = literal;
			lzWindowPos = (lzWindowPos + 1) & 0x0FFF;
		} else {
			int copyPos, copyLen;
			copyPos = source.readByte();
			copyLen = source.readByte();
			copyPos = copyPos | ((copyLen & 0xF0) << 4);
			copyLen = (copyLen & 0x0F) + 3;
			while (copyLen--) {
				byte literal = lzWindow[copyPos];
				copyPos = (copyPos + 1) & 0x0FFF;
				*outBuffer++ = literal;
				lzWindow[lzWindowPos] = literal;
				lzWindowPos = (lzWindowPos + 1) & 0x0FFF;
			}
		}
	} while ((!outSize || outBuffer < outBufferEnd) && (!inSize || source.pos() < endPos));
}

} // End of namespace Sherlock
