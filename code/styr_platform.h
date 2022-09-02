#ifndef STYR_PLATFORM_H
//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif
	
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif
	
#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif
	
#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
	// TODO(Denis): More compilers!!!!
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif
	
#if COMPILER_MSVC
#include <intrin.h>
	//#pragma intrinsic(_BitScanForward)
#endif
	
	//
	// NOTE(Denis): Types
	//
	
#include <stdint.h>
#include <float.h>
	
#if STYR_INTERNAL
#include <stdio.h>
#endif
	
	// TODO(Denis): Implement sine ourselves
	
	typedef uint8_t  uint8;
	typedef uint16_t  uint16;
	typedef uint32_t  uint32;
	typedef uint64_t  uint64;
	
	typedef int8_t  int8;
	typedef int16_t  int16;
	typedef int32_t  int32;
	typedef int64_t  int64;
	typedef int32 bool32;
	
	typedef intptr_t intptr;
	typedef uintptr_t uintptr;
	
	typedef size_t memory_index;
	
	typedef float real32;
	typedef double real64;
	
	typedef uint8 u8;
	typedef uint16 u16;
	typedef uint32 u32;
	typedef uint64 u64;
	
	typedef int8 s8;
	typedef int16 s16;
	typedef int32 s32;
	typedef int64 s64;
	typedef bool32 b32;
	
	typedef real32 r32;
	typedef real64 r64;
	
	typedef uintptr_t umm;
	
#define U32FromPointer(Pointer) ((u32)(memory_index)(Pointer))
	
#define Real32Maximum FLT_MAX
#define Real32Minimum -FLT_MAX
	
#if !defined(internal)
#define internal static
#endif
#define local_presist static
#define global_variable static
	
#define Pi32 3.14159265359f
#define INT_MAX 2147483647
#define Tau32 6.283185307179f
#define RadianInDegrees 57.2957795131f
#define DegreeInRadians 0.01745329251f
	
#if STYR_VULKAN_VALIDATION
#define VulkanErrorMessage(Expression)\
Expression == 0 ? "VK_SUCCESS" :  Expression == 1 ? "VK_NOT_READY" : Expression == 2 ? "VK_TIMEOUT" : Expression == 3 ? "VK_EVENT_SET" : Expression == 4 ? "VK_EVENT_RESET" : Expression == 5 ? "VK_INCOMPLETE" : Expression == -1 ? "VK_ERROR_OUT_OF_HOST_MEMORY" : Expression == -2 ? "VK_ERROR_OUT_OF_DEVICE_MEMORY" : Expression == -3 ? "VK_ERROR_INITIALIZATION_FAILED" : Expression == -4 ? "VK_ERROR_DEVICE_LOST" : Expression == -5 ? "VK_ERROR_MEMORY_MAP_FAILED" : Expression == -6 ? "VK_ERROR_LAYER_NOT_PRESENT" : Expression == -7 ? "VK_ERROR_EXTENSION_NOT_PRESENT" : Expression == -8 ? "VK_ERROR_FEATURE_NOT_PRESENT" : Expression == -9 ? "VK_ERROR_INCOMPATIBLE_DRIVER" : Expression == -10 ? "VK_ERROR_TOO_MANY_OBJECTS" : Expression == -11 ? "VK_ERROR_FORMAT_NOT_SUPPORTED" : Expression == -12 ? "VK_ERROR_FRAGMENTED_POOL" : Expression == -12 ? "VK_ERROR_FRAGMENTED_POOL" : "VK_ERROR_UNKNOWN"
	
#define Assert_(Expression, type) \
TextBuffer[256];\
_snprintf_s(TextBuffer,sizeof(TextBuffer), type, VulkanErrorMessage((Expression)));\
MessageBox(NULL, TextBuffer, "caption", 0);
	
#define Assert_NotVulkan(Expression, type) \
TextBuffer[256];\
_snprintf_s(TextBuffer,sizeof(TextBuffer), type, Expression);\
MessageBox(NULL, TextBuffer, "caption", 0);
	
#else
#define Assert_(Expression,...) Expression;
#define Assert_NotVulkan(Expression, type)
#endif
	
#if STYR_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#define Assert_(Expression,...) Expression;
#endif
	
#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break
	
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
	
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
	
#define AlignPow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Alignment) - 1))
#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)
	
	inline uint32 SafeTruncateUInt64(uint64 Value)
	{
		// TODO(Denis): Defines for maximum values
		Assert(Value <= 0xFFFFFFFF);
		uint32 Result = (uint32)Value;
		return(Result);
	}
	
	struct platform_work_queue;
	
	typedef struct platform_file_handle
	{
		b32 NoErrors;
		void *Platform;
	}  platform_file_handle;
	
#define PLATFORM_ALLOCATE_MEMORY(name) void *name(memory_index Size)
	typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);
	
#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void *Memory)
	typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);
	
	typedef struct read_file_result
	{
		u32 ContentsSize;
		void *Contents;
	} read_file_result;
	
#define PLATFORM_READ_ENTIRE_FILE(name) read_file_result name(char *Filename)
	typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);
	
#define PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
	typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);
	
#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)
	
#define BITMAP_BYTES_PER_PIXEL 4
	typedef struct game_offscreen_buffer
	{
		// NOTE(Denis): Pixels are always 32-bits wide, Memory Order BB GG RR XX
		void *Memory;
		int Width;
		int Height;
		int Pitch;
	} game_offscreen_buffer;
	
	typedef struct platform_api
	{
		platform_allocate_memory *AllocateMemory;
		platform_deallocate_memory *DeallocateMemory;
	} platform_api;
	
	typedef struct game_memory
	{
		uint64 PermanentStorageSize;
		void *PermanentStorage; // NOTE(Denis): REQUIRED to be celared to zero at startup.
		
		uint64 TransientStorageSize;
		void *TransientStorage; // NOTE(Denis): REQUIRED to be celared to zero at startup.
		
		platform_work_queue *HighPriorityQueue;
		platform_work_queue *LowPriorityQueue;
		
		b32 ExecutableReloaded;
		platform_api PlatformAPI;
		
		// NOTE(Denis): Signals back to the platform layer
		b32 QuitRequested;
		
	} game_memory;
	
	typedef struct game_button_state
	{
		s32 HalfTransitionCount;
		b32 EndedDown;
	} game_button_state; 
	
	typedef struct game_controller_input
	{
		bool32 IsConntected;
		bool32 IsAnalog;
		real32 StickAverageX;
		real32 StickAverageY;
		
		union
		{
			game_button_state Buttons[12];
			struct {
				game_button_state MoveUp;
				game_button_state MoveDown;
				game_button_state MoveLeft;
				game_button_state MoveRight;
				
				game_button_state ActionUp;
				game_button_state ActionDown;
				game_button_state ActionLeft;
				game_button_state ActionRight;
				
				game_button_state LeftShoulder;
				game_button_state RightShoulder;
				
				game_button_state Back;
				game_button_state Start;
			};
		};
	} game_controller_input;
	
	enum game_input_mouse_button
	{
		PlatformMouseButton_Left,
		PlatformMouseButton_Middle,
		PlatformMouseButton_Right,
		PlatformMouseButton_Extended0,
		PlatformMouseButton_Extended1,
		
		PlatformMouseButton_Count,
		
	};
	
	inline b32 WasPressed(game_button_state State)
	{
		b32 Result = ((State.HalfTransitionCount > 1) ||
					  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
		return(Result);
	}
	
	typedef struct game_input
	{
		r32 dtForFrame;
		
		game_controller_input Controllers[5];
		
		// NOTE(Denis): For debbuiging only
		game_button_state MouseButtons[PlatformMouseButton_Count];
		r32 MouseX, MouseY, MouseZ;
		b32 ShiftDown, AltDown, ControlDown;
	}  game_input;
	
#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input)
	typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
	
	inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
	{
		Assert(ControllerIndex < ArrayCount(Input->Controllers));
		
		game_controller_input *Result = &Input->Controllers[ControllerIndex];
		return(Result);
	}
	
#pragma pack(push, 1)
	
	struct bitmap_id
	{
		u32 Value;
	};
	
	struct sound_id
	{
		u32 Value;
	};
	
	struct font_id
	{
		u32 Value;
	};
	
	union v2
	{
		struct
		{
			real32 x,y;
		};
		struct
		{
			real32 u,v;
		};
		real32 E[2];
	};
	
#pragma pack(pop)
	
	union v3
	{
		struct
		{
			real32 x,y,z;
		};
		struct
		{
			real32 r,g,b;
		};
		struct
		{
			real32 u,v,w;
		};
		struct
		{
			v2 xy;
			real32 Ignored0_;
		};
		struct
		{
			real32 Ignored1_;
			v2 yz;
		};
		struct
		{
			v2 uv;
			real32 Ignored2_;
		};
		struct
		{
			real32 Ignored3_;
			v2 yw;
		};
		real32 E[3];
	};
	
	
	union v4
	{
		struct
		{
			real32 x,y,z,w;
		};
		struct
		{
			union
			{
				v3 xyz;
				struct
				{
					real32 x,y,z;
				};
			};
			
			real32 w;
			
		};
		struct
		{
			union
			{
				v3 rgb;
				struct
				{
					real32 r,g,b;
				};
			};
			
			real32 a;
			
		};
		struct
		{
			v2 xy;
			real32 Ignored0_;
			real32 Ignored1_;
		};
		struct
		{
			real32 Ignored2_;
			v2 yz;
			real32 Ignored3_;
		};
		struct
		{
			real32 Ignored4_;
			real32 Ignored5_;
			v2 zw;
		};
		real32 E[4];
	};
	
	struct rectangle2
	{
		v2 Min;
		v2 Max;
	};
	
	struct rectangle3
	{
		v3 Min;
		v3 Max;
	};
	
	
	union mat3
	{
		struct 
		{
			r32 Value1_1;
			r32 Value1_2;
			r32 Value1_3;
			r32 Value2_1;
			r32 Value2_2;
			r32 Value2_3;
			r32 Value3_1;
			r32 Value3_2;
			r32 Value3_3;
		};
		r32 E[9];
	};
	
	union mat4
	{
		struct 
		{
			r32 Value1_1;
			r32 Value1_2;
			r32 Value1_3;
			r32 Value1_4;
			r32 Value2_1;
			r32 Value2_2;
			r32 Value2_3;
			r32 Value2_4;
			r32 Value3_1;
			r32 Value3_2;
			r32 Value3_3;
			r32 Value3_4;
			r32 Value4_1;
			r32 Value4_2;
			r32 Value4_3;
			r32 Value4_4;
		};
		r32 E[16];
	};
}

// NOTE(Denis): Probably need to be relocated to its own file, to the game inside code

#define STYR_PLATFORM_H
#endif //STYR_PLATFORM_H
