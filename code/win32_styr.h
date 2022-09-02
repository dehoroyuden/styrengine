#ifndef WIN32_STYR_H
//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	uint8 BytesPerPixel;
};

struct win32_state
{
	u64 TotalSize;
	void *GameMemoryBlock;
};

struct win32_game_code
{
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;
	
	// IMPORTANT(Denis): Either of the callbacks can be 0!
	// You must check before calling.
	game_update_and_render *UpdateAndRender;
	
	bool32 IsValid;
};

struct win32_transient_state
{
	uint64 StorageSize;
	void *Storage; // NOTE(Denis): REQUIRED to be celared to zero at startup.
};

struct QueueFamilyIndices
{
	u32 GraphicsFamily;
	u32 PresentFamily;
};

struct Array
{
	memory_index Size;
	u32 Count;
	void *Base;
};

struct UniformBufferObject
{
	__declspec(align(16)) glm::mat4 Model;
	__declspec(align(16)) glm::mat4 View;
	__declspec(align(16)) glm::mat4 Proj;
};

#define WIN32_STYR_H
#endif //WIN32_STYR_H
