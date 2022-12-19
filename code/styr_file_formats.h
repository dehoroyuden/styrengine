#ifndef DENGINE_FILE_FORMATS_H
/* date = January 26th 2022 10:25 pm */
/*====================================================================
 $File: $
 $Date: $
 $Revision: $
 $Creator: Denis Hoida $
 $Notice: (C) Copyright 2022 by Deden DEHO, Inc. All Rights Reserved. $
======================================================================*/

enum asset_font_type
{
	FontType_Default = 0,
	FontType_Debug = 10,
};

enum asset_tag_id
{
	Tag_Smoothness,
	Tag_Flatness,
	Tag_UnicodeCodepoint,
	Tag_FontType,// NOTE(Denis): 0 - Default Game Font, 1 - Debug Font ?
	Tag_TextureType, // NOTE(Denis): 0 - Funny Den Monkey, 1 - Statue, 2 - viking room
	
	Tag_Count,
};

enum asset_type_id
{
	Asset_None,
	
	//
	// NOTE(Denis): Bitmaps! 
	//
	Asset_Texture,
	Asset_Mesh,
	Asset_Font,
	Asset_FontGlyph,
	
	//
	// NOTE(Denis): Sounds!
	//
	
	
	//
	//
	//
	
	Asset_Count,
};

#define STA_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

#pragma pack(push, 1)

struct sta_header
{
#define STA_MAGIC_VALUE STA_CODE('s', 't', 'a', 'f')
	u32 MagicValue;
	
#define STA_VERISON 0
	u32 Version;
	
	u32 TagCount;
	u32 AssetTypeCount;
	u32 AssetCount;
	
	u64 Tags; // sta_tag[TagCount]
	u64 AssetTypes; // sta_asset_type[AssetTypeCount]
	u64 Assets; // sta_asset[AssetCount]
	
};

struct sta_tag
{
	u32 ID;
	f32 Value;
};

struct sta_asset_type
{
	u32 TypeID;
	u32 FirstAssetIndex;
	u32 OnePastLastAssetIndex;
};

struct sta_bitmap
{
	u32 Dim[2];
	f32 AlignPercentage[2];
	u32 MipLevels;
	/*NOTE(Denis): Data is
 
 u32 Pixels[Dim[0]][Dim[1]]
*/
};

struct sta_font_glyph
{
	u32 UnicodeCodePoint;
	bitmap_id BitmapID;
};

struct sta_font
{
	u32 OnePastHighestCodepoint;
	u32 GlyphCount;
	f32 AscenderHeight;
	f32 DescenderHeight;
	f32 ExternalLeading;
	/* NOTE(Denis): Data is

	 sta_font_glyph *CodePoints[GlyphCount];
	r32 *HorizontalAdvance[GlyphCount][GlyphCount];

*/
};

struct sta_mesh
{
	u32 VerticesCount;
	u32 IndicesCount;
};

struct sta_asset
{
	u64 DataOffset;
	u32 FirstTagIndex;
	u32 OnePastLastTagIndex;
	union
	{
		sta_bitmap Bitmap;
		sta_font Font;
		sta_mesh Mesh;
	};
};

#pragma pack(pop)

#define DENGINE_FILE_FORMATS_H
#endif //DENGINE_FILE_FORMATS_H
