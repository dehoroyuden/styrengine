//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------
#include "styr_engine.h"
#include "styr_asset.cpp"
#include "styr_debug.cpp"
#include "styr_material.cpp"
#include "styr_transform.cpp"
#include "styr_mesh.cpp"
#include "styr_scene_graph.cpp"
#include "styr_camera.cpp"
#include "styr_basic_ui.cpp"
#include "styr_basic_ui_text.cpp"
#include "styr_irrklang_audio.cpp"


internal void
PushRenderElement(engine_state EngineState, memory_arena *TranArena, engine_render_command_type Type)
{
	engine_render_commands *Commands = EngineState.RenderCommands;
	
	u32 Size = sizeof(engine_render_entry_header);
	engine_render_entry_header *Header = (engine_render_entry_header *)(Commands->CommandBufferBase + Commands->CommandBufferSize);
	*Header = {};
	Commands->CommandBufferSize += Size;
	Header->Type = Type;
	
	switch(Type)
	{
		case EngineRenderCommandType_None:
		{
			
		} break;
		case EngineRenderCommandType_PushAllMeshes:
		{
			engine_render_entry_push_all_meshes Result = {};
			scene_graph graph = GetSceneGraph(*EngineState.scene_graph_state_ptr, GetMainSceneGraph());
			u32 entities_count = graph.entity_count;
			u32 entities_mesh_vertices_count = 0;
			for(u32 index = 0; index < entities_count; ++index)
			{
				game_entity Entity = GetGameEntityByID(graph, index);
				mesh entity_mesh = GetMeshByID(*EngineState.mesh_state_ptr, Entity.mesh_id);
				if(entity_mesh.loaded_mesh_data.VerticesCount > 0)
				{
					Result.vertex_count += entity_mesh.loaded_mesh_data.VerticesCount;
					Result.index_count += entity_mesh.loaded_mesh_data.IndicesCount;
					Result.mesh_entry_offsets_count++;
				}
			}
			
			Result.mesh_entry_vertex_offsets = PushArray(TranArena, Result.mesh_entry_offsets_count, u32);
			Result.mesh_entry_index_offsets = PushArray(TranArena, Result.mesh_entry_offsets_count, u32);
			Result.mesh_entry_texture_indices = PushArray(TranArena, Result.mesh_entry_offsets_count, u32);
			
			Result.vertices = PushArray(TranArena, Result.vertex_count, vertex_3d);
			Result.indices = PushArray(TranArena, Result.index_count, u32);
			
			u32 *vertex_offset_array_ptr = Result.mesh_entry_vertex_offsets;
			u32 *index_offset_array_ptr = Result.mesh_entry_index_offsets;
			
			vertex_3d *vertex_array_ptr = Result.vertices;
			u32 *index_array_ptr = Result.indices;
			
			u32 offset_index = 0;
			u32 textures_index = 0;
			u32 entity_index = 0;
			for(u32 index = 0; index < entities_count; ++index)
			{
				game_entity Entity = GetGameEntityByID(graph, index);
				mesh entity_mesh = GetMeshByID(*EngineState.mesh_state_ptr, Entity.mesh_id);
				if(entity_mesh.loaded_mesh_data.VerticesCount > 0)
				{
					*vertex_offset_array_ptr = entity_mesh.loaded_mesh_data.VerticesCount;
					vertex_offset_array_ptr++;
					
					*index_offset_array_ptr = entity_mesh.loaded_mesh_data.IndicesCount;
					index_offset_array_ptr++;
					
					for(u32 vert_idx = 0; vert_idx < entity_mesh.loaded_mesh_data.VerticesCount; ++vert_idx)
					{
						vertex_array_ptr[vert_idx] = entity_mesh.loaded_mesh_data.Vertices[vert_idx];
					}
					
					for(u32 indices_idx = 0; indices_idx < entity_mesh.loaded_mesh_data.IndicesCount; ++indices_idx)
					{
						index_array_ptr[indices_idx] = entity_mesh.loaded_mesh_data.Indices[indices_idx] + offset_index;
					}
					
					vertex_array_ptr += entity_mesh.loaded_mesh_data.VerticesCount;
					index_array_ptr += entity_mesh.loaded_mesh_data.IndicesCount;
					
					offset_index += entity_mesh.loaded_mesh_data.VerticesCount;
					
					// NOTE(Denis): Here we fill array of texture indices for proper rendering objects
					// which will be not in order with actuall meshes against entitites.
					u32 texture_id = FindMaterialById(entity_mesh.material_index, *EngineState.mat_state_ptr).texture_index;
					Result.mesh_entry_texture_indices[textures_index] = texture_id;
					textures_index++;
					
					graph.entities_model_matrices[entity_index] = get_transform_matrix(graph, index);
					entity_index++;
				}
			}
			
			Header->command_push_all_meshes = Result;
			++Commands->CommandBufferElementCount;
		} break;
		
		case EngineRenderCommandType_UpdateCamera:
		{
			engine_render_entry_update_camera Result = {};
			Result.proj_mat4x4 = get_camera_by_id(*EngineState.camera_state_ptr, get_default_camera_index()).projection_mat4x4;
			Result.view_mat4x4 = get_camera_by_id(*EngineState.camera_state_ptr, get_default_camera_index()).view_mat4x4;
			Result.model_mats4x4 = EngineState.scene_graph_state_ptr->scene_graphs[0].entities_model_matrices;
			
			Header->command_update_camera = Result;
			++Commands->CommandBufferElementCount;
		} break;
		
		case EngineRenderCommandType_PushTexture:
		{
			engine_render_entry_push_texture Result = {};
			Result.width = EngineState.Textures[EngineState.TextureCount-1].Width;
			Result.height = EngineState.Textures[EngineState.TextureCount-1].Height;
			Result.mip_maps_count = EngineState.Textures[EngineState.TextureCount-1].MipMapLevels;
			Result.data = EngineState.Textures[EngineState.TextureCount-1].Data;
			
			Header->command_push_texture = Result;
			++Commands->CommandBufferElementCount;
		} break;
		
		case EngineRenderCommandType_TextRect:
		{
			engine_render_entry_text_rect Result = {};
			basic_ui_text_state text_state = *EngineState.basic_ui_text_state_ptr;
			
			Result.verts_array = text_state.vertices_buffer;
			Result.indices_array = text_state.indices_buffer;
			Result.vertex_count = text_state.vertices_count;
			Result.indices_count = text_state.indices_count;
			
			Header->command_text_rect = Result;
			++Commands->CommandBufferElementCount;
		} break;
		
		case EngineRenderCommandType_Quad:
		{
			engine_render_entry_quad Result = {};
			basic_ui_state quad_state = *EngineState.basic_ui_state_ptr;
			
			Result.verts_array = quad_state.vertices_buffer;
			Result.indices_array = quad_state.indices_buffer;
			Result.vertex_count = quad_state.vertices_count;
			Result.indices_count = quad_state.indices_count;
			
			Header->command_quad = Result;
			++Commands->CommandBufferElementCount;
		} break;
	}
}

inline void
push_texture(engine_state *EngineState, u32 ID, char* useless)
{
	u32 texture_index = GetMatchAssetFrom(EngineState->Assets, Asset_Texture, ID);
	LoadBitmap(EngineState->Assets, {texture_index});
	loaded_bitmap *Bitmap = GetBitmap(EngineState->Assets, {texture_index});
	texture_2d Texture = {};
	Texture.Width = Bitmap->Width;
	Texture.Height = Bitmap->Height;
	Texture.MipMapLevels = Bitmap->MipLevels;
	Texture.Data = (memory_index)Bitmap->Memory;
	EngineState->Textures[EngineState->TextureCount++] = Texture;
	
	PushRenderElement(*EngineState, &EngineState->TransientMemChunk, EngineRenderCommandType_PushTexture);
}

inline void
push_texture(engine_state *EngineState, loaded_bitmap *Bitmap, char* useless)
{
	texture_2d Texture = {};
	Texture.Width = Bitmap->Width;
	Texture.Height = Bitmap->Height;
	Texture.MipMapLevels = Bitmap->MipLevels;
	Texture.Data = (memory_index)Bitmap->Memory;
	EngineState->Textures[EngineState->TextureCount++] = Texture;
	
	PushRenderElement(*EngineState, &EngineState->TransientMemChunk, EngineRenderCommandType_PushTexture);
}

inline void
push_texture(engine_state *EngineState, texture_2d Texture, char* useless)
{
	EngineState->Textures[EngineState->TextureCount++] = Texture;
	
	PushRenderElement(*EngineState, &EngineState->TransientMemChunk, EngineRenderCommandType_PushTexture);
}

inline void
push_bitmap(engine_state *EngineState, bitmap_id ID, v3 Position, v4 Color)
{
    loaded_bitmap *Bitmap = GetBitmap(EngineState->Assets, ID);
    if(!Bitmap)
    {
        LoadBitmap(EngineState->Assets, ID);
        Bitmap = GetBitmap(EngineState->Assets, ID);
    }
    
    if(Bitmap)
    {
        push_bitmap(EngineState, Bitmap, Position, Color);
    }
    else
    {
        LoadBitmap(EngineState->Assets, ID);
    }
}


inline loaded_font *
push_font(engine_state *EngineState, font_id ID) 
{
	loaded_font *Font = GetFont(EngineState->Assets, ID);
	if(Font)
	{
		// NOTE(Denis): Nothing to do
	}
	else
	{
		LoadFont(EngineState->Assets, ID);
	}
	
	return(Font);
}

inline texture_2d
make_font_texture(memory_arena *TranMem, game_assets *Assets, engine_font_info font_info, engine_font_glyph *font_glyphs, u32 font_glyph_count, u32 AltasWidthHeight)
{
	texture_2d FontAtlasTexture = {};
	
	// NOTE(Denis): Load and fill EngineFontGlyphs array for future use as indices and UV coords inside one big
	// font texture at a start.
	loaded_font *Font = font_info.DebugFont;
	sta_font *Info = font_info.DebugFontInfo;
	
	u32 Index = 0;
	u32 MaxDimension = 0;
	for(char CodePoint = '!';
		CodePoint <= '~';
		++CodePoint)
	{
		bitmap_id ID = GetBitmapForGlyph(Info, Font, CodePoint);
		loaded_bitmap *Bitmap = GetBitmap(Assets, ID);
		
		if(!Bitmap)
		{
			LoadBitmap(Assets, ID);
			Bitmap = GetBitmap(Assets, ID);
		}
		
		u32 CurrentDim = (Bitmap->Width > Bitmap->Height) ? Bitmap->Width : Bitmap->Height;
		
		MaxDimension = (MaxDimension < CurrentDim ? CurrentDim : MaxDimension);
	}
	
	MaxDimension = MaxDimension > 16 ? 32 : 16;
	MaxDimension = MaxDimension > 32 ? 64 : MaxDimension;
	MaxDimension = MaxDimension > 64 ? 128 : MaxDimension;
	
	u32 AtlasWidth = AltasWidthHeight;
	u32 AtlasHeight = AltasWidthHeight;
	u32 CharactersInLine = AtlasWidth / MaxDimension;
	f32 dim_to_uv_percentage = 1.0f / AltasWidthHeight;
	
	Assert((CharactersInLine * CharactersInLine) >= font_glyph_count);
	
	FontAtlasTexture.Width = AtlasWidth;
	FontAtlasTexture.Height = AtlasHeight;
	FontAtlasTexture.MipMapLevels = 1;
	
	u32 *Pixels = (u32 *)PushSize(TranMem, ((AtlasWidth * AtlasHeight)*4));
	FontAtlasTexture.Data = (memory_index)Pixels;
	
	u32 font_atlas_width = 0;
	u32 font_atlas_height = 0;
	for(u32 GlyphIndex = 0;
		GlyphIndex < font_glyph_count;
		++GlyphIndex)
	{
		for(u32 Height = 0; 
			Height < font_glyphs[GlyphIndex].Height;
			++Height)
		{
			for(u32 Width = 0; 
				Width < font_glyphs[GlyphIndex].Width;
				++Width)
			{
				u32 FinalWidthIndex = font_atlas_width + Width;
				u32 FinalHeightIndex = (font_atlas_height + Height) * AtlasWidth;
				u32 *FontPixels = (u32 *)font_glyphs[GlyphIndex].Data;
				Pixels[FinalWidthIndex + FinalHeightIndex] =  FontPixels[Width + Height * (u32)font_glyphs[GlyphIndex].Width];
			}
		}
		
		font_atlas_width += CharactersInLine;
		
		f32 uv_Width = (font_atlas_width - (CharactersInLine - font_glyphs[GlyphIndex].Width)) * dim_to_uv_percentage;
		f32 uv_Height = (font_atlas_height + font_glyphs[GlyphIndex].Height) * dim_to_uv_percentage;
		
		v2 BottomLeft = V2((font_atlas_width - CharactersInLine) * dim_to_uv_percentage, uv_Height);
		v2 BottomRight = V2(uv_Width, uv_Height);
		v2 TopLeft = V2((font_atlas_width - CharactersInLine) * dim_to_uv_percentage, font_atlas_height * dim_to_uv_percentage);
		v2 TopRight = V2(uv_Width ,font_atlas_height * dim_to_uv_percentage);
		
		font_glyphs[GlyphIndex].TexCoord_LeftBottom = BottomLeft;
		font_glyphs[GlyphIndex].TexCoord_RightBottom = BottomRight;
		font_glyphs[GlyphIndex].TexCoord_LeftTop = TopLeft;
		font_glyphs[GlyphIndex].TexCoord_RightTop = TopRight;
		
		if(font_atlas_width % AtlasWidth == 0)
		{
			font_atlas_height += CharactersInLine;
			font_atlas_width = 0;
		}
	}
	
	return FontAtlasTexture;
}

internal void
ClearRenderCommands(engine_render_commands *RenderCommands)
{
	RenderCommands->CommandBufferSize = 0;
	RenderCommands->CommandBufferElementCount = 0;
}

internal void
EngineInitializeAtStart(game_memory *Memory, game_input *Input, engine_render_commands *RenderCommands)
{
	Platform = Memory->PlatformAPI;
	
	Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == 
		   (ArrayCount(Input->Controllers[0].Buttons) - 1));
	
	Assert(sizeof(engine_state) <= Memory->PermanentStorageSize);
	engine_state *EngineStatePtr = (engine_state *)Memory->PermanentStorage;
	
	InitializeArena(&EngineStatePtr->EngineMemChunk, Memory->PermanentStorageSize - sizeof(engine_state), (u8 *)Memory->PermanentStorage + sizeof(engine_state));
	
	InitializeArena(&EngineStatePtr->TransientMemChunk, Memory->TransientStorageSize, (u8 *)Memory->TransientStorage);
	
	EngineStatePtr->Assets = AllocateGameAssets(&EngineStatePtr->TransientMemChunk, Megabytes(128));
	EngineStatePtr->Textures = PushArray(&EngineStatePtr->TransientMemChunk, 128, texture_2d);
	EngineStatePtr->MaxTextureCount = 128;
	EngineStatePtr->RenderCommands = RenderCommands;
	
	SubArena(&EngineStatePtr->MaterialsMemoryChunk, &EngineStatePtr->EngineMemChunk, sizeof(material_state));
	SubArena(&EngineStatePtr->MeshesMemoryChunk, &EngineStatePtr->EngineMemChunk, sizeof(meshes_state));
	SubArena(&EngineStatePtr->SceneGraphsMemoryChunk, &EngineStatePtr->EngineMemChunk, sizeof(scene_graph_state));
	SubArena(&EngineStatePtr->CameraMemoryChunk, &EngineStatePtr->EngineMemChunk, sizeof(styr_camera_state));
	SubArena(&EngineStatePtr->BasicUIMemoryChunk, &EngineStatePtr->EngineMemChunk, sizeof(basic_ui_state));
	SubArena(&EngineStatePtr->BasicUITextMemoryChunk, &EngineStatePtr->EngineMemChunk, sizeof(basic_ui_text_state));
	//SubArena(&EngineStatePtr->TexturesMemoryChunk, &EngineStatePtr->EngineMemChunk, sizeof(material_state));
	
	// NOTE(Denis): Initialize text state
	EngineStatePtr->basic_ui_text_state_ptr = initialize_basic_ui_text_state(&EngineStatePtr->BasicUITextMemoryChunk, &EngineStatePtr->TransientMemChunk, EngineStatePtr->Assets, *RenderCommands->WindowWidth, *RenderCommands->WindowHeight, (4 * 2048));
	
	// NOTE(Denis): Building scene graph
	EngineStatePtr->mat_state_ptr = InitializeMaterials(&EngineStatePtr->MaterialsMemoryChunk, &EngineStatePtr->TransientMemChunk, 256);
	CreateMaterial("landcruiser_color_mat", MaterialType_Default, 1, EngineStatePtr->mat_state_ptr);
	CreateMaterial("viking_room_color_mat", MaterialType_Default, 2, EngineStatePtr->mat_state_ptr);
	CreateMaterial("sculpture_color_mat", MaterialType_Default, 3, EngineStatePtr->mat_state_ptr);
	
	EngineStatePtr->mesh_state_ptr = InitializeMeshes(&EngineStatePtr->MeshesMemoryChunk, &EngineStatePtr->TransientMemChunk, 1024);
	EngineStatePtr->basic_ui_state_ptr = InitializeBasicUIState(&EngineStatePtr->BasicUIMemoryChunk, &EngineStatePtr->TransientMemChunk, 512 * 4);
	
	// NOTE(Denis): For now, this first mesh must be _null or empty_ because 
	// for some objects we dont need to add actual mesh! In future we will 
	// replace with blender-like empty model.
	CreateMesh("none", 0, 0, 0, 0, 0, EngineStatePtr->mesh_state_ptr);
	CreateMesh("landcruiser", 0, 0, EngineStatePtr->mesh_state_ptr, EngineStatePtr->Assets);
	CreateMesh("viking_room", 1, 1, EngineStatePtr->mesh_state_ptr, EngineStatePtr->Assets);
	CreateMesh("cube", 2, 2, EngineStatePtr->mesh_state_ptr, EngineStatePtr->Assets);
	CreateMesh("miku", 3, 1, EngineStatePtr->mesh_state_ptr, EngineStatePtr->Assets);
	CreateMesh("thank_you", 4, 2, EngineStatePtr->mesh_state_ptr, EngineStatePtr->Assets);
	CreateMesh("cube", 2, 0, EngineStatePtr->mesh_state_ptr, EngineStatePtr->Assets);
	
	EngineStatePtr->scene_graph_state_ptr = InitializeSceneGraph(&EngineStatePtr->SceneGraphsMemoryChunk, &EngineStatePtr->TransientMemChunk, 1);
	CreateSceneGraph(EngineStatePtr->scene_graph_state_ptr, &EngineStatePtr->TransientMemChunk, 256);
	scene_graph *scene_graph_01 = GetSceneGraphPtr(*EngineStatePtr->scene_graph_state_ptr, 0);
	
	CreateGameEntity(scene_graph_01, 4, "MIKU");
	CreateGameEntity(scene_graph_01, 2, "room");
	CreateGameEntity(scene_graph_01, 3, "kubik");
	CreateGameEntity(scene_graph_01, 6, "kubik_01");
	CreateGameEntity(scene_graph_01, 5, "susanna");
	CreateGameEntity(scene_graph_01, 1, "landcruiser");
	
	f32 screen_width = (f32)*RenderCommands->WindowWidth;
	f32 screen_height = (f32)*RenderCommands->WindowHeight;
	
	// NOTE(Denis): Initialize camera state
	EngineStatePtr->camera_state_ptr = initialize_camera_state(&EngineStatePtr->CameraMemoryChunk, &EngineStatePtr->TransientMemChunk, 2);
	
	// NOTE(Denis): Camera create functions automatically creates game entity on the scene graph!
	u32 camera_id = create_perspective_camera(scene_graph_01, EngineStatePtr->camera_state_ptr, 65, screen_width, screen_height, 0.05f, 100.0f);
	u32 camera_entity_index = EngineStatePtr->camera_state_ptr->cameras[camera_id].entity_id;
	// NOTE(Denis): set initial camera transform
	set_transform(*scene_graph_01, default_camera_transform(), camera_entity_index);
	
	u32 orthogonal_camera_id = create_ortho_camera(scene_graph_01, EngineStatePtr->camera_state_ptr, 8, screen_width, screen_height, 0.05f, 100.0f);
	
	// NOTE(Denis): Set initial scale for all objects to be 1
	for(u32 i = 0 ; i < scene_graph_01->entity_count; ++i)
	{
		set_scale(*scene_graph_01, 0.5f, i);
	}
	
	PushRenderElement(*EngineStatePtr, &EngineStatePtr->TransientMemChunk, EngineRenderCommandType_PushAllMeshes);
	
	basic_ui_text_state *text_state_ptr = EngineStatePtr->basic_ui_text_state_ptr;
	texture_2d FontAtlasTexture = make_font_texture(&EngineStatePtr->TransientMemChunk, EngineStatePtr->Assets, text_state_ptr->font_info, text_state_ptr->font_glyphs, text_state_ptr->font_glyph_count, 608);
	
	push_texture(EngineStatePtr, FontAtlasTexture, "font_texture");
	push_texture(EngineStatePtr, 3, "landcruiser_tex");
	push_texture(EngineStatePtr, 2, "viking_room_tex");
	push_texture(EngineStatePtr, 1, "sculpture_tex");
	
	EngineStatePtr->CloseApp = false;
	EngineStatePtr->IsInitialized = true;

	//adding the sound engine made by IrrKlang
	EngineStatePtr->sound_engine = irrklang::createIrrKlangDevice();
	if (!EngineStatePtr->sound_engine) {
		GlobalRunning = false; // error starting up the engine
		EngineStatePtr->sound_engine->drop();
	}
	//if you need to play some 2D or 3D sound without any customization 
	//just call EngineStatePtr->sound_engine->play2D() or play3D() method for once (it will be played in loop of EngineUpdateAndRender)
	//but for customization its better to use ISound class that is made of engine class 
	//also his work and dependences will be in styr_irrklang_audio.cpp
	//play_3D_customized_sound();

	//now we need to create customized sound
	//now max sounds count id 64 but it can be changed in styr_irrklang_audio.cpp -> sounds array
	create_customized_3D_sound("P:/baka_mitai.mp3", V3(0, 0, 0), 1.0f, 1.0f, false);
	//then we can tell to engine play customized sound by play_3D_customized_sound();
	// but it has to be initialized once if you dont need huge amount of sounds at the same time
	//you need to use id of sound to play it (id begins from 0)
	play_3D_customized_sound(EngineStatePtr, 0);

}

// NOTE(Denis): Its important that we have pure functions without side effects
// because we need only 1 function where we actually write data to memory.
// 
// And probably one function where we get all data we need.
internal void
EngineUpdateAndRender(game_memory *Memory, game_input *Input, engine_render_commands *RenderCommands)
{
	b32 QuitRequested = false;
	
	Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == 
		   (ArrayCount(Input->Controllers[0].Buttons) - 1));
	
	engine_state *EngineStatePtr = (engine_state *)Memory->PermanentStorage;
	basic_ui_text_state *text_state_ptr = EngineStatePtr->basic_ui_text_state_ptr;
	
	// NOTE(Denis): Reset debug text positioning on screen
	u32 screen_width = *RenderCommands->WindowWidth;
	u32 screen_height = *RenderCommands->WindowHeight;
	text_state_ptr->font_info.AtY = 0.05f*(*RenderCommands->WindowHeight);
	
	u32 camera_id = get_default_camera_index();
	EngineStatePtr->camera_state_ptr->cameras[camera_id] = set_camera_fly_speed(*EngineStatePtr->camera_state_ptr, camera_id, Input->MouseZ);
	styr_camera camera = EngineStatePtr->camera_state_ptr->cameras[camera_id];
	
	scene_graph graph = GetSceneGraph(*EngineStatePtr->scene_graph_state_ptr, 0);
	transform camera_transform = get_transform(graph, camera.entity_id);
	v3 delta_pos = {};
	
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
				delta_pos += (camera.CameraSpeed * Input->dtForFrame * camera_transform.Front);
			}
			
			if(Controller->MoveDown.EndedDown)
			{
				delta_pos += -(camera.CameraSpeed * Input->dtForFrame * camera_transform.Front);
			}
			
			if(Controller->MoveLeft.EndedDown)
			{
				delta_pos += (camera.CameraSpeed * Input->dtForFrame * camera_transform.Right);
			}
			
			if(Controller->MoveRight.EndedDown)
			{
				delta_pos += -(camera.CameraSpeed * Input->dtForFrame * camera_transform.Right);
			}
			
			if(Controller->RotateLeft.EndedDown)
			{
				delta_pos += (camera.CameraSpeed * Input->dtForFrame * camera_transform.Up);
			}
			
			if(Controller->RotateRight.EndedDown)
			{
				delta_pos += -(camera.CameraSpeed * Input->dtForFrame * camera_transform.Up);
			}
			if(WasPressed(Controller->Back))
			{
				EngineStatePtr->sound_engine->drop();
				int i = 0;
				for (i; sounds[i].filepath != nullptr;i++)
					sounds[i].music->drop();
				QuitRequested = true;
			}
		}
	}
	
	// NOTE(Denis): Mouse position in vulkan space (-1, 1)
	v2 MousePos = {Input->MouseX, Input->MouseY};
	MousePos.x = (MousePos.x / (f32)*RenderCommands->WindowWidth * 2) - 1.0f;
	MousePos.y = -(MousePos.y / (f32)*RenderCommands->WindowHeight * 2) + 1.0f; // NOTE(Denis): Intentionally inverted Y coord
	
	f32 pitch = (Input->MouseDeltaX * 90.0f);
	f32 yaw = (Input->MouseDeltaY * 90.0f);
	f32 roll = 0;
	
	// NOTE(Denis): Note, that set_transform already update memory (current transform index in array)
	// we dont need to manualy update positions. set_transform() does all work. If you need to know
	// how it works, just go to _styr_transform.cpp_
	transform new_camera_transform = RotateCamera(graph, *EngineStatePtr->camera_state_ptr, camera_id, pitch, yaw, roll, 0, V3(0, 0, 1));
	set_transform(graph, new_camera_transform, EngineStatePtr->camera_state_ptr->cameras[camera_id].entity_id);
	
	u32 camera_transform_id = get_camera_transform_id(*EngineStatePtr->camera_state_ptr, camera_id);
	
	// NOTE(Denis): Move entities
	{
		move_entity(graph, camera_transform_id, delta_pos);
		// move_entity(graph, 5, V3(0,0,1), Input->dtForFrame);
		// move_entity(graph, "kubik", V3(0,-1,0.5f), Input->dtForFrame);
		// move_entity(graph, "kubik_01", V3(0.5f,-1,-0.5f), Input->dtForFrame);
	}

	create_writeline(v2{ 200,200 }, screen_width, screen_height, "first", "Penis to suck is so fucking");
	create_writeline(v2{ 400,200 }, screen_width, screen_height, "second", "Sucking penis is so fucking nice!");


	for (int i = 0; i < (sizeof(lines) / sizeof(write_line)); i++) {

		if (IsInRectangle(lines[i].pos_in_space, MousePos) && WasPressed(Input->MouseButtons[PlatformMouseButton_Left])) {

			for (int j = i + 1;j < (sizeof(lines) / sizeof(write_line));j++) {
				lines[j].acces = false;
				lines[i].color = { 0.0f,0.0f ,0.0f ,1.0f };
			}
			for (int j = i - 1;j >= 0;j--) {
				lines[j].acces = false;
				lines[i].color = { 0.0f,0.0f ,0.0f ,1.0f };
			}

			lines[i].acces = true;
			lines[i].color = { 0.05f,0.05f ,0.05f ,1.0f };

			clean_the_buffer();
			wrl_buffer.step = 0;
			wrl_buffer.position = {};
		}
		else if (!IsInRectangle(lines[i].pos_in_space, MousePos) && WasPressed(Input->MouseButtons[PlatformMouseButton_Left])) {
			lines[i].acces = false;
			lines[i].color = { 0.0f,0.0f ,0.0f ,1.0f };
		}

		draw_writeline(lines[i], EngineStatePtr, screen_width, screen_height);
	}

	for (int i = 0; i < (sizeof(lines) / sizeof(write_line)); i++) {
		if (lines[i].acces == true) {
			work_with_text(&lines[i], EngineStatePtr, Input, screen_width, screen_height);
		}
	}

	//draw_quad_screen_space(EngineStatePtr->basic_ui_state_ptr, screen_width, screen_height, V2(500, 400), V2(200,200), V4(1,1,0,1));

	//this is needed to work with sound
	v3 cam_pos = get_camera_v3_position(&new_camera_transform);
	v3 cam_front = get_camera_v3_front(&new_camera_transform);

	EngineStatePtr->sound_engine->setListenerPosition(get_v3_to_vec3df(cam_pos), get_v3_to_vec3df(cam_front));
	
	// NOTE(Denis): Debug Text Output
	{
		char TextBuffer[256];
		
		_snprintf_s(TextBuffer, sizeof(TextBuffer), "Frame time: %.0fms | FPS: %.0f", Memory->FrameSecondsElapsed * 1000, 1.0f / Memory->FrameSecondsElapsed);
		text_state_ptr->font_info.AtY = DEBUGTextLine(EngineStatePtr->Assets, text_state_ptr, screen_width, screen_height, TextBuffer);
		
		_snprintf_s(TextBuffer, sizeof(TextBuffer), "Camera speed: %d", (u32)camera.CameraSpeed);
		text_state_ptr->font_info.AtY = DEBUGTextLine(EngineStatePtr->Assets, text_state_ptr, screen_width, screen_height, TextBuffer);
		
		text_state_ptr->font_info.AtY = DEBUGTextLine(EngineStatePtr->Assets, text_state_ptr, screen_width, screen_height, "\\#080Press \\#900ESC \\#080to exit.");
		
		text_state_ptr->font_info.AtY = DEBUGTextLine(EngineStatePtr->Assets, text_state_ptr, screen_width, screen_height, "\\^8\\#312Version 0.0.037 Alpha.");

		text_state_ptr->font_info.AtY = DEBUGTextLine(EngineStatePtr->Assets, text_state_ptr, screen_width, screen_height, wrl_buffer.data);
	}
	
	// NOTE(Denis): Finalization of render commands. First, we clear render command buffer, and then 
	// push new commands
	{
		ClearRenderCommands(EngineStatePtr->RenderCommands);
		
		PushRenderElement(*EngineStatePtr, &EngineStatePtr->TransientMemChunk, EngineRenderCommandType_Quad);
		PushRenderElement(*EngineStatePtr, &EngineStatePtr->TransientMemChunk, EngineRenderCommandType_TextRect);
		PushRenderElement(*EngineStatePtr, &EngineStatePtr->TransientMemChunk, EngineRenderCommandType_UpdateCamera);
		
		// NOTE(Denis): Clear variables before next frame. Its impotant!!!
		EngineStatePtr->basic_ui_text_state_ptr->quad_count = 0;
		EngineStatePtr->basic_ui_text_state_ptr->vertices_count = 0;
		EngineStatePtr->basic_ui_text_state_ptr->indices_count = 0;
		
		EngineStatePtr->basic_ui_state_ptr->quad_count = 0;
		EngineStatePtr->basic_ui_state_ptr->vertices_count = 0;
		EngineStatePtr->basic_ui_state_ptr->indices_count = 0;
	}
	
	Memory->QuitRequested = QuitRequested;
}