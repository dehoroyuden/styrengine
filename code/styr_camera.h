#ifndef STYR_CAMERA_H
#define STYR_CAMERA_H

struct styr_camera
{
	mat4 projection_mat4x4;
	mat4 view_mat4x4;
	
	f32 CameraSpeed;
	f32 CameraBlend;
	f32 MaxCameraSpeed;
	f32 MinCameraSpeed;
	
	u32 entity_id;
};

struct styr_camera_state
{
	u32 max_cameras_count;
	u32 current_cameras_count;
	
	styr_camera *cameras;
};

#endif //STYR_CAMERA_H
