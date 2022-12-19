#ifndef STYR_BASIC_UI_TEXT_H
#define STYR_BASIC_UI_TEXT_H

struct engine_font_glyph
{
	u32 Codepoint;
	f32 Width;
	f32 Height;
	f32 WidthOverHeight;
	v2 AlignPercentage;
	void *Data;
	
	v2 TexCoord_LeftTop;
	v2 TexCoord_RightTop;
	v2 TexCoord_LeftBottom;
	v2 TexCoord_RightBottom;
};

struct engine_font_info
{
	loaded_font *DebugFont;
	sta_font *DebugFontInfo;
	
	f32 LeftEdge;
	f32 RightEdge;
	f32 AtY;
	f32 FontScale;
	font_id FontID;
	f32 GlobalWidth;
	f32 GlobalHeight;
};

struct basic_ui_text_state
{
	u32 max_vertices_count;
	u32 vertices_count;
	u32 indices_count;
	u32 quad_count;
	
	vertex_3d *vertices_buffer;
	u32 *indices_buffer;
	
	u32 font_glyph_count;
	engine_font_glyph *font_glyphs;
	engine_font_info font_info;
};

#endif //STYR_BASIC_UI_TEXT_H
