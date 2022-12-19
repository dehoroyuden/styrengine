#ifndef STYR_SCENE_GRAPH_H
#define STYR_SCENE_GRAPH_H

struct game_entity
{
	u32 mesh_id;
	u32 transform_id;
	u32 parent_entity_id; // NOTE(Denis): Zero means that our object has no parent object.
	char *name;
};

struct scene_graph
{
	u32 entity_count;
	game_entity *entities;
	transform *entities_transforms;
	mat4 *entities_model_matrices;
};

struct scene_graph_state
{
	u32 max_scene_graph_count;
	u32 scene_graph_count;
	scene_graph *scene_graphs;
};

#endif //STYR_SCENE_GRAPH_H
