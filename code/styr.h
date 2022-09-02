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
InitializeArena(memory_arena *Arena, memory_index Size, void *Base)
{
	Arena->Size = Size;
	Arena->Base = (u8 *)Base;
	Arena->Used = 0;
	Arena->TempCount = 0;
}

inline memory_index
GetAlignmentOffset(memory_arena *Arena, memory_index Alignment)
{
	memory_index AlignmentOffset = 0;
	
	memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
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

struct Vertex
{
	v3 Pos;
	v3 Color;
	v2 TexCoord;
};

struct Mesh
{
	u32 VertexCount;
	Vertex *Vertices;
};

struct Camera
{
	mat4 ProjectionMatrix;
	mat4 ViewMatrix;
};

struct engine_state
{
	b32 IsInitialized;
	memory_arena EngineArena;
	memory_arena TransientArena;
	
	u32 VerticesCount;
	Vertex *VerticesBuffer;
};

internal void EngineUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer);

#define STYR_H
#endif //STYR_H
