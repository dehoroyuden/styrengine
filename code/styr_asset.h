/* date = January 11th 2022 11:14 pm */
#ifndef STYR_ASSET_H

#include "styr_file_formats.h"

struct loaded_bitmap
{
	void *Memory;
	f32 WidthOverHeight;// NOTE(Denis): Scale value that says : Dimentionless Ratio 
	s32 Width;
	s32 Height;
	// TODO(Denis): Get rid of pitch!
	s32 Pitch;
	u32 MipLevels;
	v2 AlignPercentage;
	
	void *TextureHandle;
};

struct loaded_font
{
	sta_font_glyph *Glyphs;
	bitmap_id *CodePoints;
	f32 *HorizontalAdvance;
	u32 BitmapIDOffset;
	u16 *UnicodeMap;
};

struct loaded_mesh
{
	u32 VerticesCount;
	u32 IndicesCount;
	
	u32 *Indices;
	vertex_3d *Vertices;
	v3 *Normals;
	
	u32 SubObjectsCount;
};

// TODO(Denis): Streamline this, by using header pointer as an indicator of unloaded status?
enum asset_state
{
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded,
};

enum asset_header_type
{
	AssetType_None,
	AssetType_Font,
	AssetType_Bitmap,
	AssetType_Mesh,
};

struct asset_memory_header
{
	asset_memory_header *Next;
	asset_memory_header *Prev;
	
	u32 AssetType;
	u32 AssetIndex;
	u32 TotalSize;
	u32 GenerationID;
	union
	{
		loaded_bitmap Bitmap;
		loaded_font Font;
		loaded_mesh Mesh;
	};
};

struct asset
{
	u32 State;
	asset_memory_header *Header;
	
	sta_asset STA;
	u32 FileIndex;
};

struct asset_bitmap_info
{
	char *FileName;
	v2 AlignPercentage;
};

struct asset_tag
{
	uint32 ID; // NOTE(Denis): Tag ID
	real32 Value;
};

struct asset_vector
{
	real32 E[Tag_Count];
};

struct asset_type
{
	uint32 FirstAssetIndex;
	uint32 OnePastLastAssetIndex;
};

struct asset_group
{
	uint32 FirstTagIndex;
	uint32 OnePastLastTagIndex;
};

struct asset_file
{
	platform_file_handle Handle;
	
	// TODO(Denis): If we ever do thread stacks, 
	// AssetTypeArray doesn't actually need to be kept here probably.
	sta_header Header;
	sta_asset_type *AssetTypeArray;
	
	u32 TagBase;
	u32 FontBitmapIDOffset;
};

enum asset_memory_block_flags
{
	AssetMemory_Used = 0x1,
};

struct asset_memory_block
{
	asset_memory_block *Prev;
	asset_memory_block *Next;
	u64 Flags;
	memory_index Size;
};

struct game_assets
{
	u32 NextGenerationID;
	
	// TODO(Denis): Not thrilled about this back-pointer
	asset_memory_block MemorySentinel;
	
	asset_memory_header LoadedAssetSentinel;
	
	real32 TagRange[Tag_Count];
	
	u32 FileCount;
	asset_file *Files;
	
	uint32 TagCount;
	sta_tag *Tags;
	
	uint32 AssetCount;
	asset *Assets;
	
	asset_type AssetTypes[Asset_Count];
	
	u32 InFlightGenerationCount;
	u32 InFlightGenerations[16];
};

inline void
InsertAssetHeaderAtFront(game_assets *Assets, asset_memory_header *Header)
{
	asset_memory_header *Sentinel = &Assets->LoadedAssetSentinel;
	
	Header->Prev = Sentinel;
	Header->Next = Sentinel->Next;
	
	Header->Next->Prev = Header;
	Header->Prev->Next = Header;
}

inline void
RemoveAssetHeaderFromList(asset_memory_header *Header)
{
	Header->Prev->Next = Header->Next;
	Header->Next->Prev = Header->Prev;
	
	Header->Prev = Header->Next = 0;
}

inline asset_memory_header *GetAsset(game_assets *Assets, u32 ID)
{
	Assert(ID <= Assets->AssetCount);
	asset *Asset = Assets->Assets + ID;
	
	asset_memory_header *Result = 0;
	
	if(Asset->State == AssetState_Loaded)
	{
		Result = Asset->Header;
		RemoveAssetHeaderFromList(Result);
		InsertAssetHeaderAtFront(Assets, Result);
	}
	
	return(Result);
}

inline loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID)
{
	asset_memory_header *Header = GetAsset(Assets, ID.Value);
	
	loaded_bitmap *Result = Header ? &Header->Bitmap : 0;
	
	return(Result);
}

inline loaded_mesh *
GetMesh(game_assets *Assets, mesh_id ID)
{
	asset_memory_header *Header = GetAsset(Assets, ID.Value);
	
	loaded_mesh *Result = Header ? &Header->Mesh : 0;
	
	return(Result);
}

inline sta_bitmap
GetBitmapInfo(game_assets Assets, bitmap_id ID)
{
	Assert(ID.Value <= Assets.AssetCount);
	sta_bitmap Result =  Assets.Assets[ID.Value].STA.Bitmap;
	
	return(Result);
}

inline loaded_font *
GetFont(game_assets *Assets, font_id ID)
{
	asset_memory_header *Header = GetAsset(Assets, ID.Value);
	
	loaded_font *Result = Header ? &Header->Font : 0;
	
	return(Result);
}

inline sta_font *
GetFontInfo(game_assets *Assets, font_id ID)
{
	Assert(ID.Value <= Assets->AssetCount);
	sta_font *Result =  &Assets->Assets[ID.Value].STA.Font;
	
	return(Result);
}

inline bool32
IsValid(bitmap_id ID)
{
	bool32 Result  = (ID.Value != 0);
	
	return(Result);
}

inline bool32
IsValid(sound_id ID)
{
	bool32 Result  = (ID.Value != 0);
	
	return(Result);
}

internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
internal void LoadFont(game_assets *Assets, font_id ID);

inline u32 
BeginGeneration(game_assets *Assets)
{
	Assert(Assets->InFlightGenerationCount < ArrayCount(Assets->InFlightGenerations));
	u32 Result = Assets->NextGenerationID++;
	Assets->InFlightGenerations[Assets->InFlightGenerationCount++] = Result;
	
	return(Result);
}

inline void
EndGeneration(game_assets *Assets, u32 GenerationID)
{
	for(u32 Index = 0;
		Index < Assets->InFlightGenerationCount;
		++Index)
	{
		if(Assets->InFlightGenerations[Index] == GenerationID)
		{
			Assets->InFlightGenerations[Index] = Assets->InFlightGenerations[--Assets->InFlightGenerationCount];
			break;
		}
	}
}

#define STYR_ASSET_H
#endif //STYR_ASSET_H
