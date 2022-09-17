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

struct queue_family_indices
{
	u32 GraphicsFamily;
	u32 PresentFamily;
};

// NOTE(Denis): This in future will be the linked list array!
/*struct win32_styr_array_element
{
	void *Data;
	win32_styr_array_element *Next;
};*/

struct win32_styr_array
{
	b32 IsInitialized;
	memory_index Size;
	u32 Count;
	void *Data;
};

// NOTE(Denis): Initialize and _allocate memory from memory chunk_ array which is pointer
#define initialize_array(array, memory_chunk_pointer, count, type)\
(array).Data = PushArray(memory_chunk_pointer, count, type);\
(array).Size = 0;\
(array).Count = count;\
(array).IsInitialized = true;

// NOTE(Denis): Add element to array into first free slot and increment index
#define push_array_element(pointer_to_array, data, type)\
*((type *)((u8 *)pointer_to_array->Data + pointer_to_array->Size)) = data;\
pointer_to_array->Size += sizeof(type);\
++pointer_to_array->Count;

// NOTE(Denis): Add element to array into position by index
#define push_array_element_at_index(pointer_to_array, index, data, type)\
*((type *)((u8 *)pointer_to_array->Data + (sizeof(type) * index))) = data;\
pointer_to_array->Size += sizeof(type);\
++pointer_to_array->Count;

// NOTE(Denis): Get element by its index
#define get_array_element(pointer_to_array, index, type)\
*(type *)((u8 *)pointer_to_array->Data + (index * sizeof(type)));

#define get_array_pointer(array, type) (type *)((array).Data)

#define WIN32_STYR_H
#endif //WIN32_STYR_H
