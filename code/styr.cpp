//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------
#include "styr.h"

internal void
PushRenderElement(engine_state EngineState, engine_render_commands *Commands, engine_render_command_type Type)
{
	u32 Size = sizeof(engine_render_entry_header);
	engine_render_entry_header *Header = (engine_render_entry_header *)(Commands->CommandBufferBase + Commands->CommandBufferSize);
	Commands->CommandBufferSize += Size;
	
	Header->Offset = Commands->CommandBufferSize;
	Header->Type = Type;
	
	switch(Type)
	{
		case EngineRenderCommandType_None:
		{
			
		} break;
		
		case EngineRenderCommandType_ViewMatrix:
		{
			mat4 *Result = (mat4 *)(Commands->CommandBufferBase + Commands->CommandBufferSize);
			Size = sizeof(mat4);
			*Result = EngineState.Camera.Transform.Matrix;
			Commands->CommandBufferSize += Size;
			++Commands->CommandBufferElementCount;
		} break;
	}
}

internal void
EndRenderCommands(engine_render_commands *Commands)
{
	Commands->CommandBufferSize = 0;
	Commands->CommandBufferElementCount = 0;
}

internal void
EngineUpdateAndRender(game_memory *Memory, game_input *Input, engine_render_commands *RenderCommands)
{
	Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == 
		   (ArrayCount(Input->Controllers[0].Buttons) - 1));
	
	Assert(sizeof(engine_state) <= Memory->PermanentStorageSize);
	engine_state *EngineState = (engine_state *)Memory->PermanentStorage;
	
	if(!EngineState->IsInitialized)
	{
		InitializeArena(&EngineState->EngineMemChunk, Memory->PermanentStorageSize - sizeof(engine_state),
						(u8 *)Memory->PermanentStorage + sizeof(engine_state));
		
		InitializeArena(&EngineState->TransientMemChunk, Memory->TransientStorageSize, (u8 *)Memory->TransientStorage);
		
		// NOTE(Denis): Initialization scene object meshes
		EngineState->SceneObjectsCount = 1;
		EngineState->SceneObjects = (game_entity *)PushArray(&EngineState->TransientMemChunk, EngineState->SceneObjectsCount, game_entity);
		
		EngineState->Camera = {};
		EngineState->Camera.Transform.Matrix = Identity();
		EngineState->Camera.Transform.Position = V3(0,0,0);
		EngineState->Camera.Front = {0, -1, 0};
		EngineState->Camera.Right = {1, 0, 0};
		EngineState->Camera.Up = {0, 0, 1};
		EngineState->Camera.Yaw = -90.0f;
		EngineState->Camera.Pitch = 0.0f;
		
		EngineState->IsInitialized = true;
	}
	
	game_entity Camera = EngineState->Camera;
	v3 CameraPos = Camera.Transform.Position;
	v3 CameraFront = EngineState->Camera.Front;
	v3 CameraRight = EngineState->Camera.Right;
	v3 CameraUp = EngineState->Camera.Up;
	
	f32 CameraSpeed = 2.5f * Input->dtForFrame;
	
	// NOTE(Denis): Movement
	for(int ControllerIndex = 0;
		ControllerIndex < ArrayCount(Input->Controllers);
		++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input,ControllerIndex);
		
		if(Controller->IsAnalog)
		{
			// NOTE(Denis): Use analog movement tuning
		}
		else 
		{
			// NOTE(Denis): Use digital movement tuning
			if(Controller->MoveUp.EndedDown)
			{
				CameraPos += CameraSpeed * CameraFront;
			}
			if(Controller->MoveDown.EndedDown)
			{
				CameraPos -= CameraSpeed * CameraFront;
			}
			if(Controller->MoveLeft.EndedDown)
			{
				CameraPos += CameraSpeed * CameraRight;
			}
			if(Controller->MoveRight.EndedDown)
			{
				CameraPos -= CameraSpeed * CameraRight;
			}
			if(Controller->RotateLeft.EndedDown)
			{
				CameraPos += CameraSpeed * CameraUp;
			}
			if(Controller->RotateRight.EndedDown)
			{
				CameraPos -= CameraSpeed * CameraUp;
			}
			
			if(WasPressed(Controller->Back))
			{
				printf("Not handled yet!\n");
			}
		}
		
	}
	
	v2 MousePos = {Input->MouseX, Input->MouseY};
	MousePos.x = (MousePos.x / (f32)*RenderCommands->WindowWidth * 2) - 1.0f;
	MousePos.y = (-MousePos.y / (f32)*RenderCommands->WindowHeight * 2) - 1.0f;
	
	f32 Yaw = EngineState->Camera.Yaw - (Input->MouseDeltaX * 90.0f);
	f32 Pitch = EngineState->Camera.Pitch - (Input->MouseDeltaY * 90.0f);
	
	if(Pitch > 89.0f)
	{
		Pitch = 89.0f;
	}
	
	if(Pitch < -89.0f)
	{
		Pitch = -89.0f;
	}
	
	EngineState->Camera.Yaw = Yaw;
	EngineState->Camera.Pitch = Pitch;
	
	v3 Front = {};
	Front.x = Cos(RadiansFromDegrees(Yaw)) * Cos(RadiansFromDegrees(Pitch));
	Front.y = Sin(RadiansFromDegrees(Yaw)) * Cos(RadiansFromDegrees(Pitch));
	Front.z = Sin(RadiansFromDegrees(Pitch));
	CameraFront = Normalize(Front);
	
	v3 Right = {};
	Right = Normalize(CrossProduct(EngineState->Camera.Up, CameraFront));
	
	v3 CPF = CameraPos + (CameraFront);
	glm::vec3 LookAtMe = glm::vec3(CPF.x, CPF.y, CPF.z);
	
	mat4 View = ConvertToMatrix4x4(glm::lookAt(glm::vec3(CameraPos.x, CameraPos.y, CameraPos.z), LookAtMe, glm::vec3(CameraUp.x, CameraUp.y, CameraUp.z)));
	EngineState->Camera.Right = Right;
	EngineState->Camera.Transform.Matrix = View;
	EngineState->Camera.Transform.Position = CameraPos;
	EngineState->Camera.Front = CameraFront;
	
	PushRenderElement(*EngineState, RenderCommands, EngineRenderCommandType_ViewMatrix);
	
	EndRenderCommands(RenderCommands);
}