#ifndef STYR_MATERIAL_H
#define STYR_MATERIAL_H

enum material_type
{
	MaterialType_Default,
	MaterialType_Diffuse,
	MaterialType_PBR_Metal_Roughness_Diffuse,
	MaterialType_PBR_Specular_Glossiness_Albedo,
	MaterialType_Unlit,
	MaterialType_UI,
	MaterialType_UIText,
	MaterialType_WorldGrid,
	
	MaterialType_Count,
};

struct material
{
	char *Name;
	u32 material_index;
	u32 texture_index;
	material_type Type;
};

struct material_state
{
	u32 material_count;
	u32 bitmaps_count;
	material *materials_array;
	loaded_bitmap *bitmaps_array;
};

#endif //STYR_MATERIAL_H
