//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------
#include "styr_engine.h"

internal void
PushRenderElement(engine_state EngineState, engine_render_commands *Commands, engine_render_command_type Type)
{
	u32 Size = sizeof(engine_render_entry_header);
	engine_render_entry_header *Header = (engine_render_entry_header *)(Commands->CommandBufferBase + Commands->CommandBufferSize);
	Commands->CommandBufferSize += Size;
	Header->Type = Type;
	
	switch(Type)
	{
		case EngineRenderCommandType_None:
		{
			
		} break;
		
		case EngineRenderCommandType_ViewMatrix:
		{
			Header->Offsets[0] = Commands->CommandBufferSize;
			Header->OffsetsCount += 1;
			
			mat4 *Result = (mat4 *)(Commands->CommandBufferBase + Commands->CommandBufferSize);
			Size = sizeof(mat4);
			*Result = EngineState.Camera.Transform.Matrix;
			Commands->CommandBufferSize += Size;
			++Commands->CommandBufferElementCount;
			
			Header->Sizes[0] = Size;
		} break;
		
		case EngineRenderCommandType_Rect:
		{
			Header->Offsets[0] = Commands->CommandBufferSize;
			Header->OffsetsCount += 1;
			vertex_3d *verts = (vertex_3d *)(Commands->CommandBufferBase + Commands->CommandBufferSize);
			Size = EngineState.VerticesCount * sizeof(vertex_3d);
			for(u32 index = 0;
				index < EngineState.VerticesCount;
				++index)
			{
				verts[index] = EngineState.ui_vertices_buffer[index];
			}
			Commands->CommandBufferSize += Size;
			Header->Sizes[0] = Size;
			
			Header->Offsets[1] = Commands->CommandBufferSize;
			Header->OffsetsCount += 1;
			u32 *inds = (u32 *)(Commands->CommandBufferBase + Commands->CommandBufferSize);
			Size = EngineState.IndicesCount * sizeof(u32);
			for(u32 index = 0;
				index < EngineState.IndicesCount;
				++index)
			{
				inds[index] = EngineState.ui_indices_buffer[index];
			}
			Commands->CommandBufferSize += Size;
			++Commands->CommandBufferElementCount;
			Header->Sizes[1] = Size;
		} break;
	}
}

internal void
push_ui_rect(engine_state *EngineState, engine_render_commands Commands, f32 width_in_pixels, f32 height_in_pixels, v2 Origin, v3 Color)
{
	rectangle2 rect = RectCenterDim(Origin, V2(width_in_pixels, height_in_pixels));
	
	mat4 ScreenSpaceMatrix = ScreenSpaceToVulkanSpace(*Commands.WindowWidth, *Commands.WindowHeight);
	
	v3 vt1 = (ScreenSpaceMatrix * V4(rect.Min.x, rect.Min.y, 0, 1.0f)).xyz;
	v3 vt2 = (ScreenSpaceMatrix * V4(rect.Max.x, rect.Min.y, 0, 1.0f)).xyz;
	v3 vt3 = (ScreenSpaceMatrix * V4(rect.Min.x, rect.Max.y, 0, 1.0f)).xyz;
	v3 vt4 = (ScreenSpaceMatrix * V4(rect.Max.x, rect.Max.y, 0, 1.0f)).xyz;
	
	vertex_3d Vert1 = {vt1, Color, {0, 0}};
	vertex_3d Vert2 = {vt2, Color, {0, 0}};
	vertex_3d Vert3 = {vt3, Color, {0, 0}};
	vertex_3d Vert4 = {vt4, Color, {0, 0}};
	
	EngineState->ui_vertices_buffer[EngineState->VerticesCount++] = Vert1;
	EngineState->ui_vertices_buffer[EngineState->VerticesCount++] = Vert2;
	EngineState->ui_vertices_buffer[EngineState->VerticesCount++] = Vert3;
	EngineState->ui_vertices_buffer[EngineState->VerticesCount++] = Vert4;
	
	EngineState->ui_indices_buffer[EngineState->IndicesCount++] = EngineState->ui_rectangle_count * 4 + 0;
	EngineState->ui_indices_buffer[EngineState->IndicesCount++] = EngineState->ui_rectangle_count * 4 + 1;
	EngineState->ui_indices_buffer[EngineState->IndicesCount++] = EngineState->ui_rectangle_count * 4 + 2;
	EngineState->ui_indices_buffer[EngineState->IndicesCount++] = EngineState->ui_rectangle_count * 4 + 1;
	EngineState->ui_indices_buffer[EngineState->IndicesCount++] = EngineState->ui_rectangle_count * 4 + 3;
	EngineState->ui_indices_buffer[EngineState->IndicesCount++] = EngineState->ui_rectangle_count * 4 + 2;
	
	++EngineState->ui_rectangle_count;
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
		EngineState->Camera.Transform.Position = V3(0,1,1);
		EngineState->Camera.Front = {0, -1, 0};
		EngineState->Camera.Right = {1, 0, 0};
		EngineState->Camera.Up = {0, 0, 1};
		EngineState->Camera.Yaw = -90.0f;
		EngineState->Camera.Pitch = 0.0f;
		EngineState->CameraSpeed = 2.5f;
		EngineState->CameraBlend = 0;
		EngineState->MaxCameraSpeed = 40.0f;
		EngineState->MinCameraSpeed = 1.0f;
		EngineState->VerticesCount = 0;
		EngineState->IndicesCount = 0;
		EngineState->ui_rectangle_count = 0;
		EngineState->ui_vertices_buffer = (vertex_3d *)PushSize(&EngineState->TransientMemChunk, Kilobytes(5));
		EngineState->ui_indices_buffer = (u32 *)PushSize(&EngineState->TransientMemChunk, Kilobytes(1));
		
		EngineState->IsInitialized = true;
	}
	
	game_entity Camera = EngineState->Camera;
	v3 CameraPos = Camera.Transform.Position;
	v3 CameraFront = EngineState->Camera.Front;
	v3 CameraRight = EngineState->Camera.Right;
	v3 CameraUp = EngineState->Camera.Up;
	
	EngineState->CameraBlend = Clamp01(EngineState->CameraBlend + Input->MouseZ);
	EngineState->CameraSpeed = Lerp(EngineState->MinCameraSpeed, EngineState->CameraBlend, EngineState->MaxCameraSpeed);
	
	f32 CameraSpeed = EngineState->CameraSpeed * Input->dtForFrame;
	
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
	//MousePos.y = (MousePos.y / (f32)*RenderCommands->WindowHeight * 2) - 1.0f;
	MousePos.y = -(MousePos.y / (f32)*RenderCommands->WindowHeight * 2) + 1.0f; // NOTE(Denis): Intentionally inverted Y coord
	
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
	
	mat4 ScreenSpaceMatrix = ScreenSpaceToVulkanSpace(*RenderCommands->WindowWidth, *RenderCommands->WindowHeight);
	v3 CameraPosOffset = {};
	
	v3 Right = {};
	Right = Normalize(CrossProduct(EngineState->Camera.Up, CameraFront));
	
	v3 CPF = CameraPos + (CameraFront);
	glm::vec3 LookAtMe = glm::vec3(CPF.x, CPF.y, CPF.z);
	
	// Shift camera point of rotation
	mat4 Model = ConvertToMatrix4x4(glm::translate(glm::mat4(1.0f), glm::vec3(-CameraFront.x, -CameraFront.y, -CameraFront.z)));
	mat4 View = ConvertToMatrix4x4(glm::lookAt(glm::vec3(CameraPos.x, CameraPos.y, CameraPos.z), LookAtMe, glm::vec3(CameraUp.x, CameraUp.y, CameraUp.z)));
	EngineState->Camera.Right = Right;
	EngineState->Camera.Transform.Matrix = View;// * Model;
	EngineState->Camera.Transform.Position = CameraPos;
	EngineState->Camera.Front = CameraFront;
	
	v2 bg_rect_size = {500.0f, (f32)*RenderCommands->WindowHeight};
	v2 bg_rect_center = {(f32)(bg_rect_size.x / 2), (f32)*RenderCommands->WindowHeight - (bg_rect_size.y / 2)};
	
	rectangle2 rect = RectCenterDim(bg_rect_center, V2(bg_rect_size.x * 0.8f, bg_rect_size.y * 0.9f));
	v2 RectMin = (ScreenSpaceMatrix * V4(rect.Min.x, rect.Min.y, 0, 1.0f)).xy;
	v2 RectMax = (ScreenSpaceMatrix * V4(rect.Max.x, rect.Max.y, 0, 1.0f)).xy;
	rectangle2 rect_in_vulkan_space = {RectMin, RectMax};
	
	v3 Color = IsInRectangle(rect_in_vulkan_space, V2(MousePos.x, MousePos.y)) ? V3(0,0,0.345f) : V3(1, 1, 0);
	
	// TODO(Denis): Remake this functions call to be render _agnostic_, meaning that we just need to create command,
	// not vertices and indices, but simply width and height with origin.
	push_ui_rect(EngineState, *RenderCommands, bg_rect_size.x, bg_rect_size.y, bg_rect_center, V3(0.05f,0.05f,0.05f));
	push_ui_rect(EngineState, *RenderCommands, bg_rect_size.x * 0.8f, bg_rect_size.y * 0.9f, bg_rect_center, Color);
	//push_ui_rect(EngineState, *RenderCommands, 300, 300, V2(800, 200), V3(0.25f, 0, 0));
	//push_ui_rect(*EngineState, RenderCommands, 300, 300, V2(500, 500), V3(0, 1, 0));
	
	PushRenderElement(*EngineState, RenderCommands, EngineRenderCommandType_Rect);
	PushRenderElement(*EngineState, RenderCommands, EngineRenderCommandType_ViewMatrix);
	
	EngineState->VerticesCount = 0;
	EngineState->IndicesCount = 0;
	EngineState->ui_rectangle_count = 0;
}