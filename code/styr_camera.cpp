//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

internal styr_camera_state *
initialize_camera_state(memory_arena *camera_state_mem, memory_arena *tran_memory, u32 max_cameras_count)
{
	styr_camera_state *Result = PushStruct(camera_state_mem, styr_camera_state);
	Result->max_cameras_count = max_cameras_count;
	Result->current_cameras_count = 0;
	Result->cameras = PushArray(tran_memory, max_cameras_count, styr_camera);
	
	for(u32 i = 0;
		i < Result->max_cameras_count;
		++i)
	{
		Result->cameras[i].entity_id |= (0x00000001 << 31);
	}
	
	return Result;
}

internal styr_camera *
get_camera_ptr_by_id(styr_camera_state state, u32 camera_id)
{
	styr_camera *Result = {};
	
	Result = &state.cameras[camera_id];
	
	return Result;
}

internal styr_camera
get_camera_by_id(styr_camera_state state, u32 camera_id)
{
	styr_camera Result = {};
	
	Result = state.cameras[camera_id];
	
	return Result;
}

inline transform
default_camera_transform()
{
	transform Result = {};
	
	Result.Position = V3(11,12,5);
	Result.Front = {0, -1, 0};
	Result.Right = {1, 0, 0};
	Result.Up = {0, 0, 1};
	Result.Yaw = -128.0f;
	Result.Pitch = -15.0f;
	
	return Result;
}

inline styr_camera
default_camera_settings(styr_camera_state state, u32 camera_id, transform default_transform)
{
	v3 CameraPos = default_transform.Position;
	v3 CameraUp = default_transform.Up;
	
	v3 Front = default_transform.Front;
	Front.x = Cos(RadiansFromDegrees(default_transform.Yaw)) * Cos(RadiansFromDegrees(default_transform.Pitch));
	Front.y = Sin(RadiansFromDegrees(default_transform.Yaw)) * Cos(RadiansFromDegrees(default_transform.Pitch));
	Front.z = Sin(RadiansFromDegrees(default_transform.Pitch));
	default_transform.Front = Normalize(Front);
	default_transform.Right = Normalize(CrossProduct(CameraUp, default_transform.Front));
	
	v3 CPF = CameraPos + (default_transform.Front);
	glm::vec3 LookAtMe = glm::vec3(CPF.x, CPF.y, CPF.z);
	
	mat4 View = ConvertToMatrix4x4(glm::lookAt(glm::vec3(CameraPos.x, CameraPos.y, CameraPos.z), LookAtMe, glm::vec3(CameraUp.x, CameraUp.y, CameraUp.z)));
	styr_camera default_camera = {};
	default_camera.view_mat4x4 = View;
	
	return default_camera;
}

internal u32
create_perspective_camera(scene_graph *scene_graph, styr_camera_state *state, u8 fov, f32 screen_width, f32 screen_height, f32 near_plane, f32 far_plane)
{
	Assert(state->current_cameras_count < state->max_cameras_count);
	
	u32 camera_id = state->current_cameras_count;
	styr_camera camera = default_camera_settings(*state, camera_id, default_camera_transform());
	
	camera.projection_mat4x4 = PerspectiveProjection(RadiansFromDegrees(fov), screen_width / screen_height, near_plane, far_plane);
	camera.CameraSpeed = 2.5f;
	camera.CameraBlend = 0;
	camera.MaxCameraSpeed = 40.0f;
	camera.MinCameraSpeed = 1.0f;
	camera.entity_id = CreateGameEntity(scene_graph, 0, "Main Camera Perspective");
	state->current_cameras_count++;
	state->cameras[camera_id] = camera;
	
	return camera_id;
}

internal u32
create_ortho_camera(scene_graph *scene_graph, styr_camera_state *state, u32 orthographic_scale, f32 screen_width, f32 screen_height, f32 near_plane, f32 far_plane)
{
	Assert(state->current_cameras_count < state->max_cameras_count);
	
	u32 camera_id = state->current_cameras_count;
	styr_camera camera = default_camera_settings(*state, camera_id, default_camera_transform());
	
	camera.projection_mat4x4 = OrthographicProjection(orthographic_scale, screen_width, screen_height, near_plane, far_plane);
	camera.CameraSpeed = 2.5f;
	camera.CameraBlend = 0;
	camera.MaxCameraSpeed = 40.0f;
	camera.MinCameraSpeed = 1.0f;
	camera.entity_id = CreateGameEntity(scene_graph, 0, "Main Camera Orthographic");
	state->current_cameras_count++;
	state->cameras[camera_id] = camera;
	
	return camera_id;
}

inline u32
get_default_camera_index()
{
	u32 result = 0;
	
	return result;
}

inline u32
get_camera_transform_id(styr_camera_state state, u32 camera_id)
{
	u32 result = {};
	result = state.cameras[camera_id].entity_id;
	
	return result;
}

inline styr_camera
set_camera_fly_speed(styr_camera_state state, u32 camera_id, f32 MouseZ)
{
	styr_camera result = get_camera_by_id(state, camera_id);
	result.CameraBlend = Clamp01(result.CameraBlend + MouseZ * 6);
	result.CameraSpeed = Lerp(result.MinCameraSpeed, result.CameraBlend, result.MaxCameraSpeed);
	
	return result;
}

inline transform
RotateCamera(scene_graph graph, styr_camera_state camera_state, u32 camera_id, f32 yaw, f32 pitch, f32 roll, f32 rotation_angle, v3 RotationAxis)
{
	transform new_camera_transform = get_transform(graph, get_camera_by_id(camera_state, camera_id).entity_id);
	
	v3 CameraPos = new_camera_transform.Position;
	v3 CameraFront = new_camera_transform.Front;
	v3 CameraRight = new_camera_transform.Right;
	v3 CameraUp = new_camera_transform.Up;
	
	f32 Yaw = new_camera_transform.Yaw;
	f32 Pitch = new_camera_transform.Pitch;
	f32 pitch_limit = 80.0f;
	
	new_camera_transform.Yaw -= yaw;
	new_camera_transform.Pitch = Clamp(-pitch_limit, new_camera_transform.Pitch - pitch, pitch_limit);
	new_camera_transform.Roll -= roll;
	
	v3 Front = {};
	Front.x = Cos(RadiansFromDegrees(Yaw)) * Cos(RadiansFromDegrees(Pitch));
	Front.y = Sin(RadiansFromDegrees(Yaw)) * Cos(RadiansFromDegrees(Pitch));
	Front.z = Sin(RadiansFromDegrees(Pitch));
	CameraFront = Normalize(Front);
	
	v3 Right = {};
	Right = Normalize(CrossProduct(CameraUp, CameraFront));
	
	v3 CPF = CameraPos + (CameraFront);
	glm::vec3 LookAtMe = glm::vec3(CPF.x, CPF.y, CPF.z);
	
	// Shift camera point of rotation (model matrix)
	mat4 EyesPositionMatrix = ConvertToMatrix4x4(glm::translate(glm::mat4(1.0f), glm::vec3(CameraFront.x, CameraFront.y, CameraFront.z)* -1.0f));
	
	new_camera_transform.Right = Right;
	new_camera_transform.Position = CameraPos;
	new_camera_transform.Front = CameraFront;
	
	mat4 View = ConvertToMatrix4x4(glm::lookAt(glm::vec3(CameraPos.x, CameraPos.y, CameraPos.z), LookAtMe, glm::vec3(CameraUp.x, CameraUp.y, CameraUp.z)));
	get_camera_ptr_by_id(camera_state, camera_id)->view_mat4x4 = View;// * EyesPositionMatrix;
	
	//set_transform(graph, new_camera_transform, camera_state.cameras[camera_id].entity_id);
	return new_camera_transform;
}

inline transform
MoveCamera(scene_graph graph, styr_camera_state camera_state, u32 camera_id, v3 delta_position)
{
	transform camera_transform = get_transform(graph, get_camera_by_id(camera_state, camera_id).entity_id);
	camera_transform.Position += delta_position;
	
	set_transform(graph, camera_transform, camera_state.cameras[camera_id].entity_id);
	return camera_transform;
}