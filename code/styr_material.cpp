
internal material_state *
InitializeMaterials(memory_arena *MemoryChunk, memory_arena *TranMemory, u32 max_material_count)
{
	
	material_state *Result = PushStruct(MemoryChunk, material_state);
	Result->materials_array = PushArray(TranMemory, max_material_count, material);
	
	ZeroSize(max_material_count * sizeof(material), Result->materials_array);
	
	return (Result);
}

internal material
CreateMaterial(char *Name, material_type Type, u32 texture_index, material_state *MatState)
{
	material Result = {};
	
	if(MatState->materials_array)
	{
		material Result = {};
		Result.Type = Type;
		Result.Name = Name;
		Result.material_index = MatState->material_count;
		Result.texture_index = texture_index;
		
		MatState->materials_array[MatState->material_count++] = Result;
	}
	
	return Result;
}

internal material
FindMaterialById(u32 ID, material_state state)
{
	material Result = {};
	
	Result = state.materials_array[ID];
	
	return Result;
}