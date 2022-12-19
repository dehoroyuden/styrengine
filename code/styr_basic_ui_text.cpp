
inline engine_font_info
get_default_font_info(game_assets *assets, u32 screen_width, u32 screen_height)
{
	engine_font_info result = {};
	
	u32 font_index = GetMatchAssetFrom(assets, Asset_Font, FontType_Debug);
	//u32 font_index = GetMatchAssetFrom(assets, Asset_Font, 5);
	result.DebugFontInfo = GetFontInfo(assets, {font_index});
	LoadFont(assets, {font_index});
	result.DebugFont = GetFont(assets, {font_index});
	result.FontID = {font_index};
	
	result.FontScale = 1.0f;
	result.LeftEdge = 0.01f * screen_width;
	result.RightEdge = 0.5f * screen_width;
	result.AtY = 0.05f * screen_height;
	
	return result;
}

internal basic_ui_text_state *
initialize_basic_ui_text_state(memory_arena *basic_ui_text_state_mem, memory_arena *tran_memory, game_assets *assets, u32 screen_width, u32 screen_height, u32 max_vertices_count)
{
	basic_ui_text_state *Result = PushStruct(basic_ui_text_state_mem, basic_ui_text_state);
	Result->max_vertices_count = max_vertices_count;
	Result->quad_count = 0;
	Result->vertices_buffer = PushArray(tran_memory, max_vertices_count, vertex_3d);
	Result->indices_buffer = PushArray(tran_memory, (max_vertices_count / 4) * 6, u32);
	
	Result->font_info = get_default_font_info(assets, screen_width, screen_height);
	Result->font_glyph_count = Result->font_info.DebugFontInfo->GlyphCount;
	
	Result->font_glyphs = PushArray(tran_memory, Result->font_glyph_count, engine_font_glyph);
	
	u32 Index = 0;
	for(char CodePoint = '!';
		CodePoint <= '~';
		++CodePoint)
	{
		bitmap_id ID = GetBitmapForGlyph(Result->font_info.DebugFontInfo, Result->font_info.DebugFont, CodePoint);
		loaded_bitmap *Bitmap = GetBitmap(assets, ID);
		
		if(!Bitmap)
		{
			LoadBitmap(assets, ID);
			Bitmap = GetBitmap(assets, ID);
		}
		
		Result->font_glyphs[Index].Codepoint = CodePoint;
		Result->font_glyphs[Index].Width = (f32)Bitmap->Width;
		Result->font_glyphs[Index].Height = (f32)Bitmap->Height;
		Result->font_glyphs[Index].WidthOverHeight = Bitmap->WidthOverHeight;
		Result->font_glyphs[Index].AlignPercentage = Bitmap->AlignPercentage;
		Result->font_glyphs[Index].Data = Bitmap->Memory;
		
		u32 CurrentDim = (Bitmap->Width > Bitmap->Height) ? Bitmap->Width : Bitmap->Height;
		++Index;
	}
	
	return Result;
}

internal void
draw_ui_text(basic_ui_text_state *state, u32 screen_width, u32 screen_height, v2 Position, f32 Height, u32 Codepoint, v4 Color)
{
	engine_font_glyph Glyph = {};
	
	for(u32 index =  0;
		index < state->font_glyph_count;
		++index)
	{
		if(Codepoint == state->font_glyphs[index].Codepoint)
		{
			Glyph = state->font_glyphs[index];
			break;
		}
	}
	
	v2 Size = V2(Height*Glyph.WidthOverHeight, Height);
	v2 Align = Hadamard(Glyph.AlignPercentage, Size);
	v2 AlignedP = Position + Align;
	
	v2 MinP = AlignedP;
	v2 MaxP = MinP + Size.x * V2(1,0) - (Size.y * V2(0,1));
	rectangle2 rect = RectMinMax(MinP, MaxP);
	mat4 ScreenSpaceMatrix = ScreenSpaceToVulkanSpace(screen_width, screen_height);
	
	v3 vt1 = (ScreenSpaceMatrix * V4(rect.Min.x, rect.Min.y, 0, 1.0f)).xyz;
	v3 vt2 = (ScreenSpaceMatrix * V4(rect.Max.x, rect.Min.y, 0, 1.0f)).xyz;
	v3 vt3 = (ScreenSpaceMatrix * V4(rect.Min.x, rect.Max.y, 0, 1.0f)).xyz;
	v3 vt4 = (ScreenSpaceMatrix * V4(rect.Max.x, rect.Max.y, 0, 1.0f)).xyz;
	
	vertex_3d Vert1 = {vt1, Color, Glyph.TexCoord_LeftTop};
	vertex_3d Vert2 = {vt2, Color, Glyph.TexCoord_RightTop};
	vertex_3d Vert3 = {vt3, Color, Glyph.TexCoord_LeftBottom};
	vertex_3d Vert4 = {vt4, Color, Glyph.TexCoord_RightBottom};
	
	state->vertices_buffer[state->vertices_count++] = Vert1;
	state->vertices_buffer[state->vertices_count++] = Vert2;
	state->vertices_buffer[state->vertices_count++] = Vert3;
	state->vertices_buffer[state->vertices_count++] = Vert4;
	
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 0;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 3;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	
	++state->quad_count;
}

// Space Dimensions in Pixels (Screen width -- Screen Height)
internal rectangle2
draw_text_quad_screen_space(basic_ui_state *state, u32 screen_width, u32 screen_height, v2 Origin, v2 Dim, v4 Color)
{
	rectangle2 rect = RectCenterDim(Origin, Dim);
	rect = screen_to_vulkan_coords_rect(rect, screen_width, screen_height);
	
	v3 vt1 = V3(rect.Min.x, rect.Min.y, 0);
	v3 vt2 = V3(rect.Max.x, rect.Min.y, 0);
	v3 vt3 = V3(rect.Min.x, rect.Max.y, 0);
	v3 vt4 = V3(rect.Max.x, rect.Max.y, 0);
	
	vertex_3d Vert1 = {vt1, Color, {0, 1.0f}};
	vertex_3d Vert2 = {vt2, Color, {1.0f, 1.0f}};
	vertex_3d Vert3 = {vt3, Color, {0, 0}};
	vertex_3d Vert4 = {vt4, Color, {1.0f, 0}};
	
	state->vertices_buffer[state->vertices_count++] = Vert1;
	state->vertices_buffer[state->vertices_count++] = Vert2;
	state->vertices_buffer[state->vertices_count++] = Vert3;
	state->vertices_buffer[state->vertices_count++] = Vert4;
	
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 0;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 3;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	
	++state->quad_count;
	
	return rect;
}

// Space Dimensions in Camera Space (0.0f -- 1.0f)
internal rectangle2
draw_text_quad_camera_space(basic_ui_state *state, v2 Origin, v2 Dim, v4 Color)
{
	rectangle2 rect = RectCenterDim(Origin, Dim);
	
	mat4 camera_space_matrix = CameraSpaceToVulkanSpace();
	rect = RectMulMat4x4(rect, camera_space_matrix);
	
	v3 vt1 = V3(rect.Min.x, rect.Min.y, 0);
	v3 vt2 = V3(rect.Max.x, rect.Min.y, 0);
	v3 vt3 = V3(rect.Min.x, rect.Max.y, 0);
	v3 vt4 = V3(rect.Max.x, rect.Max.y, 0);
	
	vertex_3d Vert1 = {vt1, Color, {0, 1.0f}};
	vertex_3d Vert2 = {vt2, Color, {1.0f, 1.0f}};
	vertex_3d Vert3 = {vt3, Color, {0, 0}};
	vertex_3d Vert4 = {vt4, Color, {1.0f, 0}};
	
	state->vertices_buffer[state->vertices_count++] = Vert1;
	state->vertices_buffer[state->vertices_count++] = Vert2;
	state->vertices_buffer[state->vertices_count++] = Vert3;
	state->vertices_buffer[state->vertices_count++] = Vert4;
	
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 0;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 3;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	
	++state->quad_count;
	
	return rect;
}