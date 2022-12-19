/*====================================================================
 $File: $
 $Date: $
 $Revision: $
 $Creator: Denis Hoida $
 $Notice: (C) Copyright 2021 by Deden DEHO, Inc. All Rights Reserved. $
======================================================================*/

enum finalize_asset_operation
{
	FinalizeAsset_None,
	FinalizeAsset_Font,
	FinalizeAsset_Bitmap,
};

struct load_asset
{
	asset *Asset;
	
	platform_file_handle *Handle;
	u64 Offset;
	u64 Size;
	void *Destination;
	
	finalize_asset_operation FinalizeOperation;
	u32 FinalState;
};

internal void 
LoadAssetDirectly(load_asset *LoadAsset)
{
	Platform.ReadDataFromFile(LoadAsset->Handle, LoadAsset->Offset, LoadAsset->Size, LoadAsset->Destination);
	if(PlatformNoFileErrors(LoadAsset->Handle))
	{
		switch(LoadAsset->FinalizeOperation)
		{
			case FinalizeAsset_None:
			{
				// NOTE(Denis): Nothing to do.
			} break;
			
			case FinalizeAsset_Font:
			{
				loaded_font *Font = &LoadAsset->Asset->Header->Font;
				sta_font *STA = &LoadAsset->Asset->STA.Font;
				for(u32 GlyphIndex = 1;
					GlyphIndex < STA->GlyphCount;
					++GlyphIndex)
				{
					sta_font_glyph *Glyph = Font->Glyphs + GlyphIndex;
					
					Assert(Glyph->UnicodeCodePoint < STA->OnePastHighestCodepoint);
					Assert((u16)GlyphIndex == GlyphIndex);
					Font->UnicodeMap[Glyph->UnicodeCodePoint] = (u16)GlyphIndex;
				}
			} break;
			
			case FinalizeAsset_Bitmap:
			{
				loaded_bitmap *Bitmap = &LoadAsset->Asset->Header->Bitmap;
				//Bitmap->TextureHandle = Platform.AllocateTexture(Bitmap->Width, Bitmap->Height, Bitmap->Memory);
			} break;
		}
	}
	
	if(!PlatformNoFileErrors(LoadAsset->Handle))
	{
		ZeroSize(LoadAsset->Size, LoadAsset->Destination);
	}
	
	LoadAsset->Asset->State = LoadAsset->FinalState;
}

internal void
LoadAssets(void *Data)
{
	load_asset *LoadAsset = (load_asset *)Data;
	LoadAssetDirectly(LoadAsset);
}

inline asset_file *
GetFile(game_assets *Assets, u32 FileIndex)
{
	Assert(FileIndex < Assets->FileCount);
	asset_file *Result = Assets->Files + FileIndex;
	
	return(Result);
}

inline platform_file_handle *
GetFileHandleFor(game_assets *Assets, u32 FileIndex)
{
	Assert(FileIndex < Assets->FileCount);
	
	platform_file_handle *Result = &GetFile(Assets, FileIndex)->Handle;
	
	return(Result);
}

internal asset_memory_block *
InsertBlock(asset_memory_block *Prev, u64 Size, void *Memory)
{
	Assert(Size > sizeof(asset_memory_block));
	asset_memory_block *Block = (asset_memory_block *)Memory;
	Block->Flags = 0;
	Block->Size = Size - sizeof(asset_memory_block);
	Block->Prev = Prev;
	Block->Next = Prev->Next;
	Block->Prev->Next = Block;
	Block->Next->Prev = Block;
	return(Block);
}

internal asset_memory_block *
FindBlockForSize(game_assets *Assets, memory_index Size)
{
	asset_memory_block *Result = 0;
	
	// TODO(Denis): This probably will need to be accelerated in the
	// future as the resident asset count grows.
	
	// TODO(Denis): Best match block!
	for(asset_memory_block *Block = Assets->MemorySentinel.Next;
		Block != &Assets->MemorySentinel;
		Block = Block->Next)
	{
		if(!(Block->Flags & AssetMemory_Used))
		{
			if(Block->Size >= Size)
			{
				Result = Block;
				break;
			}
		}
	}
	
	return(Result);
}

internal b32
MergeIfPossible(game_assets *Assets, asset_memory_block *First, asset_memory_block *Second)
{
	b32 Result = false;
	
	if((First != &Assets->MemorySentinel) &&
	   (Second != &Assets->MemorySentinel))
	{
		if(!(First->Flags & AssetMemory_Used) &&
		   !(Second->Flags & AssetMemory_Used))
		{
			u8 *ExpectedSecond = (u8 *)First + sizeof(asset_memory_block) + First->Size;
			if((u8 *)Second == ExpectedSecond)
			{
				Second->Next->Prev = Second->Prev;
				Second->Prev->Next = Second->Next;
				
				First->Size += sizeof(asset_memory_block) + Second->Size;
				
				Result = true;
			}
		}
	}
	
	return(Result);
}

internal b32 
GenerationHasCompleted(game_assets *Assets, u32 CheckID)
{
	b32 Result = true;
	
	for(u32 Index = 0;
		Index < Assets->InFlightGenerationCount;
		++Index)
	{
		if(Assets->InFlightGenerations[Index] == CheckID)
		{
			Result = false;
			break;
		}
	}
	
	return(Result);
}

inline asset_memory_header *
AcquireAssetMemory(game_assets *Assets, u32 Size, u32 AssetIndex, asset_header_type AssetType)
{
	asset_memory_header *Result = 0;
	
	asset_memory_block *Block = FindBlockForSize(Assets, Size);
	for(;;)
	{
		if(Block && (Size <= (Block->Size)))
		{
			Block->Flags |= AssetMemory_Used;
			
			Result = (asset_memory_header *)(Block + 1);
			
			memory_index RemainingSize = Block->Size - Size;
			memory_index BlockSplitThreshold = 4096;
			if(RemainingSize > BlockSplitThreshold)
			{
				Block->Size -= RemainingSize;
				InsertBlock(Block, RemainingSize, (u8 *)Result + Size);
			}
			
			break;
		}
		else
		{
			for(asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
				Header != &Assets->LoadedAssetSentinel;
				Header = Header->Prev)
			{
				asset *Asset = Assets->Assets + Header->AssetIndex;
				if((Asset->State >= AssetState_Loaded) && 
				   (GenerationHasCompleted(Assets, Asset->Header->GenerationID)))
				{
					u32 AssetIndex = Header->AssetIndex;
					asset *Asset = Assets->Assets + AssetIndex;
					
					Assert(Asset->State == AssetState_Loaded);
					
					RemoveAssetHeaderFromList(Header);
					
					if(AssetType == AssetType_Bitmap)
					{
						Platform.DeallocateTexture(Asset->Header->Bitmap.TextureHandle);
					}
					
					Block = (asset_memory_block *)Asset->Header - 1;
					Block->Flags &= ~AssetMemory_Used;
					
					if(MergeIfPossible(Assets, Block->Prev, Block))
					{
						Block = Block->Prev;
					}
					
					MergeIfPossible(Assets, Block, Block->Next);
					
					Asset->State = AssetState_Unloaded;
					Asset->Header = 0;
					
					// TODO(Denis): Actually do this, instead of just saying you're going to do it!
					//Block = EvictAsset(Assets, Header);
					break;
				}
			}
		}
	}
	
	if(Result)
	{
		Result->AssetType = AssetType;
		Result->AssetIndex = AssetIndex;
		Result->TotalSize = Size;
		InsertAssetHeaderAtFront(Assets, Result);
	}
	
	return(Result);
}

struct asset_memory_size
{
	u32 Total;
	u32 Data;
	u32 Section;
};

internal void
LoadBitmap(game_assets *Assets, bitmap_id ID)
{
	asset *Asset = Assets->Assets + ID.Value;
	if(ID.Value)
	{
		if(Asset->State == AssetState_Unloaded)
		{
			asset *Asset = Assets->Assets + ID.Value;
			sta_bitmap *Info = &Asset->STA.Bitmap;
			
			asset_memory_size Size = {};
			u32 Width = Info->Dim[0];
			u32 Height = Info->Dim[1];
			Size.Section = 4*Width;
			Size.Data = Size.Section * Height;
			Size.Total = Size.Data + sizeof(asset_memory_header);
			
			Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value, AssetType_Bitmap);
			
			loaded_bitmap *Bitmap = &Asset->Header->Bitmap;
			Bitmap->WidthOverHeight = (f32)Info->Dim[0] / (f32)Info->Dim[1];
			Bitmap->Width = Info->Dim[0];
			Bitmap->Height = Info->Dim[1];
			Bitmap->Pitch = Size.Section;
			Bitmap->TextureHandle = 0;
			Bitmap->Memory = (Asset->Header + 1);
			Bitmap->MipLevels = Info->MipLevels;
			Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
			
			load_asset LoadAsset;
			LoadAsset.Asset = Assets->Assets + ID.Value;
			LoadAsset.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
			LoadAsset.Offset = Asset->STA.DataOffset;
			LoadAsset.Size = Size.Data;
			LoadAsset.Destination = Bitmap->Memory;
			LoadAsset.FinalizeOperation = FinalizeAsset_Bitmap;
			LoadAsset.FinalState = AssetState_Loaded;
			
			LoadAssetDirectly(&LoadAsset);
			Asset->State = AssetState_Loaded;
		}
	}
	else
	{
		Asset->State = AssetState_Unloaded;
	}
}

internal void
LoadFont(game_assets *Assets, font_id ID)
{
	// TODO(Denis): Merge this boilerplate!!! Same between LoadBitmap, LoadSound, and LoadFont
	asset *Asset = Assets->Assets + ID.Value;
	if(ID.Value)
	{
		if(Asset->State == AssetState_Unloaded)
		{
			asset *Asset = Assets->Assets + ID.Value;
			sta_font *Info = &Asset->STA.Font;
			
			u32 HorizontalAdvanceSize = sizeof(f32)*Info->GlyphCount*Info->GlyphCount;
			u32 GlyphsSize = Info->GlyphCount*sizeof(sta_font_glyph);
			u32 UnicodeMapSize = sizeof(u16)*Info->OnePastHighestCodepoint;
			u32 SizeData = GlyphsSize + HorizontalAdvanceSize;
			u32 SizeTotal = SizeData + sizeof(asset_memory_header) + UnicodeMapSize;
			
			Asset->Header = AcquireAssetMemory(Assets, SizeTotal, ID.Value, AssetType_Font);
			
			loaded_font *Font = &Asset->Header->Font;
			Font->BitmapIDOffset = GetFile(Assets, Asset->FileIndex)->FontBitmapIDOffset;
			Font->Glyphs = (sta_font_glyph *)(Asset->Header + 1);
			Font->HorizontalAdvance = (f32 *)((u8 *)Font->Glyphs + GlyphsSize);
			Font->UnicodeMap = (u16 *)((u8 *)Font->HorizontalAdvance + HorizontalAdvanceSize);
			
			ZeroSize(UnicodeMapSize, Font->UnicodeMap);
			
			load_asset Work;
			Work.Asset = Assets->Assets + ID.Value;
			Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
			Work.Offset = Asset->STA.DataOffset;
			Work.Size = SizeData;
			Work.Destination = Font->Glyphs;
			Work.FinalizeOperation = FinalizeAsset_Font;
			Work.FinalState = AssetState_Loaded;
			
			LoadAssetDirectly(&Work);
			Asset->State = AssetState_Loaded;
		}
	}
	else
	{
		Asset->State = AssetState_Unloaded;
	}
}

internal void
LoadMesh(game_assets *Assets, mesh_id ID)
{
	// TODO(Denis): Merge this boilerplate!!! Same between LoadBitmap, LoadSound, LoadFont, and Load Mesh
	asset *Asset = Assets->Assets + ID.Value;
	if(ID.Value)
	{
		if(Asset->State == AssetState_Unloaded)
		{
			asset *Asset = Assets->Assets + ID.Value;
			sta_mesh *Info = &Asset->STA.Mesh;
			
			asset_memory_size Size = {};
			Size.Data = (Info->VerticesCount * sizeof(vertex_3d)) + (Info->IndicesCount * sizeof(u32));
			Size.Total = Size.Data + sizeof(asset_memory_header);
			
			Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value, AssetType_Mesh);
			
			loaded_mesh *Mesh = &Asset->Header->Mesh;
			Mesh->VerticesCount = Info->VerticesCount;
			Mesh->IndicesCount = Info->IndicesCount;
			Mesh->Vertices = (vertex_3d *)(Asset->Header + 1);
			Mesh->Indices = (u32 *)(Mesh->Vertices + Mesh->VerticesCount);
			
			load_asset LoadAsset;
			LoadAsset.Asset = Assets->Assets + ID.Value;
			LoadAsset.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
			LoadAsset.Offset = Asset->STA.DataOffset;
			LoadAsset.Size = Size.Data;
			LoadAsset.Destination = Mesh->Vertices;
			LoadAsset.FinalState = AssetState_Loaded;
			
			LoadAssetDirectly(&LoadAsset);
			Asset->State = AssetState_Loaded;
		}
	}
	else
	{
		Asset->State = AssetState_Unloaded;
	}
}

internal uint32
GetBestMatchAssetFrom(game_assets *Assets, asset_type_id TypeID, 
					  asset_vector *MatchVector, asset_vector *WeightVector)
{
	uint32 Result = 0;
	
	real32 BestDiff = Real32Maximum;
	asset_type *Type = Assets->AssetTypes + TypeID;
	for(uint32 AssetIndex = Type->FirstAssetIndex;
		AssetIndex  < Type->OnePastLastAssetIndex;
		++AssetIndex)
	{
		asset *Asset = Assets->Assets + AssetIndex;
		
		real32 TotalWeightedDiff = 0.0f;
		for(uint32 TagIndex = Asset->STA.FirstTagIndex;
			TagIndex < Asset->STA.OnePastLastTagIndex;
			++TagIndex)
		{
			sta_tag *Tag = Assets->Tags + TagIndex;
			
			real32 A = MatchVector->E[Tag->ID];
			real32 B = Tag->Value;
			real32 D0 = AbsoluteValue(A - B);
			real32 D1 = AbsoluteValue((A - Assets->TagRange[Tag->ID]*SignOf(A)) - B);
			real32 Difference = Minimum(D0,D1);
			
			real32 Weighted = WeightVector->E[Tag->ID]*Difference;
			TotalWeightedDiff += Weighted;
		}
		
		if(BestDiff > TotalWeightedDiff)
		{
			BestDiff = TotalWeightedDiff;
			Result = AssetIndex;
		}
	}
	
	return(Result);
}

internal u32
GetMatchAssetFrom(game_assets *Assets, asset_type_id TypeID, u32 ID)
{
	uint32 Result = 0;
	
	asset_type *Type = Assets->AssetTypes + TypeID;
	for(uint32 AssetIndex = Type->FirstAssetIndex;
		AssetIndex  < Type->OnePastLastAssetIndex;
		++AssetIndex)
	{
		asset *Asset = Assets->Assets + AssetIndex;
		
		for(uint32 TagIndex = Asset->STA.FirstTagIndex;
			TagIndex < Asset->STA.OnePastLastTagIndex;
			++TagIndex)
		{
			sta_tag *Tag = Assets->Tags + TagIndex;
			if(Tag->Value == ID)
			{
				Result = AssetIndex;
				return(Result);
			}
		}
	}
	
	return(Result);
}

internal uint32
GetFirstAssetFrom(game_assets *Assets, asset_type_id TypeID)
{
	// TODO(Denis): Actually look!
	uint32 Result = 0;
	
	asset_type *Type = Assets->AssetTypes + TypeID;
	if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
	{
		Result = Type->FirstAssetIndex;
	}
	
	return(Result);
}

inline bitmap_id
GetBestMatchBitmapFrom(game_assets *Assets, asset_type_id TypeID, 
					   asset_vector *MatchVector, asset_vector *WeightVector)
{
	bitmap_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
	return(Result);
}

inline bitmap_id
GetFirstBitmapFrom(game_assets *Assets, asset_type_id TypeID)
{
	bitmap_id Result = {GetFirstAssetFrom(Assets, TypeID)};
	return(Result);
}

internal font_id
GetBestMatchFontFrom(game_assets *Assets, asset_type_id TypeID, u32 FontIndex)
{
	font_id Result = {GetMatchAssetFrom(Assets, TypeID, FontIndex)};
	return(Result);
}

internal game_assets *
AllocateGameAssets(memory_arena *Arena, memory_index Size)
{
	game_assets *Assets = PushStruct(Arena, game_assets);
	
	Assets->NextGenerationID = 0;
	Assets->InFlightGenerationCount = 0;
	
	Assets->MemorySentinel.Flags = 0;
	Assets->MemorySentinel.Size = 0;
	Assets->MemorySentinel.Prev = &Assets->MemorySentinel;
	Assets->MemorySentinel.Next = &Assets->MemorySentinel;
	
	InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size, NoClear()));
	
	Assets->LoadedAssetSentinel.Next = 
		Assets->LoadedAssetSentinel.Prev =
		&Assets->LoadedAssetSentinel;
	
	for(uint32 TagType = 0;
		TagType < Tag_Count;
		++TagType)
	{
		Assets->TagRange[TagType] = 1000000.0f;
	}
	
	Assets->TagCount = 1;
	Assets->AssetCount = 1;
	
	platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin(PlatformFileType_AssetFile);
	Assets->FileCount = FileGroup.FileCount;
	Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
	
	for(u32 FileIndex = 0;
		FileIndex < Assets->FileCount;
		++FileIndex)
	{
		asset_file *File = Assets->Files + FileIndex;
		
		File->FontBitmapIDOffset = 0;
		File->TagBase = Assets->TagCount;
		
		ZeroStruct(File->Header);
		File->Handle = Platform.OpenNextFile(&FileGroup);
		Platform.ReadDataFromFile(&File->Handle, 0, sizeof(File->Header), &File->Header);
		
		u32 AssetTypeArraySize  = File->Header.AssetTypeCount*sizeof(sta_asset_type);
		File->AssetTypeArray = (sta_asset_type *)PushSize(Arena, AssetTypeArraySize);
		Platform.ReadDataFromFile(&File->Handle, File->Header.AssetTypes, 
								  AssetTypeArraySize, File->AssetTypeArray);
		
		if(File->Header.MagicValue != STA_MAGIC_VALUE)
		{
			Platform.FileError(&File->Handle, "STA file has invalid magic value.");
		}
		
		if(File->Header.Version > STA_VERISON)
		{
			Platform.FileError(&File->Handle, "STA file is of a later version.");
		}
		
		if(PlatformNoFileErrors(&File->Handle))
		{
			// NOTE(Denis): The first asset and tag slot in every STA 
			// is a null (reserved) so we dont count it as 
			// something we will need space for!
			Assets->TagCount += (File->Header.TagCount - 1);
			Assets->AssetCount += (File->Header.AssetCount - 1);
		}
		else
		{
			// TODO(Denis): Eventually, have some way of notyfying users of bogus files?
			InvalidCodePath;
		}
		
	}
	
	Platform.GetAllFilesOfTypeEnd(&FileGroup);
	
	// NOTE(Denis): Allocate all metadata space
	Assets->Assets = PushArray(Arena,  Assets->AssetCount, asset);
	Assets->Tags = PushArray(Arena,  Assets->TagCount, sta_tag);
	
	// NOTE(Denis): Reserve one null asset at the begining
	ZeroStruct(Assets->Tags[0]);
	
	// NOTE(Denis): Load Tags
	for(u32 FileIndex = 0;
		FileIndex < Assets->FileCount;
		++FileIndex)
	{
		asset_file *File = Assets->Files + FileIndex;
		if(PlatformNoFileErrors(&File->Handle))
		{
			// NOTE(Denis): Skip the first tag, since it's null
			u32 TagArraySize = sizeof(sta_tag)*(File->Header.TagCount-1);
			Platform.ReadDataFromFile(&File->Handle, File->Header.Tags + sizeof(sta_tag), // skip the null tag!
									  TagArraySize, Assets->Tags + File->TagBase);
		}
	}
	
	// NOTE(Denis): Reserve one null asset at the begining
	u32 AssetCount = 0;
	ZeroStruct(*(Assets->Assets + AssetCount));
	++AssetCount;
	
	// TODO(Denis): Excersize for the reader - how would you do this in a way
	// that scaled gracefully to hundreds of asset pack files? (or more!)
	for(u32 DestTypeID = 0;
		DestTypeID < Asset_Count;
		++DestTypeID)
	{
		asset_type *DestType = Assets->AssetTypes + DestTypeID;
		DestType->FirstAssetIndex = AssetCount;
		
		for(u32 FileIndex = 0;
			FileIndex < Assets->FileCount;
			++FileIndex)
		{
			asset_file *File = Assets->Files + FileIndex;
			
			if(PlatformNoFileErrors(&File->Handle))
			{
				for(u32 SourceIndex = 0;
					SourceIndex < File->Header.AssetTypeCount;
					++SourceIndex)
				{
					sta_asset_type *SourceType = File->AssetTypeArray + SourceIndex;
					
					if(SourceType->TypeID == DestTypeID)
					{
						if(SourceType->TypeID == Asset_FontGlyph)
						{
							File->FontBitmapIDOffset = AssetCount - SourceType->FirstAssetIndex;
						}
						
						u32 AssetCountForType = (SourceType->OnePastLastAssetIndex - 
												 SourceType->FirstAssetIndex); 
						
						temporary_memory TempMem = BeginTemporaryMemory(Arena);
						sta_asset *STAAssetArray = PushArray(Arena, AssetCountForType, sta_asset);
						
						Platform.ReadDataFromFile(&File->Handle,
												  File->Header.Assets + 
												  SourceType->FirstAssetIndex*sizeof(sta_asset),
												  AssetCountForType * sizeof(sta_asset),
												  STAAssetArray);
						for(u32 AssetIndex = 0;
							AssetIndex < AssetCountForType;
							++AssetIndex)
						{
							sta_asset *STAAsset = STAAssetArray + AssetIndex;
							
							Assert(AssetCount < Assets->AssetCount);
							asset *Asset = Assets->Assets + AssetCount++;
							
							Asset->FileIndex = FileIndex;
							Asset->STA = *STAAsset;
							if(Asset->STA.FirstTagIndex == 0)
							{
								Asset->STA.FirstTagIndex = Asset->STA.OnePastLastTagIndex = 0;
							}
							else
							{
								Asset->STA.FirstTagIndex += (File->TagBase - 1);
								Asset->STA.OnePastLastTagIndex += (File->TagBase - 1);
							}
							
						}
						
						EndTemporaryMemory(TempMem);
					}
				}
			}
		}
		
		DestType->OnePastLastAssetIndex = AssetCount;
	}
	
	Assert(AssetCount == Assets->AssetCount);
	
	return(Assets);
}

inline u32
GetGlyphFromCodePoint(sta_font *Info, loaded_font *Font, u32 CodePoint)
{
	u32 Result = 0;
	if(CodePoint < Info->OnePastHighestCodepoint)
	{
		Result = Font->UnicodeMap[CodePoint];
		Assert(Result < Info->GlyphCount);
	}
	
	return(Result);
}

internal f32
GetHorizontalAdvanceForPair(sta_font *Info, loaded_font *Font, u32 DesiredPrevCodePoint, u32 DesiredCodePoint)
{
	u32 PrevGlyph = GetGlyphFromCodePoint(Info, Font, DesiredPrevCodePoint);
	u32 Glyph = GetGlyphFromCodePoint(Info, Font, DesiredCodePoint);
	
	f32 Result = Font->HorizontalAdvance[PrevGlyph*Info->GlyphCount + Glyph];
	
	return(Result);
}

internal bitmap_id 
GetBitmapForGlyph(sta_font *Info, loaded_font *Font, u32 DesiredCodePoint)
{
	u32 Glyph = GetGlyphFromCodePoint(Info, Font, DesiredCodePoint);
	bitmap_id Result = Font->Glyphs[Glyph].BitmapID;
	Result.Value += Font->BitmapIDOffset;
	
	return(Result);
}

internal f32 
GetLineAdvanceFor(sta_font *Info)
{
	f32 Result = Info->AscenderHeight + Info->DescenderHeight + Info->ExternalLeading;
	
	return(Result);
}

internal f32 
GetStartingBaselineY(sta_font *Info)
{
	f32 Result = Info->AscenderHeight;
	
	return(Result);
}