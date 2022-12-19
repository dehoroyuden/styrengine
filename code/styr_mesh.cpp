internal meshes_state *
InitializeMeshes(memory_arena *MeshesChunk, memory_arena *TranMemory, u32 max_mesh_count)
{
	meshes_state *Result = PushStruct(MeshesChunk, meshes_state);
	
	Result->mesh_count = 0;
	Result->max_mesh_count = max_mesh_count;
	Result->meshes_array = PushArray(TranMemory, max_mesh_count, mesh);
	
	return Result;
}

internal void
CreateMesh(char *name, u32 loaded_mesh_index, u32 material_id, meshes_state *State, game_assets *Assets)
{
	mesh Result = {};
	Result.material_index = material_id;
	Result.name = name;
	
	u32 mesh_ID = GetMatchAssetFrom(Assets, Asset_Mesh, loaded_mesh_index);
	loaded_mesh *Mesh = GetMesh(Assets, {mesh_ID});
	
	if(!Mesh)
	{
		LoadMesh(Assets, {mesh_ID});
		Mesh = GetMesh(Assets, {mesh_ID});
	}
	
	Result.loaded_mesh_data = *Mesh;
	
	State->meshes_array[State->mesh_count++] = Result;
}

internal void
CreateMesh(char *name, u32 vertex_count, u32 index_count, vertex_3d *vertices, u32 *indices, u32 material_id, meshes_state *State)
{
	mesh Result = {};
	Result.material_index = material_id;
	Result.name = name;
	
	loaded_mesh mesh = {};
	mesh.VerticesCount = vertex_count;
	mesh.IndicesCount = index_count;
	mesh.Indices = indices;
	mesh.Vertices = vertices;
	mesh.SubObjectsCount = 0;
	
	Result.loaded_mesh_data = mesh;
	State->meshes_array[State->mesh_count++] = Result;
}

inline mesh
GetMeshByID(meshes_state state, u32 id)
{
	Assert(id < state.mesh_count);
	mesh Result = state.meshes_array[id];
	
	return Result;
}