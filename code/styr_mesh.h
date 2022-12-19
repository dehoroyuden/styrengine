#ifndef STYR_MESH_H
#define STYR_MESH_H

struct mesh
{
	u32 mesh_index;
	u32 material_index;
	
	loaded_mesh loaded_mesh_data;
	char *name;
};

struct meshes_state
{
	u32 mesh_count;
	u32 max_mesh_count;
	mesh *meshes_array;
};

#endif //STYR_MESH_H
