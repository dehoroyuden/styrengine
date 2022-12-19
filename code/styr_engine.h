#ifndef STYR_H
//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

struct memory_arena
{
	memory_index Size;
	u8 *Base;
	memory_index Used;
	
	u32 TempCount;
};

struct temporary_memory
{
	memory_arena *Arena;
	memory_index Used;
};

inline void
InitializeArena(memory_arena *Chunk, memory_index Size, void *Base)
{
	Chunk->Size = Size;
	Chunk->Base = (u8 *)Base;
	Chunk->Used = 0;
	Chunk->TempCount = 0;
}

inline memory_index
GetAlignmentOffset(memory_arena *Chunk, memory_index Alignment)
{
	memory_index AlignmentOffset = 0;
	
	memory_index ResultPointer = (memory_index)Chunk->Base + Chunk->Used;
	memory_index AlignmentMask = Alignment - 1;
	if(ResultPointer & AlignmentMask)
	{
		AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
	}
	
	return AlignmentOffset;
}

enum arena_push_flag
{
	ArenaFlag_ClearToZero = 0x1,
};

struct arena_push_params
{
	u32 Flags;
	u32 Alignment;
};

inline arena_push_params
DefaultArenaParams()
{
	arena_push_params Params;
	Params.Flags = ArenaFlag_ClearToZero;
	Params.Alignment = 4;
	return Params;
}

inline arena_push_params
AlignNoClear(u32 Alignment)
{
	arena_push_params Params = DefaultArenaParams();
	Params.Flags &= ~ArenaFlag_ClearToZero;
	Params.Alignment = Alignment;
	return Params;
}

inline arena_push_params
Align(u32 Alignment, b32 Clear)
{
	arena_push_params Params = DefaultArenaParams();
	if(Clear)
	{
		Params.Flags &= ~ArenaFlag_ClearToZero;
	} else
	{
		Params.Flags |= ~ArenaFlag_ClearToZero;
	}
	Params.Alignment = Alignment;
	return(Params);
}

inline arena_push_params
NoClear(void)
{
	arena_push_params Params = DefaultArenaParams();
	Params.Flags &= ~ArenaFlag_ClearToZero;
	return(Params);
}

inline memory_index
GetArenaSizeRemaining(memory_arena *Arena, arena_push_params Params = DefaultArenaParams())
{
	memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Params.Alignment));
	return(Result);
}

#define PushArray(Arena, Count, type, ...) ((type *)PushSize_(Arena, (Count)*sizeof(type), ##__VA_ARGS__))
#define PushStruct(Arena, type, ...) ((type *)PushSize_(Arena, sizeof(type), ##__VA_ARGS__))
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ##__VA_ARGS__)

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count * sizeof((Pointer)[0]), Pointer)

inline void
ZeroSize(memory_index Size, void *Ptr)
{
	u8 *Byte = (u8 *)Ptr;
	while(Size--)
	{
		*Byte++ = 0;
	}
}

inline memory_index
GetEffectiveSizeFor(memory_arena *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
{
	memory_index Size = SizeInit;
	
	memory_index AlignmentOffset = GetAlignmentOffset(Arena, Params.Alignment);
	Size += AlignmentOffset;
	
	return Size;
}

inline b32
ArenaHasRoomFor(memory_arena *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
{
	memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Params);
	b32 Result = ((Arena->Used + Size) <= Arena->Size);
	return Result;
}

inline void *
PushSize_(memory_arena *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
{
	memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Params);
	
	Assert((Arena->Used + Size) <= Arena->Size);
	
	memory_index AlignmentOffset = GetAlignmentOffset(Arena, Params.Alignment);
	void *Result = Arena->Base + Arena->Used + AlignmentOffset;
	Arena->Used += Size;
	
	Assert(Size >= SizeInit);
	
	if(Params.Flags & ArenaFlag_ClearToZero)
	{
		ZeroSize(SizeInit, Result);
	}
	
	return Result;
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
	temporary_memory Result;
	
	Result.Arena = Arena;
	Result.Used = Arena->Used;
	
	++Arena->TempCount;
	
	return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
	memory_arena *Arena = TempMem.Arena;
	Assert(Arena->Used >= TempMem.Used);
	Arena->Used = TempMem.Used;
	Assert(Arena->TempCount > 0);
	--Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
	Assert(Arena->TempCount == 0);
}

inline void
SubArena(memory_arena *Result, memory_arena *Arena, memory_index Size, arena_push_params Params = DefaultArenaParams())
{
	Result->Size = Size;
	Result->Base = (uint8 *)PushSize_(Arena, Size, Params);
	Result->Used = 0;
	Result->TempCount = 0;
}

#include "styr_asset.h"
#include "styr_debug.h"
#include "styr_material.h"
#include "styr_transform.h"
#include "styr_mesh.h"
#include "styr_scene_graph.h"
#include "styr_camera.h"
#include "styr_basic_ui.h"
#include "styr_basic_ui_text.h"

enum engine_render_command_type
{
	EngineRenderCommandType_None,
	
	EngineRenderCommandType_Quad,
	EngineRenderCommandType_PushMesh,
	EngineRenderCommandType_PushAllMeshes,
	EngineRenderCommandType_TextRect,
	EngineRenderCommandType_PushTexture,
	EngineRenderCommandType_TexturedRect,
	EngineRenderCommandType_UpdateCamera,
	EngineRenderCommandType_UpdateMeshMatrix,
};

struct engine_render_entry_push_all_meshes
{
	u32 vertex_count;
	u32 index_count;
	u32 mesh_entry_offsets_count;
	u32 *mesh_entry_vertex_offsets;
	u32 *mesh_entry_index_offsets;
	u32 *mesh_entry_texture_indices;
	
	vertex_3d *vertices;
	u32 *indices;
};

struct engine_render_entry_update_camera
{
	mat4 view_mat4x4;
	mat4 proj_mat4x4;
	mat4 *model_mats4x4;
	
	u32 game_entities_to_update_count;
};

struct engine_render_entry_text_rect
{
	vertex_3d *verts_array;
	u32 *indices_array;
	u32 vertex_count;
	u32 indices_count;
};

struct engine_render_entry_quad
{
	vertex_3d *verts_array;
	u32 *indices_array;
	u32 vertex_count;
	u32 indices_count;
};

struct engine_render_entry_push_texture
{
	u32 width;
	u32 height;
	u32 mip_maps_count;
	memory_index data;
};

struct engine_render_entry_header
{
	engine_render_command_type Type;
	engine_render_entry_push_all_meshes command_push_all_meshes;
	engine_render_entry_update_camera command_update_camera;
	engine_render_entry_push_texture command_push_texture;
	engine_render_entry_text_rect command_text_rect;
	engine_render_entry_quad command_quad;
};

struct texture_2d
{
	u32 Width;
	u32 Height;
	u32 MipMapLevels;
	memory_index Data;
};

struct engine_state
{
	b32 CloseApp;
	b32 IsInitialized;
	memory_arena TransientMemChunk;
	memory_arena EngineMemChunk;
	
	memory_arena MaterialsMemoryChunk;
	memory_arena MeshesMemoryChunk;
	memory_arena CameraMemoryChunk;
	memory_arena SceneGraphsMemoryChunk;
	memory_arena BasicUIMemoryChunk;
	memory_arena BasicUITextMemoryChunk;
	//memory_arena TexturesMemoryChunk;
	
	styr_camera_state *camera_state_ptr;
	material_state *mat_state_ptr;
	meshes_state *mesh_state_ptr;
	scene_graph_state *scene_graph_state_ptr;
	basic_ui_state *basic_ui_state_ptr;
	basic_ui_text_state *basic_ui_text_state_ptr;
	
	game_assets *Assets;
	engine_render_commands *RenderCommands;
	
	u32 TextureCount;
	u32 MaxTextureCount;
	texture_2d *Textures;
	
	u32 *meshes_texture_index;
};

global_variable platform_api Platform;

inline void push_bitmap(engine_state *EngineState, bitmap_id ID, v3 Position, v4 Color = V4(1, 1, 1, 1));
inline void push_bitmap(engine_state *EngineState, loaded_bitmap *Bitmap, v3 Position, v4 Color = V4(1, 1, 1, 1));
internal void draw_ui_text(basic_ui_text_state *state, u32 screen_width, u32 screen_height, v2 Position, f32 Height, u32 Codepoint, v4 Color = V4(1,1,1,1));

internal void EngineUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer);

#define STYR_H
#endif //STYR_H
