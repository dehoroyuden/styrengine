//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------
#include "styr.h"

internal Camera
SetPerspectiveProjection(r32 Fovy, r32 Aspect, r32 Near, r32 Far)
{
	Camera Result = {};
	
	Result.ProjectionMatrix = ConvertToMatrix4x4(glm::perspective(glm::radians(Fovy), Aspect, Near, Far));
	
	return Result;
}

internal void
EngineUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer)
{
	Assert(sizeof(engine_state) <= Memory->TransientStorageSize);
	engine_state *EngineState = (engine_state *)Memory->TransientStorage;
	if(!EngineState->IsInitialized)
	{
		InitializeArena(&EngineState->TransientArena, Memory->TransientStorageSize - sizeof(engine_state),
						(u8 *)Memory->TransientStorage + sizeof(engine_state));
		
		// NOTE(Denis): Vertices
		EngineState->VerticesCount = 3;
		EngineState->VerticesBuffer = PushArray(&EngineState->TransientArena, EngineState->VerticesCount, Vertex);
		
		EngineState->VerticesBuffer[0] = {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}};
		EngineState->VerticesBuffer[1] = {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}};
		EngineState->VerticesBuffer[2] = {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}};
	}
}