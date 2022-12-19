
internal basic_ui_state *
InitializeBasicUIState(memory_arena *basic_ui_state_mem, memory_arena *tran_memory, u32 max_vertices_count)
{
	basic_ui_state *Result = PushStruct(basic_ui_state_mem, basic_ui_state);
	Result->max_vertices_count = max_vertices_count;
	Result->quad_count = 0;
	Result->vertices_buffer = PushArray(tran_memory, max_vertices_count, vertex_3d);
	Result->indices_buffer = PushArray(tran_memory, (max_vertices_count / 4) * 6, u32);
	
	return Result;
}

// Space Dimensions in Pixels (Screen width -- Screen Height)
internal rectangle2
draw_quad_screen_space(basic_ui_state *state, u32 screen_width, u32 screen_height, v2 Origin, v2 Dim, v4 Color)
{
	rectangle2 rect = RectCenterDim(Origin, Dim);
	rect = screen_to_vulkan_coords_rect(rect, screen_width, screen_height);
	
	v3 vt1 = V3(rect.Min.x, rect.Min.y, 0);
	v3 vt2 = V3(rect.Max.x, rect.Min.y, 0);
	v3 vt3 = V3(rect.Min.x, rect.Max.y, 0);
	v3 vt4 = V3(rect.Max.x, rect.Max.y, 0);
	
	vertex_3d Vert1 = {vt1, Color, {0, 1.0f}};
	vertex_3d Vert2 = {vt2, Color, {1.0f, 1.0f}};
	vertex_3d Vert3 = {vt3, Color, {0, 0}};
	vertex_3d Vert4 = {vt4, Color, {1.0f, 0}};
	
	state->vertices_buffer[state->vertices_count++] = Vert1;
	state->vertices_buffer[state->vertices_count++] = Vert2;
	state->vertices_buffer[state->vertices_count++] = Vert3;
	state->vertices_buffer[state->vertices_count++] = Vert4;
	
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 0;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 3;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	
	++state->quad_count;
	
	return rect;
}

// Space Dimensions in Camera Space (0.0f -- 1.0f)
internal rectangle2
draw_quad_camera_space(basic_ui_state *state, v2 Origin, v2 Dim, v4 Color)
{
	rectangle2 rect = RectCenterDim(Origin, Dim);
	
	mat4 camera_space_matrix = CameraSpaceToVulkanSpace();
	rect = RectMulMat4x4(rect, camera_space_matrix);
	
	v3 vt1 = V3(rect.Min.x, rect.Min.y, 0);
	v3 vt2 = V3(rect.Max.x, rect.Min.y, 0);
	v3 vt3 = V3(rect.Min.x, rect.Max.y, 0);
	v3 vt4 = V3(rect.Max.x, rect.Max.y, 0);
	
	vertex_3d Vert1 = {vt1, Color, {0, 1.0f}};
	vertex_3d Vert2 = {vt2, Color, {1.0f, 1.0f}};
	vertex_3d Vert3 = {vt3, Color, {0, 0}};
	vertex_3d Vert4 = {vt4, Color, {1.0f, 0}};
	
	state->vertices_buffer[state->vertices_count++] = Vert1;
	state->vertices_buffer[state->vertices_count++] = Vert2;
	state->vertices_buffer[state->vertices_count++] = Vert3;
	state->vertices_buffer[state->vertices_count++] = Vert4;
	
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 0;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 1;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 3;
	state->indices_buffer[state->indices_count++] = state->quad_count * 4 + 2;
	
	++state->quad_count;
	
	return rect;
}

// TODO(Denis): Make state cleaning
inline void
basic_ui_end_frame(basic_ui_state *state)
{
	
}

internal void
create_writeline(v2 center, u32 WindowWidth, u32 WindowHeight, char* name, char* data) {
	int i = 0;

	for (i;lines[i].name != NULL;i++) {
		if (lines[i].name == name)
			return;
	}

	lines[i].size = v2{ 100.0f, 20.0f };
	lines[i].center = center;
	lines[i].color = v4{ 0.0f, 0.0f, 0.0f, 1.0f };
	lines[i].t_color = v4{ 0.6f, 0.6f, 0.6f, 1.0f };
	lines[i].name = name;

	int j = 0;
	for(j; data[j]!='\0'&&j < ArrayCount(lines[i].data);j++)
		lines[i].data[j] = data[j];

	lines[i].data_length = j;
	lines[i].acces = false;
	lines[i].pos_in_space = screen_to_vulkan_coords_rect(RectCenterDim(center, lines[i].size), WindowWidth, WindowHeight);
		
}

internal void
draw_writeline(write_line wrl, basic_ui_state* state, game_assets* assets, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {
	draw_quad_screen_space(state, screen_width, screen_height, wrl.center, wrl.size, wrl.color);

	f32 space_size = 12.0f;

	char line[128] = {};
	char letter[2] = " ";
	rectangle2 t_size = {};
	f32 t_len = 0.0f;

	if (wrl.data != NULL) {
		for (int i = 0;wrl.data[i + wrl.step] != '\0';i++) {
			letter[0] = wrl.data[i + wrl.step];
			line[i] = wrl.data[i + wrl.step];
			if (letter[0] != ' ') {
				t_size = DEBUGGetTextSize(assets, text_state, screen_width, screen_height, letter);
				t_len += t_size.Max.x - t_size.Min.x;
			}
			else
				t_len += space_size;

			if (t_len > wrl.size.x+2) {
				line[i] = '\0';
				break;
			}
		}
		DEBUGTextOutAt(assets, text_state, screen_width, screen_height, v2{ wrl.center.x - (wrl.size.x / 2) , wrl.center.y + 6 }, line, wrl.t_color);
	}
}

internal f32
count_length_of_marked_text(int l_index, int r_index, char* data, f32 max_border, game_assets* assets, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {

	f32 space_size = 12.0f;
	char letter[2] = " ";
	rectangle2 t_size = {};
	f32 t_len = 0.0f;

	for (int j = l_index;
		j - l_index != r_index - l_index;
		j++) {

		letter[0] = data[j];

		if (letter[0] != ' ') {
			t_size = DEBUGGetTextSize(assets, text_state, screen_width, screen_height, letter);
			t_len += t_size.Max.x - t_size.Min.x;
		}
		else
			t_len += space_size;
	}
	if (max_border != 0 && t_len >= max_border)
		return max_border;
	else
		return t_len;

}

internal void
change_step(bool direction, write_line* wrl, game_assets* assets, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {

	f32 space_size = 12.0f;
	char letter[2] = " ";
	rectangle2 t_size = {};
	f32 letter_size = 0.0f;


	if (wrl->data[wrl->step + wrl_buffer.step] == '\0' && direction) {
		return;
	}
	else if ((wrl->step + wrl_buffer.step) != 0 && !direction )
		letter[0] = wrl->data[wrl->step + wrl_buffer.step - 1];
	else
		letter[0] = wrl->data[wrl->step + wrl_buffer.step];



	if (letter[0] != ' ') {
		t_size = DEBUGGetTextSize(assets, text_state, screen_width, screen_height, letter);
		letter_size = (t_size.Max.x - t_size.Min.x);
	}
	else
		letter_size = space_size;

	if (direction && (wrl_buffer.position.x + letter_size) <= (wrl->center.x + (wrl->size.x / 2))) {
		wrl_buffer.position.x += letter_size;
		wrl_buffer.step++;

	}
	else if (!direction  && (wrl_buffer.position.x - letter_size) >= (wrl->center.x - (wrl->size.x / 2))) {
		wrl_buffer.position.x -= letter_size;
		wrl_buffer.step--;

	}
	else {
		int i = 0;
		for (i;wrl->data[i] != '\0';i++) {
		}

		if (direction  && wrl->step < (i - 1))
			wrl->step++;
		else if (!direction && wrl->step != 0)
			wrl->step--;
	}
}

internal void
clean_the_buffer() {
	wrl_buffer.l_index = 0;
	wrl_buffer.r_index = 0;
	wrl_buffer.l_position = {};
	wrl_buffer.r_position = {};

	wrl_buffer.prew_direction_acces = false;

	for (int i = 0;wrl_buffer.data[i]!='\0';i++)
		wrl_buffer.data[i] = '\0';
}

internal void
change_buffer_info(bool direction, write_line* wrl, game_assets* assets, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {

	int length = sizeof(wrl_buffer.data) / sizeof(char);
	f32 t_len = 0.0f;
	

	if (direction && wrl->data[wrl->step + wrl_buffer.step] == '\0' )
		return;
	else if (!direction  && (wrl->step + wrl_buffer.step) == 0 && wrl_buffer.l_index == 0)
		return;
	else if (wrl_buffer.prew_direction_acces == false) {
		wrl_buffer.prew_direction_acces = true;
		wrl_buffer.prew_direction = direction;
	}


	if ((wrl_buffer.r_index - wrl_buffer.l_index) == length && direction == wrl_buffer.prew_direction)
		return;


	if (direction) {
		if(direction == wrl_buffer.prew_direction && wrl_buffer.prew_direction_acces == true)
		{
			if (wrl_buffer.l_index == 0 && wrl_buffer.data[0] == '\0')
				wrl_buffer.l_index = wrl->step + wrl_buffer.step;
				

			change_step(direction, wrl, assets, text_state, screen_width, screen_height);

			wrl_buffer.r_index = wrl->step + wrl_buffer.step;

			wrl_buffer.r_position = wrl_buffer.position;

			if (wrl_buffer.l_index - wrl->step >= 0) {
				t_len = count_length_of_marked_text(wrl_buffer.l_index, wrl->step + wrl_buffer.step, wrl->data, wrl->size.x, assets, text_state, screen_width, screen_height);
				wrl_buffer.l_position = v2{ (wrl_buffer.r_position.x - t_len),wrl_buffer.position.y };
			}
			else {
				wrl_buffer.l_position = v2{ wrl->center.x - (wrl->size.x/2),wrl_buffer.position.y};
			}

			wrl_buffer.data[wrl_buffer.r_index - wrl_buffer.l_index - 1] = wrl->data[wrl_buffer.r_index - 1];
		}
		else if (direction != wrl_buffer.prew_direction && wrl_buffer.prew_direction_acces == true) {

			change_step(direction, wrl, assets, text_state, screen_width, screen_height);

			wrl_buffer.l_index = wrl->step + wrl_buffer.step;
			wrl_buffer.l_position = wrl_buffer.position;

			for (int i = 0 ;
				i <= wrl_buffer.r_index - wrl_buffer.l_index;
				i++)
				wrl_buffer.data[i] = wrl_buffer.data[i + 1];
		}
	}
	else if (!direction) {
		if (direction == wrl_buffer.prew_direction && wrl_buffer.prew_direction_acces == true) {
			if (wrl_buffer.r_index == 0 && wrl_buffer.data[0] == '\0')
				wrl_buffer.r_index = wrl->step + wrl_buffer.step;

			change_step(direction, wrl, assets, text_state, screen_width, screen_height);

			wrl_buffer.l_index = wrl->step + wrl_buffer.step;

			wrl_buffer.l_position = wrl_buffer.position;

			t_len = count_length_of_marked_text(wrl_buffer.l_index, wrl_buffer.r_index, wrl->data, wrl->size.x, assets, text_state, screen_width, screen_height);
			wrl_buffer.r_position = v2{ (wrl_buffer.l_position.x + t_len),wrl_buffer.position.y };

			for (int i = wrl_buffer.r_index - wrl_buffer.l_index - 1;
				i > 0;
				i--)
				wrl_buffer.data[i] = wrl_buffer.data[i - 1];
			wrl_buffer.data[0] = wrl->data[wrl_buffer.l_index];
		}
		else if (direction != wrl_buffer.prew_direction && wrl_buffer.prew_direction_acces == true) {

			change_step(direction, wrl, assets, text_state, screen_width, screen_height);

			wrl_buffer.r_index = wrl->step + wrl_buffer.step;
			wrl_buffer.r_position = wrl_buffer.position;

			wrl_buffer.data[wrl_buffer.r_index - wrl_buffer.l_index] = '\0';
		}
	}

	if (wrl_buffer.l_index == wrl_buffer.r_index && wrl_buffer.prew_direction_acces == true)
	{
		clean_the_buffer();
	}
}

internal void
delete_data(write_line* wrl, game_assets* assets, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {

	f32 t_len = 0.0f;

	if (wrl_buffer.data[0] != '\0') {

		int i = wrl_buffer.r_index;

		if (wrl_buffer.prew_direction == true) {
			for(int j = wrl_buffer.r_index - wrl_buffer.l_index;
				j!=0;
				j--)
				change_step(false, wrl, assets, text_state, screen_width, screen_height);
		}

		for (i;
			wrl->data[i] != '\0';
			i++) {
			wrl->data[i - (wrl_buffer.r_index - wrl_buffer.l_index)] = wrl->data[i];
		}

		for (i;
			wrl->data[i - (wrl_buffer.r_index - wrl_buffer.l_index)] != '\0';
			i++)
			wrl->data[i - (wrl_buffer.r_index - wrl_buffer.l_index)] = '\0';

		wrl->data_length -= wrl_buffer.r_index - wrl_buffer.l_index;

		clean_the_buffer();
	}
	else if (wrl->step + wrl_buffer.step != 0) {
		if (wrl->step != 0) {
			wrl->step--;
			for (int i = wrl->step + wrl_buffer.step; wrl->data[i] != '\0';i++)
				wrl->data[i] = wrl->data[i + 1];
		}
		else {
			change_step(false, wrl, assets, text_state, screen_width, screen_height);
			for (int i = wrl->step + wrl_buffer.step; wrl->data[i] != '\0';i++)
				wrl->data[i] = wrl->data[i + 1];
		}
		wrl->data_length--;
	}

}

internal void
copy_data_to_real_buffer(bool cut, write_line* wrl, game_assets* assets, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {
	int i = 0;

	for (i; wrl_buffer.data[i] != '\0'; i++)
		real_buffer[i] = wrl_buffer.data[i];

	if (real_buffer[i] != '\0')
		for (i;real_buffer[i] != '\0';i++)
			real_buffer[i] = '\0';

	if (cut == false) {
		return;
	}
	else {
		delete_data(wrl, assets, text_state, screen_width, screen_height);
		return;
	}
}

internal void 
paste_data_to_line(write_line* wrl, game_assets* assets, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {
	int i = 0;
	int j = 0;

	if (wrl_buffer.data[0] != '\0') {
		delete_data(wrl, assets, text_state, screen_width, screen_height);
	}

	for (i;real_buffer[i] != '\0';i++);
	for (j;wrl->data[j] != '\0';j++);

	if (i + j > ArrayCount(wrl->data))//not more than length of line where it should be written
		return;
	else {
		int k = j;
		for (k;k >=(wrl->step+wrl_buffer.step);k--)
			wrl->data[j+i] = wrl->data[j--];
		for (k = 0; real_buffer[k] != '\0';k++) {
			wrl->data[wrl->step + wrl_buffer.step] = real_buffer[k];
			change_step(true, wrl, assets, text_state, screen_width, screen_height);
		}
			
	}
	

}

inline void 
write_data_in_line(write_line* wrl, char data, game_assets* assets, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {
	if (wrl_buffer.data[0] != '\0') {
		delete_data(wrl, assets, text_state, screen_width, screen_height);

		int i = 0;
		int j = 0;
		int length = sizeof(wrl->data) / sizeof(char);

		for (int i = wrl->step + wrl_buffer.step;
			wrl->data[i] != '\0';
			i++)
			j = i;
		if (i == length - 1) {
			//error to much data (more than 1024 char)
			return;
		}
		else {
			for (j;
				j >= wrl->step + wrl_buffer.step;
				j--)
				wrl->data[j + 1] = wrl->data[j];
			wrl->data[wrl->step + wrl_buffer.step] = data;
		}
	}
	else {

		int i = 0;
		int j = 0;
		int length = sizeof(wrl->data) / sizeof(char);

		for (int i = wrl->step + wrl_buffer.step;
			wrl->data[i] != '\0';
			i++)
			j = i;
		if (i == length - 1) {
			//error to much data (more than 1024 char)
			return;
		}
		else {
			for (j;
				j >= wrl->step + wrl_buffer.step;
				j--)
				wrl->data[j + 1] = wrl->data[j];
			wrl->data[wrl->step + wrl_buffer.step] = data;
		}
	}
	change_step(true, wrl, assets, text_state, screen_width, screen_height);

}

internal void
work_with_text(write_line* wrl, basic_ui_state* state, game_assets* assets, game_input* Input, basic_ui_text_state* text_state, u32 screen_width, u32 screen_height) {
	if (wrl_buffer.l_index == 0 && wrl_buffer.r_index == 0 && wrl_buffer.step == 0) {
		wrl_buffer.position = v2{ wrl->center.x - (wrl->size.x / 2) , wrl->center.y };
		draw_quad_screen_space(state, screen_width, screen_height, wrl_buffer.position, v2{1,20}, v4{1.0f,1.0f, 1.0f, 1.0f, });
	}
	else if (wrl_buffer.l_index == 0 && wrl_buffer.r_index == 0 && wrl_buffer.step != 0) {
		draw_quad_screen_space(state, screen_width, screen_height, wrl_buffer.position, v2{ 2,20 }, v4{ 1.0f,1.0f, 1.0f, 1.0f, });
	}
	else {
		//left
		draw_quad_screen_space(state, screen_width, screen_height, v2{ wrl_buffer.l_position.x+2,wrl_buffer.l_position.y - 9 }, v2{ 5,1 }, v4{ 1.0f,0.3f, 0.3f, 1.0f });
		draw_quad_screen_space(state, screen_width, screen_height, v2{ wrl_buffer.l_position.x, wrl_buffer.l_position.y - 5 }, v2{ 1,10 }, v4{ 1.0f,0.3f, 0.3f, 1.0f });

		//right
		draw_quad_screen_space(state, screen_width, screen_height, v2{ wrl_buffer.r_position.x - 2,wrl_buffer.r_position.y + 9 }, v2{ 5,1 }, v4{ 1.0f,0.3f, 0.3f, 1.0f });
		draw_quad_screen_space(state, screen_width, screen_height, v2{ wrl_buffer.r_position.x,wrl_buffer.r_position.y + 5 }, v2{ 1,10 }, v4{ 1.0f,0.3f, 0.3f, 1.0f });
	}

	for (int ControllerIndex = 0;
		ControllerIndex < ArrayCount(Input->Controllers);
		++ControllerIndex)
	{
		game_controller_input* Controller = GetController(Input, ControllerIndex);

		if (Controller->LCtrl.EndedDown && WasPressed(Controller->ButtonC))
		{
			copy_data_to_real_buffer(false, wrl, assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LCtrl.EndedDown && WasPressed(Controller->ButtonX))
		{
			copy_data_to_real_buffer(true, wrl, assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LCtrl.EndedDown && WasPressed(Controller->ButtonV))
		{
			paste_data_to_line(wrl, assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ArrowRight))
		{
			change_buffer_info(true, wrl, assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ArrowLeft))
		{
			change_buffer_info(false, wrl,assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ArrowRight))
		{
			change_step(true, wrl, assets, text_state, screen_width, screen_height);
			clean_the_buffer();
			break;
		}
		if (WasPressed(Controller->ArrowLeft))
		{
			change_step(false, wrl, assets, text_state, screen_width, screen_height);
			clean_the_buffer();
			break;
		}
		if (WasPressed(Controller->BackSpace))
		{
			delete_data(wrl, assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->RotateRight))
		{
			write_data_in_line(wrl, 'Q', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->MoveUp))
		{
			write_data_in_line(wrl, 'W', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->RotateLeft))
		{
			write_data_in_line(wrl, 'E', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonR))
		{
			write_data_in_line(wrl, 'R', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonT))
		{
			write_data_in_line(wrl, 'T', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonY))
		{
			write_data_in_line(wrl, 'Y', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonU))
		{
			write_data_in_line(wrl, 'U', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonI))
		{
			write_data_in_line(wrl, 'I', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonO))
		{
			write_data_in_line(wrl, 'O', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonP))
		{
			write_data_in_line(wrl, 'P', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button1afterP))
		{
			write_data_in_line(wrl, '{', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button2afterP))
		{
			write_data_in_line(wrl, '}', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button3afterP))
		{
			write_data_in_line(wrl, '|', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->MoveLeft))
		{
			write_data_in_line(wrl, 'A', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->MoveDown))
		{
			write_data_in_line(wrl, 'S', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->MoveRight))
		{
			write_data_in_line(wrl, 'D', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonF))
		{
			write_data_in_line(wrl, 'F', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonG))
		{
			write_data_in_line(wrl, 'G', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonH))
		{
			write_data_in_line(wrl, 'H', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonJ))
		{
			write_data_in_line(wrl, 'J', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonK))
		{
			write_data_in_line(wrl, 'K', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonL))
		{
			write_data_in_line(wrl, 'L', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button1afterL))
		{
			write_data_in_line(wrl, ':', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button2afterL))
		{
			write_data_in_line(wrl, '"', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonZ))
		{
			write_data_in_line(wrl, 'Z', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonX))
		{
			write_data_in_line(wrl, 'X', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonC))
		{
			write_data_in_line(wrl, 'C', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonV))
		{
			write_data_in_line(wrl, 'V', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonB))
		{
			write_data_in_line(wrl, 'B', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonN))
		{
			write_data_in_line(wrl, 'N', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->ButtonM))
		{
			write_data_in_line(wrl, 'M', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button1afterM))
		{
			write_data_in_line(wrl, '<', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button2afterM))
		{
			write_data_in_line(wrl, '>', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button3afterM))
		{
			write_data_in_line(wrl, '?', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button1))
		{
			write_data_in_line(wrl, '!', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button2))
		{
			write_data_in_line(wrl, '@', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button3))
		{
			write_data_in_line(wrl, '#', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button4))
		{
			write_data_in_line(wrl, '$', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button5))
		{
			write_data_in_line(wrl, '%', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button6))
		{
			write_data_in_line(wrl, '^', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button7))
		{
			write_data_in_line(wrl, '&', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button8))
		{
			write_data_in_line(wrl, '*', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button9))
		{
			write_data_in_line(wrl, '(', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button0))
		{
			write_data_in_line(wrl, ')', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button1after0))
		{
			write_data_in_line(wrl, '_', assets, text_state, screen_width, screen_height);
			break;
		}
		if (Controller->LShift.EndedDown && WasPressed(Controller->Button2after0))
		{
			write_data_in_line(wrl, '+', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->RotateRight))
		{
			write_data_in_line(wrl,'q', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->MoveUp))
		{
			write_data_in_line(wrl, 'w', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->RotateLeft))
		{
			write_data_in_line(wrl, 'e', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonR))
		{
			write_data_in_line(wrl, 'r', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonT))
		{
			write_data_in_line(wrl, 't', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonY))
		{
			write_data_in_line(wrl, 'y', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonU))
		{
			write_data_in_line(wrl, 'u', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonI))
		{
			write_data_in_line(wrl, 'i', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonO))
		{
			write_data_in_line(wrl, 'o', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonP))
		{
			write_data_in_line(wrl, 'p', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button1afterP))
		{
			write_data_in_line(wrl, '[', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button2afterP))
		{
			write_data_in_line(wrl, ']', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button3afterP))
		{
			write_data_in_line(wrl, '\\', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->MoveLeft))
		{
			write_data_in_line(wrl, 'a', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->MoveDown))
		{
			write_data_in_line(wrl, 's', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->MoveRight))
		{
			write_data_in_line(wrl, 'd', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonF))
		{
			write_data_in_line(wrl, 'f', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonG))
		{
			write_data_in_line(wrl, 'g', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonH))
		{
			write_data_in_line(wrl, 'h', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonJ))
		{
			write_data_in_line(wrl, 'j', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonK))
		{
			write_data_in_line(wrl, 'k', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonL))
		{
			write_data_in_line(wrl, 'l', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button1afterL))
		{
			write_data_in_line(wrl, ';', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button2afterL))
		{
			write_data_in_line(wrl, '\'', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonZ))
		{
			write_data_in_line(wrl, 'z', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonX))
		{
			write_data_in_line(wrl, 'x', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonC))
		{
			write_data_in_line(wrl, 'c', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonV))
		{
			write_data_in_line(wrl, 'v', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonB))
		{
			write_data_in_line(wrl, 'b', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonN))
		{
			write_data_in_line(wrl, 'n', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonM))
		{
			write_data_in_line(wrl, 'm', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button1afterM))
		{
			write_data_in_line(wrl, ',', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button2afterM))
		{
			write_data_in_line(wrl, '.', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button3afterM))
		{
			write_data_in_line(wrl, '/', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button1))
		{
			write_data_in_line(wrl, '1', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button2))
		{
			write_data_in_line(wrl, '2', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button3))
		{
			write_data_in_line(wrl, '3', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button4))
		{
			write_data_in_line(wrl, '4', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button5))
		{
			write_data_in_line(wrl, '5', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button6))
		{
			write_data_in_line(wrl, '6', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button7))
		{
			write_data_in_line(wrl, '7', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button8))
		{
			write_data_in_line(wrl, '8', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button9))
		{
			write_data_in_line(wrl, '9', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button0))
		{
			write_data_in_line(wrl, '0', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button1after0))
		{
			write_data_in_line(wrl, '-', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->Button2after0))
		{
			write_data_in_line(wrl, '=', assets, text_state, screen_width, screen_height);
			break;
		}
		if (WasPressed(Controller->ButtonSpace))
		{
			write_data_in_line(wrl, ' ', assets, text_state, screen_width, screen_height);
			break;
		}
	}

}