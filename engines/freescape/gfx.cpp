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

#include "common/config-manager.h"
#include "graphics/renderer.h"
#include "graphics/surface.h"
#if defined(USE_OPENGL_GAME) || defined(USE_OPENGL_SHADERS) || defined(USE_GLES2)
#include "graphics/opengl/context.h"
#endif

#include "engines/util.h"
#include "math/glmath.h"

#include "engines/freescape/gfx.h"

namespace Freescape {

Renderer::Renderer(OSystem *system, int screenW, int screenH)
	: _system(system) {

	_screenW = screenW;
	_screenH = screenH;
	_currentPixelFormat = Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0);
	_originalPixelFormat = Graphics::PixelFormat::createFormatCLUT8();
	_palettePixelFormat = Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0);
	_keyColor = -1;
	_palette = nullptr;
	_colorMap = nullptr;
	_isAmiga = false;
	_isAtariST = false;
}

Renderer::~Renderer() {}

Graphics::Surface *Renderer::convertFromPalette(Graphics::PixelBuffer *rawsurf) {
	Graphics::Surface *surf = new Graphics::Surface();
	surf->create(_screenW, _screenH, _originalPixelFormat);
	surf->copyRectToSurface(rawsurf->getRawBuffer(), surf->w, 0, 0, surf->w, surf->h);
	surf->convertToInPlace(_currentPixelFormat, _palette->getRawBuffer());
	return surf;
}

bool Renderer::getRGBAt(uint8 index, uint8 &r, uint8 &g, uint8 &b) {

	if (index == _keyColor)
		return false;

	if (index == 0) {
		_palette->getRGBAt(0, r, g, b);
		return true;
	}

	if (_isAmiga || _isAtariST) {
		_palette->getRGBAt(index, r, g, b);
		return true;
	}

	// assert(index-1 < _colorMap->size());
	byte *entry = (*_colorMap)[index - 1];
	uint8 color = 0;
	uint8 acc = 1;
	for (int i = 0; i < 4; i++) {
		byte be = *entry;
		if (be != 0 && be != 0xff) {
			// TODO: fix colors for non-DOS releases
			_palette->getRGBAt(index, r, g, b);
			return true;
		}

		if (be == 0xff)
			color = color + acc;

		acc = acc << 1;
		entry++;
	}
	assert(color < 16);
	_palette->getRGBAt(color, r, g, b);
	return true;
}

Common::Rect Renderer::viewport() const {
	return _screenViewport;
}

void Renderer::computeScreenViewport() {
	_screenViewport = Common::Rect(_screenW, _screenH);
}

Renderer *createRenderer(OSystem *system, int screenW, int screenH) {
	Common::String rendererConfig = ConfMan.get("renderer");
	Graphics::PixelFormat pixelFormat = Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0);
	Graphics::RendererType desiredRendererType = Graphics::kRendererTypeTinyGL;  // Graphics::parseRendererTypeCode(rendererConfig);
	Graphics::RendererType matchingRendererType = Graphics::kRendererTypeTinyGL; // Graphics::getBestMatchingAvailableRendererType(desiredRendererType);

	bool isAccelerated = 0; // matchingRendererType != Graphics::kRendererTypeTinyGL;

	if (isAccelerated) {
		initGraphics3d(screenW, screenH);
	} else {
		initGraphics(screenW, screenH, &pixelFormat);
	}

	/*#if defined(USE_OPENGL_GAME) || defined(USE_OPENGL_SHADERS) || defined(USE_GLES2)
		bool backendCapableOpenGL = g_system->hasFeature(OSystem::kFeatureOpenGLForGame);
	#endif

	#if defined(USE_OPENGL_GAME)
		// Check the OpenGL context actually supports shaders
		if (backendCapableOpenGL && matchingRendererType == Graphics::kRendererTypeOpenGLShaders && !OpenGLContext.shadersSupported) {
			matchingRendererType = Graphics::kRendererTypeOpenGL;
		}
	#endif*/

	if (matchingRendererType != desiredRendererType && desiredRendererType != Graphics::kRendererTypeDefault) {
		// Display a warning if unable to use the desired renderer
		warning("Unable to create a '%s' renderer", rendererConfig.c_str());
	}
	/*
	#if defined(USE_GLES2) || defined(USE_OPENGL_SHADERS)
		if (backendCapableOpenGL && matchingRendererType == Graphics::kRendererTypeOpenGLShaders) {
			return CreateGfxOpenGLShader(system);
		}
	#endif
	#if defined(USE_OPENGL_GAME) && !defined(USE_GLES2)
		if (backendCapableOpenGL && matchingRendererType == Graphics::kRendererTypeOpenGL) {
			return CreateGfxOpenGL(system);
		}
	#endif*/
	if (matchingRendererType == Graphics::kRendererTypeTinyGL) {
		return CreateGfxTinyGL(system, screenW, screenH);
	}

	error("Unable to create a '%s' renderer", rendererConfig.c_str());
}

} // End of namespace Freescape
