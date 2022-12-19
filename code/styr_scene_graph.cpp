//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

internal scene_graph_state *
InitializeSceneGraph(memory_arena *scene_graph_mem, memory_arena *tran_memory, u32 scene_graph_count)
{
	scene_graph_state *Result = PushStruct(scene_graph_mem, scene_graph_state);
	Result->scene_graph_count = 0;
	Result->scene_graphs = PushArray(tran_memory, scene_graph_count, scene_graph);
	Result->max_scene_graph_count = scene_graph_count;
	
	return Result;
}

// NOTE(Denis): This function creates scene graph and initialize all game entities to zero
// in scene graph entities array
internal u32 
CreateSceneGraph(scene_graph_state *state, memory_arena *tran_memory, u32 entities_count)
{
	if(state->scene_graphs)
	{
		Assert(state->scene_graph_count <= state->max_scene_graph_count);
		
		scene_graph graph = {};
		graph.entity_count = 0;
		graph.entities = PushArray(tran_memory, entities_count, game_entity);
		graph.entities_transforms = PushArray(tran_memory, entities_count, transform);
		graph.entities_model_matrices = PushArray(tran_memory, entities_count, mat4);
		state->scene_graphs[state->scene_graph_count++] = graph;
		
		for(u32 i = 0; i < entities_count; ++i)
		{
			graph.entities_model_matrices[i] = Identity();
		}
		
		return state->scene_graph_count-1;
	}
	
	return 0;
}

inline scene_graph *
GetSceneGraphPtr(scene_graph_state state, u32 scene_id)
{
	scene_graph *Result = &state.scene_graphs[scene_id];
	
	return Result;
}

inline scene_graph
GetSceneGraph(scene_graph_state state, u32 scene_id)
{
	scene_graph Result = state.scene_graphs[scene_id];
	
	return Result;
}


internal u32
CreateGameEntity(scene_graph *graph, u32 mesh_id, char *name)
{
	if(graph->entities)
	{
		u32 id = graph->entity_count++;
		game_entity *new_entity = &graph->entities[id];
		new_entity->mesh_id = mesh_id;
		new_entity->transform_id = id;
		new_entity->parent_entity_id = 0;
		new_entity->name = name;
		
		return id;
	}
	else
	{
		InvalidCodePath;
	}
	
	return 0;
}

inline game_entity
AttachToGameEntity(scene_graph graph, u32 child_entity_id, u32 parent_entity_id = 0)
{
	game_entity result = {};
	result = graph.entities[child_entity_id];
	result.parent_entity_id = parent_entity_id;
	
	return result;
}

inline game_entity
GetGameEntityByID(scene_graph graph, u32 entity_id)
{
	Assert(entity_id != (0x0001 << 31));
	game_entity Result = graph.entities[entity_id];
	
	return Result;
}

inline u32
GetEntityIDByName(scene_graph graph, char* name)
{
	u32 result = (u32)(0x0001 << 31);
	
	for(u32 i = 0; i < graph.entity_count; ++i)
	{
		if(StringsAreEqual(graph.entities[i].name, name))
		{
			result = i;
			break;
		}
	}
	
	Assert(result != (0x0001 << 31));
	return result;
}

inline u32
GetMainSceneGraph()
{
	return 0;
}