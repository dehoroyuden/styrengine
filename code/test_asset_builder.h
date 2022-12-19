#ifndef TEST_ASSET_BUILDER_H
/*====================================================================
 $File: $
 $Date: $
 $Revision: $
 $Creator: Denis Hoida $
 $Notice: (C) Copyright 2022 by Deden DEHO, Inc. All Rights Reserved. $
======================================================================*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "styr_platform.h"
#include "styr_intrinsics.h"
#include "styr_math.h"
#include "styr_file_formats.h"
#include "styr_obj_importer.h"
#include "win32_styr.h"
#include "styr_shared.h"

// NOTE(Denis): Image and File Importers
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define ONE_PAST_MAX_FONT_CODEPOINT (0x10FFFF + 1)

#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024

global_variable VOID *GlobalFontBits;
global_variable HDC GlobalFontDeviceContext;


struct loaded_bitmap
{
	int32 Width;
	int32 Height;
	int32 Pitch;
	u32 MipLevels;
	void *Memory;
	
	void *Free;
};

struct loaded_font
{
	HFONT Win32Handle;
	TEXTMETRIC TextMetric;
	f32 LineAdvance;
	
	sta_font_glyph *Glyphs;
	f32 *HorizontalAdvance;
	
	u32 MinCodePoint;
	u32 MaxCodePoint;
	
	u32 MaxGlyphCount;
	u32 GlyphCount;
	
	u32 *GlyphIndexFromCodePoint;
	u32 OnePastHighestCodepoint;
};

struct loaded_mesh
{
	u32 VerticesCount;
	u32 IndicesCount;
	
	u32 *Indices;
	vertex_3d *Vertices;
	
	u32 SubObjectsCount;
};

enum asset_type
{
	AssetType_Bitmap,
	AssetType_Font,
	AssetType_FontGlyph,
	AssetType_Mesh,
};

struct asset_source_font
{
	loaded_font *Font;
};

struct asset_source_font_glyph
{
	loaded_font *Font;
	u32 Codepoint;
};

struct asset_source_bitmap
{
	char *FileName;
};

struct asset_source_mesh
{
	char *FileName;
};

struct asset_source
{
	asset_type Type;
	union
	{
		asset_source_bitmap Bitmap;
		asset_source_font Font;
		asset_source_font_glyph Glyph;
		asset_source_mesh Mesh;
	};
};

struct bitmap_asset
{
	char *Filename;
	f32 Alignment[2];
};

#define VERY_LARGE_NUMBER 4096 // NOTE(Denis): 4096 should be enough for enybody
struct game_assets
{
	u32 TagCount;
	sta_tag Tags[VERY_LARGE_NUMBER];
	
	u32 AssetTypeCount;
	sta_asset_type AssetTypes[Asset_Count];
	
	u32 AssetCount;
	asset_source AssetSources[VERY_LARGE_NUMBER];
	sta_asset Assets[VERY_LARGE_NUMBER];
	
	sta_asset_type *DEBUGAssetType;
	u32 AssetIndex;
};


#define TEST_ASSET_BUILDER_H
#endif //TEST_ASSET_BUILDER_H
