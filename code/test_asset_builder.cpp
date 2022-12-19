/*====================================================================
 $File: $
 $Date: $
 $Revision: $
 $Creator: Denis Hoida $
 $Notice: (C) Copyright 2022 by Deden DEHO, Inc. All Rights Reserved. $
======================================================================*/

#include "test_asset_builder.h"

#pragma pack(push, 1)
struct bitmap_header
{
	int32 Width;
	int32 Height;
	int32 TextureChannels;
	uint32 BytesPerPixel;
	uint32 SizeOfBitmap;
	uint32 MipLevels;
};
#pragma pack(pop)

struct entire_file
{
	u32 ContentsSize;
	void *Contents;
};

internal entire_file
ReadEntireFile(char *FileName)
{
	entire_file Result = {};
	
	FILE *In = fopen(FileName, "rb");
	if(In)
	{
		fseek(In, 0, SEEK_END);
		Result.ContentsSize = ftell(In);
		fseek(In, 0, SEEK_SET);
		
		Result.Contents = malloc(Result.ContentsSize);
		fread(Result.Contents, Result.ContentsSize, 1, In);
		fclose(In);
	}
	else
	{
		printf("ERROR: Cannot open file %s. \n", FileName);
	}
	
	return(Result);
}

internal loaded_bitmap
LoadBMP(char *FileName)
{
	loaded_bitmap Result = {};
	
	s32 TextureWidth;
	s32 TextureHeight;
	s32 TextureChannels;
	
	stbi_uc *Pixels = stbi_load(FileName, &TextureWidth, &TextureHeight, &TextureChannels, STBI_rgb_alpha);
	u32 MipLevels = (Log2(Maximum(TextureWidth, TextureHeight))) + 1;
	
	Result.Memory = Pixels;
	Result.Width = TextureWidth;
	Result.Height = TextureHeight;
	Result.MipLevels = MipLevels;
	
	Assert(Result.Height >= 0);
	
	bitmap_header Header = {};
	Header.Width = Result.Width;
	Header.Height = Result.Height;
	Header.BytesPerPixel = 4;
	Header.SizeOfBitmap = Header.BytesPerPixel * (Header.Width + Header.Height);
	Header.MipLevels = MipLevels;
	
	// NOTE(Denis): If you are using this generically for some reason,
	// please remember that BMP files CAN GO IN EITHER DIRECTION and
	// the height will be negative for top-down
	// (Also, therecan be compression, etc.., etc... DON'T think this
	// is complete BMP loading code it isn't!!)
	
	// NOTE(Denis): Byte order in memory is determined by the Header itself,
	// so we have to read out the masks and covert the pixels ourselves.
	
	uint32 RedMask = 0x000000FF;
	uint32 GreenMask = 0x0000FF00;
	uint32 BlueMask = 0x00FF0000;
	uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
	
	bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
	bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
	bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
	bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
	
	Assert(RedScan.Found);
	Assert(GreenScan.Found);
	Assert(BlueScan.Found);
	Assert(AlphaScan.Found);
	
	int32 RedShiftDown = (int32)RedScan.Index;
	int32 GreenShiftDown = (int32)GreenScan.Index;
	int32 BlueShiftDown = (int32)BlueScan.Index;
	int32 AlphaShiftDown = (int32)AlphaScan.Index;
	
	uint32 *SourceDest = (uint32 *)Pixels;
	for(int32 Y = 0;
		Y < Header.Height;
		++Y)
	{
		for(int32 X = 0;
			X < Header.Width;
			++X)
		{
			uint32 C = *SourceDest;
			
			v4 Texel = {
				(real32)((C & RedMask) >> RedShiftDown),
				(real32)((C & GreenMask) >> GreenShiftDown),
				(real32)((C & BlueMask) >> BlueShiftDown),
				(real32)((C & AlphaMask) >> AlphaShiftDown)
			};
			
			Texel = SRGB255ToLinear1(Texel);
#if 1
			Texel.rgb *= Texel.a;
#endif
			Texel = Linear1ToSRGB255(Texel);
			
			//*SourceDest++ = (((uint32)(Texel.a + 0.5f) << 24) | ((uint32)(Texel.r + 0.5f) << 16) | ((uint32)(Texel.g + 0.5f) << 8) | ((uint32)(Texel.b + 0.5f) << 0));
			
			*SourceDest++ = ((((uint32)(Texel.r + 0.5f) << 0) |
							  ((uint32)(Texel.g + 0.5f) << 8) |
							  ((uint32)(Texel.b + 0.5f) << 16) |
							  (uint32)(Texel.a + 0.5f) << 24) );
		}
	}
	
	Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
	
	return(Result);
}

internal loaded_font *
LoadFont(char *FileName, char *FontName, int PixelHeight, bool bold = false)
{
	loaded_font *Font = (loaded_font *)malloc(sizeof(loaded_font));
	
	AddFontResourceExA(FileName, FR_PRIVATE, 0);
	Font->Win32Handle = CreateFontA(PixelHeight, 0, 0, 0, 
									bold ? FW_BOLD : FW_NORMAL, // NOTE(Denis): Weight
									FALSE, // NOTE(Denis): Italic
									FALSE, // TODO(Denis): Underline
									FALSE, // NOTE(Denis): Strikeout
									DEFAULT_CHARSET,
									OUT_DEFAULT_PRECIS,
									CLIP_DEFAULT_PRECIS,
									ANTIALIASED_QUALITY,
									DEFAULT_PITCH|FF_DONTCARE,
									FontName);
	
	SelectObject(GlobalFontDeviceContext, Font->Win32Handle);
	GetTextMetrics(GlobalFontDeviceContext, &Font->TextMetric);
	
	Font->MinCodePoint = INT_MAX;
	Font->MaxCodePoint = 0;
	
	// NOTE(Denis): 5k characters should be more than enough for _anybody_
	Font->MaxGlyphCount = 5000;
	Font->GlyphCount = 0;
	
	u32 GlyphIndexFromCodePointSize = ONE_PAST_MAX_FONT_CODEPOINT*sizeof(u32);
	Font->GlyphIndexFromCodePoint = (u32 *)malloc(GlyphIndexFromCodePointSize);
	memset(Font->GlyphIndexFromCodePoint, 0, GlyphIndexFromCodePointSize);
	
	Font->Glyphs = (sta_font_glyph *)malloc(sizeof(sta_font_glyph)*Font->MaxGlyphCount);
	size_t HorizontalAdvanceSize = sizeof(f32)*Font->MaxGlyphCount*Font->MaxGlyphCount;
	Font->HorizontalAdvance = (f32 *)malloc(HorizontalAdvanceSize);
	memset(Font->HorizontalAdvance, 0, HorizontalAdvanceSize);
	
	Font->OnePastHighestCodepoint = 0;
	
	// NOTE(Denis): Reserve space for the null glyph
	Font->GlyphCount = 1;
	Font->Glyphs[0].UnicodeCodePoint = 0;
	Font->Glyphs[0].BitmapID.Value = 0;
	
	return(Font);
}

internal void
FinalizeFontKerning(loaded_font *Font)
{
	SelectObject(GlobalFontDeviceContext, Font->Win32Handle);
	
	DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
	KERNINGPAIR *KerningPairs = (KERNINGPAIR *)malloc(KerningPairCount*sizeof(KERNINGPAIR));
	GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);
	
	for(DWORD KerningPairIndex = 0;
		KerningPairIndex < KerningPairCount;
		++KerningPairIndex)
	{
		KERNINGPAIR *Pair = KerningPairs + KerningPairIndex;
		if((Pair->wFirst < ONE_PAST_MAX_FONT_CODEPOINT) &&
		   (Pair->wSecond < ONE_PAST_MAX_FONT_CODEPOINT))
		{
			u32 First = Font->GlyphIndexFromCodePoint[Pair->wFirst];
			u32 Second = Font->GlyphIndexFromCodePoint[Pair->wSecond];
			if((First != 0) && (Second != 0))
			{
				Font->HorizontalAdvance[First*Font->MaxGlyphCount + Second] += (f32)Pair->iKernAmount;
			}
		}
	}
	
	free(KerningPairs);
}

internal void
FreeFont(loaded_font *Font)
{
	if(Font)
	{
		DeleteObject(Font->Win32Handle);
		free(Font->Glyphs);
		free(Font->HorizontalAdvance);
		free(Font->GlyphIndexFromCodePoint);
		free(Font);
	}
}

internal void
InitializeFontDC(void)
{
	GlobalFontDeviceContext = CreateCompatibleDC(GetDC(0));
	
	BITMAPINFO Info = {};
	Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
	Info.bmiHeader.biWidth = MAX_FONT_WIDTH;
	Info.bmiHeader.biHeight = MAX_FONT_HEIGHT;
	Info.bmiHeader.biPlanes = 1;
	Info.bmiHeader.biBitCount = 32;
	Info.bmiHeader.biCompression = BI_RGB;
	Info.bmiHeader.biSizeImage = 0;
	Info.bmiHeader.biXPelsPerMeter = 0;
	Info.bmiHeader.biYPelsPerMeter = 0;
	Info.bmiHeader.biClrUsed = 0;
	Info.bmiHeader.biClrImportant = 0;
	HBITMAP Bitmap = CreateDIBSection(GlobalFontDeviceContext, &Info, DIB_RGB_COLORS, &GlobalFontBits, 0, 0);
	SelectObject(GlobalFontDeviceContext, Bitmap);
	SetBkColor(GlobalFontDeviceContext, RGB(0, 0, 0));
}

internal loaded_bitmap
LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, sta_asset *Asset)
{
	loaded_bitmap Result = {};
	
	u32 GlyphIndex = Font->GlyphIndexFromCodePoint[CodePoint];
	
	SelectObject(GlobalFontDeviceContext, Font->Win32Handle);
	
	memset(GlobalFontBits, 0x00, MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32));
	
	wchar_t CheesePoint = (wchar_t)CodePoint;
	
	SIZE Size;
	GetTextExtentPoint32W(GlobalFontDeviceContext, &CheesePoint, 1, &Size);
	
	int PreStepX = 128;
	
	int BoundWidth = Size.cx + 2*PreStepX;
	if(BoundWidth > MAX_FONT_WIDTH)
	{
		BoundWidth = MAX_FONT_WIDTH;
	}
	int BoundHeight = Size.cy;
	if(BoundHeight > MAX_FONT_HEIGHT)
	{
		BoundHeight = MAX_FONT_HEIGHT;
	}
	
	//PatBlt(DeviceContext, 0, 0, Width, Height, BLACKNESS);
	SetTextColor(GlobalFontDeviceContext, RGB(255, 255 ,255));
	TextOutW(GlobalFontDeviceContext, PreStepX, 0, &CheesePoint,1);
	
	s32 MinX = 10000;
	s32 MinY = 10000;
	s32 MaxX = -10000;
	s32 MaxY = -10000;
	
	u32 *Row = (u32 *)GlobalFontBits + (MAX_FONT_HEIGHT - 1)*MAX_FONT_WIDTH;
	for(s32 Y = 0;
		Y < BoundHeight;
		++Y)
	{
		u32 *Pixel = Row;
		for(s32 X = 0;
			X < BoundWidth;
			++X)
		{
			if(*Pixel != 0)
			{
				if(MinX > X)
				{
					MinX = X;
				}
				
				if(MinY > Y)
				{
					MinY = Y;
				}
				
				if(MaxX < X)
				{
					MaxX = X;
				}
				
				if(MaxY < Y)
				{
					MaxY = Y;
				}
			}
			++Pixel;
		}
		Row -= MAX_FONT_WIDTH;
	}
	
	f32 KerningChange = 0.0f;
	
	if(MinX <= MaxX)
	{
		int Width = (MaxX - MinX) + 1;
		int Height = (MaxY - MinY) + 1;
		
		Result.Width = Width + 2;
		Result.Height = Height + 2;
		Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
		Result.Memory = malloc(Result.Height*Result.Pitch);
		Result.Free = Result.Memory;
		
		memset(Result.Memory, 0, Result.Height*Result.Pitch);
		
		u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch;
		u32 *SourceRow = (u32 *)GlobalFontBits + (MAX_FONT_HEIGHT - 1 - MinY)*MAX_FONT_WIDTH;
		for(s32 Y = MinY;
			Y <= MaxY;
			++Y)
		{
			u32 *Source = (u32 *)SourceRow + MinX;
			u32 *Dest = (u32 *)DestRow + 1;
			for(s32 X = MinX;
				X <= MaxX;
				++X)
			{
				
				u32 Pixel = *Source;
				f32 Gray = (f32)(Pixel & 0xFF);
				v4 Texel = {255.0f, 255.0f, 255.0f, Gray};
				
				Texel = SRGB255ToLinear1(Texel);
				Texel.rgb *= Texel.a;
				Texel = Linear1ToSRGB255(Texel);
				
				*Dest++ = (((uint32)(Texel.a + 0.5f) << 24) |
						   ((uint32)(Texel.r + 0.5f) << 16) |
						   ((uint32)(Texel.g + 0.5f) << 8) |
						   ((uint32)(Texel.b + 0.5f) << 0));
				
				++Source;
			}
			
			DestRow -= Result.Pitch;
			SourceRow -= MAX_FONT_WIDTH;
		}
		
		Asset->Bitmap.AlignPercentage[0] = (1.0f) / (f32)Result.Width;
		Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (f32)Result.Height;
		
		KerningChange = (f32)(MinX - PreStepX);
	}
	
	INT ThisWidth;
	GetCharWidth32W(GlobalFontDeviceContext, CodePoint, CodePoint, &ThisWidth);
	f32 CharAdvance = (f32)ThisWidth;
	
	for(u32 OtherGlyphIndex = 0;
		OtherGlyphIndex < Font->MaxGlyphCount;
		++OtherGlyphIndex)
	{
		Font->HorizontalAdvance[GlyphIndex*Font->MaxGlyphCount + OtherGlyphIndex] += CharAdvance - KerningChange;
		if(OtherGlyphIndex != 0)
		{
			Font->HorizontalAdvance[OtherGlyphIndex*Font->MaxGlyphCount + GlyphIndex] += KerningChange;
		}
	}
	
	return(Result);
}

internal u32
FindVsCount(char *Symbols)
{
	u32 Result = 0;
	if(Symbols[0] == 'v' &&
	   (IsWhitespace(Symbols[1])))
	{
		return FindVsCount(++Symbols) + 1;
	} 
	else if(Symbols[0] != '\0')
	{
		return FindVsCount(++Symbols);
	}
	
	return 0;
}

struct HashVertex
{
	u32 HashValue;
	u32 Count;
	u32 Index;
};

// TODO(Denis): Write better hash function
inline u32 
CreateHashValue(vertex_3d Vert)
{
	u32 Result = 0;
	char Buffer[64];
	s32 P = 31;
	u32 M = (u32)(1e9 + 9);
	u32 P_pow = 1;
	
	_snprintf_s(Buffer, sizeof(Buffer), "%.04f%.04f%.04f%.04f%.04f",
				Vert.Pos.x, Vert.Pos.y, Vert.Pos.z, Vert.TexCoord.x, Vert.TexCoord.y);
	
	for(u32 CharacterIndex = 0;
		CharacterIndex < ArrayCount(Buffer);
		++CharacterIndex)
	{
		Result = (Result + (Buffer[CharacterIndex] - 'a' + 1) * P_pow) % M;
		P_pow = (P_pow * P) % M;
	}
	
	return Result;
}

inline u32
FindVertexByHash(u32 HashValue, HashVertex *HashVertices, u32 HashVerticesCount)
{
	u32 Result = 0;
	
	for(u32 Index = 0;
		Index < HashVerticesCount;
		++Index)
	{
		if(HashVertices[Index].HashValue == HashValue)
		{
			Result = Index;
			break;
		}
	}
	
	return Result;
}

inline u32
FindVerticesCountByHashValue(u32 HashValue, HashVertex *HashVertices, u32 HashVerticesCount)
{
	u32 Result = 0;
	
	for(u32 Index = 0;
		Index < HashVerticesCount;
		++Index)
	{
		if(HashVertices[Index].HashValue == HashValue)
		{
			++Result;
		}
	}
	
	return Result;
}

inline u32
FindUniqueVerticesCount(HashVertex *HashVertices, u32 HashVerticesCount)
{
	u32 Result = 0;
	
	for(u32 Index = 0;
		Index < HashVerticesCount;
		++Index)
	{
		u32 VertexRepeatedCount = FindVerticesCountByHashValue(HashVertices[Index].HashValue, HashVertices, HashVerticesCount);
		if(VertexRepeatedCount == 1)
		{
			++Result;
		}
	}
	
	return Result;
}

internal loaded_mesh
LoadMesh(char *FileName)
{
	loaded_mesh Result = {};
	
	ObjectData Data = CreateTheObject(FileName);
	retval vertexdata = GetVertexArray(Data.ObjectData, 0);
	retval texturedata = GetTextureArray(Data.ObjectData, 0);
	
	u32 VertexIndicesCount = vertexdata.count_o_vals / 3;
	
	vertex_3d *Vertices = (vertex_3d *)malloc(VertexIndicesCount * sizeof(vertex_3d));
	u32 *Indices = (u32 *)malloc(VertexIndicesCount * sizeof(u32));
	
	// NOTE(Denis): Hash all the values from vertices array into hashes array
	HashVertex* Hashes = (HashVertex*)malloc(VertexIndicesCount * sizeof(HashVertex));
	memset((void *)Hashes, 0, VertexIndicesCount * sizeof(HashVertex));
	
	VertexIndicesCount = 0;
	
	u32 TexCoordIndex = 0;
	for (int i = 0; i < vertexdata.count_o_vals / 3; i++) 
	{
		vertex_3d Vert = {};
		Vert.Pos =
		{
			vertexdata.values[3 * i + 0],
			vertexdata.values[3 * i + 1],
			vertexdata.values[3 * i + 2]
		};
		
		Vert.TexCoord =
		{
			texturedata.values[2 * TexCoordIndex + 0],
			1.0f - texturedata.values[2 * TexCoordIndex + 1]
		};
		
		Vert.Color = { 1.0f, 1.0f, 1.0f };
		
		Hashes[VertexIndicesCount].HashValue = CreateHashValue(Vert);
		++VertexIndicesCount;
		++TexCoordIndex;
	}
	
	TexCoordIndex = 0;
	u32 VerticesCount = VertexIndicesCount;
	u32 UniqueIndex = 0;
	u32 VertexIndex = 0;
	
	for (int i = 0; i < vertexdata.count_o_vals / 3; i++) 
	{
		vertex_3d Vert = {};
		{
			Vert.Pos =
			{
				vertexdata.values[3 * i + 0],
				vertexdata.values[3 * i + 1],
				vertexdata.values[3 * i + 2]
			};
			
			Vert.TexCoord =
			{
				texturedata.values[2 * TexCoordIndex + 0],
				1.0f - texturedata.values[2 * TexCoordIndex + 1]
			};
			
			Vert.Color = { 1.0f, 1.0f, 1.0f };
		}
		
		u32 HashValue = CreateHashValue(Vert);
		u32 HashValueIndex = FindVertexByHash(HashValue, Hashes, VertexIndicesCount);
		u32 HashValueVertexCount = Hashes[HashValueIndex].Count;
		
		if (HashValueVertexCount == 0)
		{
			Vertices[UniqueIndex] = Vert;
			Hashes[HashValueIndex].Index = UniqueIndex;
			++Hashes[HashValueIndex].Count;
			++UniqueIndex;
		}
		
		Indices[VertexIndex] = Hashes[HashValueIndex].Index;
		++VertexIndex;
		++TexCoordIndex;
	}
	
	vertex_3d* ActualVerticesInUse = Vertices;
	Result.VerticesCount = UniqueIndex;
	Result.Vertices = ActualVerticesInUse;
	Result.Indices = Indices;
	
	Result.IndicesCount = VertexIndicesCount;
	
	return Result;
}

internal void
BeginAssetType(game_assets *Assets, asset_type_id TypeID)
{
	Assert(Assets->DEBUGAssetType == 0);
	
	Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
	Assets->DEBUGAssetType->TypeID = TypeID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount;
	Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

struct added_asset
{
	u32 ID;
	sta_asset *STA;
	asset_source *Source;
};

internal added_asset
AddAsset(game_assets *Assets)
{
	Assert(Assets->DEBUGAssetType);
	Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));
	
	u32 Index = Assets->DEBUGAssetType->OnePastLastAssetIndex++;
	asset_source *Source = Assets->AssetSources + Index;
	sta_asset *STA = Assets->Assets + Index;
	STA->FirstTagIndex = Assets->TagCount;
	STA->OnePastLastTagIndex = STA->FirstTagIndex;
	
	Assets->AssetIndex = Index;
	
	added_asset Result;
	Result.ID = Index;
	Result.STA = STA;
	Result.Source = Source;
	return(Result);
}

internal bitmap_id
AddBitmapAsset(game_assets *Assets, char *FileName, f32 AlignPercentageX = 0.5f,f32 AlignPercentageY = 0.5f)
{
	added_asset Asset = AddAsset(Assets);
	Asset.STA->Bitmap.AlignPercentage[0] = AlignPercentageX;
	Asset.STA->Bitmap.AlignPercentage[1] = AlignPercentageY;
	Asset.Source->Type = AssetType_Bitmap;
	Asset.Source->Bitmap.FileName = FileName;
	
	bitmap_id Result = {Asset.ID};
	return(Result);
}

internal mesh_id
AddMeshAsset(game_assets *Assets, char *FileName)
{
	added_asset Asset = AddAsset(Assets);
	Asset.Source->Type = AssetType_Mesh;
	Asset.Source->Bitmap.FileName = FileName;
	
	mesh_id Result = {Asset.ID};
	return(Result);
}

internal bitmap_id
AddCharacterAsset(game_assets *Assets, loaded_font *Font, u32 Codepoint)
{
	added_asset Asset = AddAsset(Assets);
	Asset.STA->Bitmap.AlignPercentage[0] = 0.0f; // NOTE(Denis): Set later by extraction
	Asset.STA->Bitmap.AlignPercentage[1] = 0.0f; // NOTE(Denis): Set later by extraction
	Asset.Source->Type = AssetType_FontGlyph;
	Asset.Source->Glyph.Font = Font;
	Asset.Source->Glyph.Codepoint = Codepoint;
	
	bitmap_id Result = {Asset.ID};
	
	Assert(Font->GlyphCount < Font->MaxGlyphCount);
	u32 GlyphIndex = Font->GlyphCount++;
	sta_font_glyph *Glyph = Font->Glyphs + GlyphIndex;
	Glyph->UnicodeCodePoint = Codepoint;
	Glyph->BitmapID = Result;
	Font->GlyphIndexFromCodePoint[Codepoint] = GlyphIndex;
	
	if(Font->OnePastHighestCodepoint <= Codepoint)
	{
		Font->OnePastHighestCodepoint = Codepoint + 1;
	}
	
	return(Result);
}

internal font_id
AddFontAsset(game_assets *Assets, loaded_font *Font)
{
	added_asset Asset = AddAsset(Assets);
	Asset.STA->Font.OnePastHighestCodepoint = Font->OnePastHighestCodepoint;
	Asset.STA->Font.GlyphCount = Font->GlyphCount;
	Asset.STA->Font.AscenderHeight = (f32)Font->TextMetric.tmAscent;
	Asset.STA->Font.DescenderHeight = (f32)Font->TextMetric.tmDescent;
	Asset.STA->Font.ExternalLeading = (f32)Font->TextMetric.tmExternalLeading;
	Asset.Source->Type = AssetType_Font;
	Asset.Source->Font.Font = Font;
	
	font_id Result = {Asset.ID};
	return(Result);
}

internal void
AddTag(game_assets *Assets, asset_tag_id ID, real32 Value)
{
	Assert(Assets->AssetIndex);
	
	sta_asset *STA = Assets->Assets + Assets->AssetIndex;
	++STA->OnePastLastTagIndex;
	sta_tag *Tag = Assets->Tags + Assets->TagCount++;
	
	Tag->ID = ID;
	Tag->Value = Value;
	
}

internal void
EndAssetType(game_assets *Assets)
{
	Assert(Assets->DEBUGAssetType);
	Assets->AssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
	
	Assets->DEBUGAssetType =  0;
	Assets->AssetIndex = 0;
}

internal void
WriteSTA(game_assets *Assets, char *FileName)
{
	FILE *Out = fopen(FileName, "wb"); // Dengine asset file format
	if(Out)
	{
		sta_header Header = {};
		Header.MagicValue = STA_MAGIC_VALUE;
		Header.Version = STA_VERISON;
		Header.TagCount = Assets->TagCount;
		Header.AssetTypeCount = Asset_Count; // TODO(Denis): Do we really want to do this? Sparseness!
		Header.AssetCount = Assets->AssetCount;
		
		u32 TagArraySize = Header.TagCount*sizeof(sta_tag);
		u32 AssetTypeArraySize = Header.AssetTypeCount*sizeof(sta_asset_type);
		u32 AssetArraySize = Header.AssetCount * sizeof(sta_asset);
		
		Header.Tags = sizeof(Header);
		Header.AssetTypes = Header.Tags + TagArraySize;
		Header.Assets = Header.AssetTypes + AssetTypeArraySize;
		
		fwrite(&Header, sizeof(Header), 1,Out);
		fwrite(Assets->Tags,TagArraySize,1,Out);
		fwrite(Assets->AssetTypes,AssetTypeArraySize,1,Out);
		fseek(Out, AssetArraySize, SEEK_CUR);
		for(u32 AssetIndex = 1;
			AssetIndex < Header.AssetCount;
			++AssetIndex)
		{
			asset_source *Source = Assets->AssetSources + AssetIndex;
			sta_asset *Dest = Assets->Assets + AssetIndex;
			
			Dest->DataOffset = ftell(Out);
			
			if(Source->Type == AssetType_Font)
			{
				loaded_font *Font = Source->Font.Font;
				
				FinalizeFontKerning(Font);
				
				u32 GlyphSize = Font->GlyphCount*sizeof(sta_font_glyph);
				fwrite(Font->Glyphs, GlyphSize, 1, Out);
				
				u8 *HorizontalAdvance = (u8 *)Font->HorizontalAdvance;
				for(u32 GlyphIndex = 0;
					GlyphIndex < Font->GlyphCount;
					++GlyphIndex)
				{
					u32 HorizontalAdvanceSliceSize = sizeof(f32)*Font->GlyphCount;
					fwrite(HorizontalAdvance, HorizontalAdvanceSliceSize, 1, Out);
					HorizontalAdvance += sizeof(f32)*Font->MaxGlyphCount;
				}
			}
			else if(Source->Type == AssetType_Mesh)
			{
				loaded_mesh Mesh = LoadMesh(Source->Mesh.FileName);
				Dest->Mesh.VerticesCount = Mesh.VerticesCount;
				Dest->Mesh.IndicesCount = Mesh.IndicesCount;
				
				fwrite(Mesh.Vertices, Mesh.VerticesCount*sizeof(vertex_3d), 1, Out);
				fwrite(Mesh.Indices, Mesh.IndicesCount*sizeof(u32), 1, Out);
			}
			else
			{
				loaded_bitmap Bitmap;
				if(Source->Type == AssetType_FontGlyph)
				{
					Bitmap = LoadGlyphBitmap(Source->Glyph.Font, Source->Glyph.Codepoint, Dest);
				}
				else
				{
					Bitmap = LoadBMP(Source->Bitmap.FileName);
				}
				
				Dest->Bitmap.Dim[0] = Bitmap.Width;
				Dest->Bitmap.Dim[1] = Bitmap.Height;
				Dest->Bitmap.MipLevels = Bitmap.MipLevels;
				
				Assert((Bitmap.Width*4) == Bitmap.Pitch);
				fwrite(Bitmap.Memory, Bitmap.Width*Bitmap.Height*4, 1, Out);
				
				free(Bitmap.Free);
			}
		}
		fseek(Out, (u32)Header.Assets, SEEK_SET);
		fwrite(Assets->Assets,AssetArraySize,1,Out);
		
		fclose(Out);
	}
	else
	{
		printf("ERROR: Couldn't open file :(\n");
	}
	
}

internal void
Initialize(game_assets *Assets)
{
	Assets->TagCount = 1;
	Assets->AssetCount = 1;
	Assets->DEBUGAssetType = 0;
	Assets->AssetIndex = 0;
	
	Assets->AssetTypeCount = Asset_Count;
	memset(Assets->AssetTypes, 0, sizeof(Assets->AssetTypes));
}

internal void
WriteFonts(void)
{
	game_assets Assets_ = {};
	game_assets *Assets = &Assets_;
	Initialize(Assets);
	
	loaded_font *Fonts[] = 
	{
		LoadFont("c:/Windows/Fonts/arial.ttf", "Arial", 20, true),
		LoadFont("c:/Windows/Fonts/LiberationMono-Regular.ttf", "Liberation Mono", 20),
		LoadFont("c:/Windows/Fonts/ShareTechMono.ttf", "Share Tech Mono", 20),
	};
	
	BeginAssetType(Assets, Asset_FontGlyph);
	for(u32 FontIndex = 0;
		FontIndex < ArrayCount(Fonts);
		++FontIndex)
	{
		loaded_font *Font = Fonts[FontIndex];
		
		AddCharacterAsset(Assets, Font, ' ');
		for(u32 Character = '!';
			Character <= '~';
			++Character)
		{
			AddCharacterAsset(Assets,Font, Character);
		}
		
		// NOTE(Denis): Kanji Owl!!
		AddCharacterAsset(Assets, Font, 0x5c0f);
		AddCharacterAsset(Assets, Font, 0x8033);
		AddCharacterAsset(Assets, Font, 0x6728);
		AddCharacterAsset(Assets, Font, 0x514e);
		AddCharacterAsset(Assets, Font, 0x0414);
	}
	EndAssetType(Assets);
	
	// TODO(Denis): This is kinda janky, because it means you have to get this 
	// order right always!
	BeginAssetType(Assets, Asset_Font);
	AddFontAsset(Assets, Fonts[0]);
	AddTag(Assets, Tag_FontType, FontType_Default);
	AddFontAsset(Assets, Fonts[1]);
	AddTag(Assets, Tag_FontType, FontType_Debug);
	AddFontAsset(Assets, Fonts[2]);
	AddTag(Assets, Tag_FontType, 5);
	EndAssetType(Assets);
	
	WriteSTA(Assets, "testfonts.sta");
}


internal void
WriteTextures(void)
{
	game_assets Assets_ = {};
	game_assets *Assets = &Assets_;
	Initialize(Assets);
	
	BeginAssetType(Assets, Asset_Texture);
	AddBitmapAsset(Assets,"textures/ava_20.jpg");
	AddTag(Assets, Tag_TextureType, 0);
	AddBitmapAsset(Assets,"textures/landcruiser_d.png");
	AddTag(Assets, Tag_TextureType, 3);
	AddBitmapAsset(Assets,"textures/texture.jpg");
	AddTag(Assets, Tag_TextureType, 1);
	AddBitmapAsset(Assets,"textures/viking_room.png");
	AddTag(Assets, Tag_TextureType, 2);
	EndAssetType(Assets);
	WriteSTA(Assets, "test_data_01.sta");
}

internal void
WriteMeshes(void)
{
	game_assets Assets_ = {};
	game_assets *Assets = &Assets_;
	Initialize(Assets);
	
	BeginAssetType(Assets, Asset_Mesh);
	AddMeshAsset(Assets,"models/landcruiser.obj");
	AddTag(Assets, Tag_TextureType, 0);
	AddMeshAsset(Assets,"models/viking_room.obj");
	AddTag(Assets, Tag_TextureType, 1);
	AddMeshAsset(Assets,"models/cube.obj");
	AddTag(Assets, Tag_TextureType, 2);
	AddMeshAsset(Assets,"models/miku.obj");
	AddTag(Assets, Tag_TextureType, 3);
	AddMeshAsset(Assets,"models/susanna.obj");
	AddTag(Assets, Tag_TextureType, 4);
	EndAssetType(Assets);
	WriteSTA(Assets, "test_meshes.sta");
}

int main(int ArgCount, char **Args)
{
	InitializeFontDC();
	
	WriteFonts();
	WriteTextures();
	WriteMeshes();
}