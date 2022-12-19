//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

// NOTE(Denis): IMPORTANT!!!(Denis): All functions that are writing directly to memory
// must be called with _set_ on the begining of name!

inline mat4
create_entitity_position_matrix(transform entity_tfm, v3 new_position)
{
	mat4 result = Identity();
	
	result.Value1_1 = entity_tfm.Scale.x;
	result.Value2_2 = entity_tfm.Scale.y;
	result.Value3_3 = entity_tfm.Scale.z;
	
	result.Value1_4 = new_position.x;
	result.Value2_4 = new_position.y;
	result.Value3_4 = new_position.z;
	
	result = TransposeMatrix(result);
	
	return result;
}

inline mat4
create_entity_scale_matrix(transform entity_tfm, v3 new_scale)
{
	mat4 result = Identity();
	
	result.Value1_1 = new_scale.x;
	result.Value2_2 = new_scale.y;
	result.Value3_3 = new_scale.z;
	
	result.Value1_4 = entity_tfm.Position.x;
	result.Value2_4 = entity_tfm.Position.y;
	result.Value3_4 = entity_tfm.Position.z;
	
	return result;
}

inline transform
get_transform(scene_graph scene_graph, u32 game_entity_id)
{
	Assert(game_entity_id != (0x0001 << 31));
	
	transform Result = {};
	
	u32 transform_index = scene_graph.entities[game_entity_id].transform_id;
	Result = scene_graph.entities_transforms[transform_index];
	
	return Result;
}

internal u32
get_transform_id(scene_graph graph, u32 game_entity_id)
{
	u32 Result = 0;
	
	Result = graph.entities[game_entity_id].transform_id;
	
	return Result;
}

inline void
set_transform(scene_graph graph, transform new_transform, u32 game_entity_id)
{
	u32 transform_index = graph.entities[game_entity_id].transform_id;
	transform *object_transform = &graph.entities_transforms[transform_index];
	*object_transform = new_transform;
	
	mat4 new_transform_mtx = create_entitity_position_matrix(new_transform, new_transform.Position);
	
	// NOTE(Denis): Update model matrices for objects - these are send to vulkan 
	// layer along with view matrix and camera projection
	graph.entities_model_matrices[game_entity_id] = new_transform_mtx;
}

inline void
set_transform(scene_graph graph, mat4 new_transform_mtx, u32 game_entity_id)
{
	mat4 transposed_mtx = TransposeMatrix(new_transform_mtx);
	transform new_transform = {};
	new_transform.Position = V3(transposed_mtx.Value1_4, transposed_mtx.Value2_4, transposed_mtx.Value3_4);
	
	u32 transform_index = graph.entities[game_entity_id].transform_id;
	transform *object_transform = &graph.entities_transforms[transform_index];
	*object_transform = new_transform;
	
	// NOTE(Denis): Update model matrices for objects - these are send to vulkan 
	// layer along with view matrix and camera projection
	graph.entities_model_matrices[game_entity_id] = new_transform_mtx;
}

inline void
set_scale(scene_graph graph, v3 new_scale, u32 game_entity_id)
{
	u32 transform_index = graph.entities[game_entity_id].transform_id;
	transform *object_transform = &graph.entities_transforms[transform_index];
	object_transform->Scale = new_scale;
}

inline void
set_scale(scene_graph graph, f32 uniform_scale, u32 game_entity_id)
{
	u32 transform_index = graph.entities[game_entity_id].transform_id;
	transform *object_transform = &graph.entities_transforms[transform_index];
	object_transform->Scale = V3(uniform_scale, uniform_scale, uniform_scale);
}

inline void
move_entity(scene_graph graph, u32 game_entity_id, v3 direction, f32 delta)
{
	transform entity_tfm = get_transform(graph, game_entity_id);
	entity_tfm.Position += direction * delta;
	
	set_transform(graph, entity_tfm, game_entity_id);
}

inline void
move_entity(scene_graph graph, u32 game_entity_id, v3 delta_pos)
{
	transform entity_tfm = get_transform(graph, game_entity_id);
	entity_tfm.Position += delta_pos;
	
	set_transform(graph, entity_tfm, game_entity_id);
}

inline void
move_entity(scene_graph graph, char* name, v3 direction, f32 delta)
{
	u32 game_entity_id = 0;
	
	for(u32 i = 0; i < graph.entity_count; ++i)
	{
		if(StringsAreEqual(name, graph.entities[i].name))
		{
			game_entity_id = i;
			break;
		}
	}
	
	transform entity_tfm = get_transform(graph, game_entity_id);
	entity_tfm.Position += direction * delta;
	
	set_transform(graph, entity_tfm, game_entity_id);
}

inline void
rotate_entity(scene_graph graph, u32 game_entity_id, f32 yaw, f32 pitch, f32 roll, f32 rotation_angle, v3 RotationAxis)
{
	
}

inline mat4
scale_entity(scene_graph graph, u32 game_entity_id, v3 scale_delta)
{
	transform entity_tfm = get_transform(graph, game_entity_id);
	entity_tfm.Scale = scale_delta;
	
	mat4 result = create_entity_scale_matrix(entity_tfm, entity_tfm.Scale);
	
	set_transform(graph, result, game_entity_id);
	
	return result;
}

inline mat4
get_transform_matrix(scene_graph graph, u32 game_entity_id)
{
	mat4 result = Identity();
	u32 transform_index = graph.entities[game_entity_id].transform_id;
	transform object_transform = graph.entities_transforms[transform_index];
	
	result.Value1_1 = object_transform.Scale.x;
	result.Value2_2 = object_transform.Scale.y;
	result.Value3_3 = object_transform.Scale.z;
	
	result.Value1_4 = object_transform.Position.x;
	result.Value2_4 = object_transform.Position.y;
	result.Value3_4 = object_transform.Position.z;
	
	TransposeMatrix(result);
	
	return result;
}