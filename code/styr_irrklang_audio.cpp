#include <windows.h>
#include <conio.h>
#include "irrKlang/include/irrKlang.h"

#pragma comment(lib, "P:/code/irrKlang/lib/Winx64-visualStudio/irrKlang.lib") // link with irrKlang.dll

struct sound_props {
	irrklang::ISound* music;
	const char* filepath;
	u32 sound_id;
	v3 sound_pos;
	f32 sound_volume;
	f32 sound_min_distance;
	bool looped;
}sounds[64];

v3 get_camera_v3_position(transform* camera_transform) {
	return camera_transform->Position;
}
v3 get_camera_v3_front(transform* camera_transform) {
	return camera_transform->Front;
}

void create_customized_3D_sound(const char* filepath, v3 sound_absolute_pos_in_space, f32 sound_volume/*between 0 and 1*/, f32 sound_min_distance, bool looped) {
	int i = 0;
	for (i;i < sizeof(sounds) / sizeof(sound_props);i++) {
		if (sounds[i].filepath == nullptr)
			break;
		else if(i == (sizeof(sounds) / sizeof(sound_props))-1 && sounds[i].filepath != nullptr) {
			;
		}
	}
	sounds[i].sound_id = i;
	sounds[i].filepath = filepath;
	sounds[i].sound_pos = sound_absolute_pos_in_space;
	sounds[i].sound_volume = sound_volume;
	sounds[i].sound_min_distance = sound_min_distance;
	sounds[i].looped = looped;
}

irrklang::vec3df get_v3_to_vec3df(v3 vector) {
	irrklang::vec3df irrklang_vector = { vector.x, vector.y, vector.z };
	return irrklang_vector;
}

void play_3D_customized_sound(engine_state* EngineStatePtr, u32 id) {
	
	sounds[id].music = EngineStatePtr->sound_engine->play3D(sounds[id].filepath,get_v3_to_vec3df(sounds[id].sound_pos), sounds[id].looped, false, true);
	
	if (sounds[id].music)
		sounds[id].music->setMinDistance(sounds[id].sound_min_distance);
}