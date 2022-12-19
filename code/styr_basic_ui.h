#ifndef STYR_BASIC_UI_H
#define STYR_BASIC_UI_H

struct basic_ui_state
{
	u32 max_vertices_count;
	u32 vertices_count;
	u32 indices_count;
	u32 quad_count;
	
	vertex_3d *vertices_buffer;
	u32 *indices_buffer;
};

#endif //STYR_BASIC_UI_H

struct write_line {
	rectangle2 pos_in_space;
	v2 size;
	v2 center;
	v4 color;
	v4 t_color;
	char data[1024];
	int data_length;
	char* name;
	bool acces;
	int step;
}lines[10];

struct write_line_buffer {
	v2 position;
	v2 l_position;
	v2 r_position;
	v4 color;
	char data[1024];
	int step;
	int l_index;
	int r_index;
	bool prew_direction;
	bool prew_direction_acces;
	
}wrl_buffer;

char real_buffer[1024] = {};