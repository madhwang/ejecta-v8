#ifndef __EJFONT_H
#define __EJFONT_H	1

#include "EJTexture.h"

class EJCanvasContext;

typedef struct
{
    wchar_t charcode;
    float kerning;
} kerning_t;

typedef struct
{
    wchar_t charcode;
    int width, height;
    int offset_x, offset_y;
    float advance_x, advance_y;
    float s0, t0, s1, t1;
    size_t kerning_count;
    kerning_t kerning[24];
} texture_glyph_t;

typedef struct
{
    size_t tex_width;
    size_t tex_height;
    size_t tex_depth;
    char tex_data[65536];
    float size;
    float height;
    float linegap;
    float ascender;
    float descender;
    size_t glyphs_count;
    texture_glyph_t glyphs[108];
} texture_font_t;

class EJFont {
private:
	texture_font_t* _font;
	EJTexture* _texture;
	float _scale;
	bool _copy;
	wchar_t* _utf32buffer;
	int _utf32bufsize;
public:
	EJFont (const char* font, int size, bool fill, float contentScale);
	void drawString (const char* text, EJCanvasContext* context, float x, float y);
	float measureString (const char* string);
	float measureStringFromBuffer (int length);
	~EJFont();
};

#endif
