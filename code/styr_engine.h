#ifndef STYR_H
//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

struct memory_chunk
{
	memory_index Size;
	u8 *Base;
	memory_index Used;
	
	u32 TempCount;
};

struct chunk_temporary_memory
{
	memory_chunk *Arena;
	memory_index Used;
};

inline void
InitializeArena(memory_chunk *Chunk, memory_index Size, void *Base)
{
	Chunk->Size = Size;
	Chunk->Base = (u8 *)Base;
	Chunk->Used = 0;
	Chunk->TempCount = 0;
}

inline memory_index
GetAlignmentOffset(memory_chunk *Chunk, memory_index Alignment)
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
GetArenaSizeRemaining(memory_chunk *Arena, arena_push_params Params = DefaultArenaParams())
{
	memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Params.Alignment));
	return(Result);
}

#define PushArray(Arena, Count, type, ...) ((type *)PushSize_(Arena, (Count)*sizeof(type), ##__VA_ARGS__))
#define PushStruct(Arena, type, ...) ((type *)PushSize_(Arena, sizeof(type), ##__VA_ARGS__))
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ##__VA_ARGS__)

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
GetEffectiveSizeFor(memory_chunk *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
{
	memory_index Size = SizeInit;
	
	memory_index AlignmentOffset = GetAlignmentOffset(Arena, Params.Alignment);
	Size += AlignmentOffset;
	
	return Size;
}

inline b32
ArenaHasRoomFor(memory_chunk *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
{
	memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Params);
	b32 Result = ((Arena->Used + Size) <= Arena->Size);
	return Result;
}

inline void *
PushSize_(memory_chunk *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
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

inline chunk_temporary_memory
BeginTemporaryMemory(memory_chunk *Arena)
{
	chunk_temporary_memory Result;
	
	Result.Arena = Arena;
	Result.Used = Arena->Used;
	
	++Arena->TempCount;
	
	return(Result);
}

inline void
EndTemporaryMemory(chunk_temporary_memory TempMem)
{
	memory_chunk *Arena = TempMem.Arena;
	Assert(Arena->Used >= TempMem.Used);
	Arena->Used = TempMem.Used;
	Assert(Arena->TempCount > 0);
	--Arena->TempCount;
}

inline void
CheckArena(memory_chunk *Arena)
{
	Assert(Arena->TempCount == 0);
}

enum engine_render_command_type
{
	EngineRenderCommandType_None,
	EngineRenderCommandType_ViewMatrix,
	EngineRenderCommandType_Rect,
	EngineRenderCommandType_Bitmap,
};

struct engine_render_entry_header
{
	engine_render_command_type Type;
	u32 OffsetsCount; // Currently max _4_
	u32 Offsets[4]; // Offsets its max data count, that we will be able to pack in a single RenderElement call.
	u32 Sizes[4];
};

struct quaternion
{
	f32 Angle;
	f32 x;
	f32 y;
	f32 z;
};

struct transform
{
	v3 Position;
	v3 Scale;
	quaternion Rotation;
	
	mat4 Matrix;
};

struct vertex_3d
{
	v3 Pos;
	v3 Color;
	v2 TexCoord;
};

struct vertex_2d
{
	v2 Pos;
	v3 Color;
	v2 TexCoord;
};

struct game_entity
{
	u32 VertexCount;
	vertex_3d *Vertices;
	
	transform Transform;
	v3 Front;
	v3 Right;
	v3 Up;
	
	f32 Yaw;
	f32 Pitch;
};

struct Camera
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
};

// TODO(Denis): Finish scene graph
struct sryr_engine_scene_graph
{
	
};

struct engine_state
{
	b32 IsInitialized;
	memory_chunk EngineMemChunk;
	memory_chunk TransientMemChunk;
	
	u32 ui_rectangle_count;
	u32 VerticesCount;
	vertex_3d *ui_vertices_buffer;
	
	u32 IndicesCount;
	u32 *ui_indices_buffer;
	
	u32 SceneObjectsCount;
	game_entity *SceneObjects;
	
	game_entity Camera;
	f32 CameraSpeed;
	f32 CameraBlend;
	f32 MaxCameraSpeed;
	f32 MinCameraSpeed;
};

internal void EngineUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer);

#define STYR_H
#endif //STYR_H
