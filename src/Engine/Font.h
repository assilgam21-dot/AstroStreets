#pragma once

// Builds the 8x8 ASCII bitmap font into an OpenGL texture (a 16x8 glyph atlas,
// single channel). Requires a current GL context. Returns the texture id.
unsigned int CreateFontAtlas();
