/* date = November 1st 2022 2:02 pm */

#ifndef STYR_TRANSFORM_H
#define STYR_TRANSFORM_H

struct quaternion
{
	f32 Angle;
	f32 x;
	f32 y;
	f32 z;
};

struct transform
{
	v3 Position;
	v3 Scale;
	v3 Front;
	v3 Right;
	v3 Up;
	
	quaternion Rotation;
	
	f32 Pitch;
	f32 Yaw;
	f32 Roll;
};

#endif //STYR_TRANSFORM_H
