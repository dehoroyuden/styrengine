//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------
// NOTE(Denis): Win32 platform specific includes
#include <windows.h>
#include <stdio.h>
#include <chrono>

// NOTE(Denis): OpenGL open math library, in future will be replaced by our own library
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// NOTE(Denis): Image and File Importers
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "styr_platform.h"
#include "dengine_shared.h"

#include "win32_styr.h"

// TODO(Denis): Get rid of the global variables!!!
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable s64 GlobalPerfCountFrequency;
global_variable WINDOWPLACEMENT GlobalWindowPosition =  {sizeof(GlobalWindowPosition)};
global_variable const s32 MAX_FRAMES_IN_FLIGHT = 2;
global_variable b32 FramebufferResized = false;
global_variable HWND GlobalWindowHandle;
global_variable char TextBuffer[256];

global_variable char *MODEL_PATH = "../data/models/cube.obj";
global_variable char *TEXTURE_PATH = "../data/textures/ava_20.jpg";

#include "styr.cpp"
#include "styr_vulkan.cpp"

PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile)
{
	read_file_result Result = {};
	
	HANDLE FileHandle = CreateFileA(Filename,GENERIC_READ,FILE_SHARE_READ, 0,OPEN_EXISTING,0,0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0,FileSize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(Result.Contents)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead,0) && 
				   (FileSize32 == BytesRead))
				{
					// NOTE(Denis): File read successfully
					Result.ContentsSize = FileSize32;
				}
				else
				{
					PlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				// TODO(Denis): Logging
			}
		}
		else
		{
			// TODO(Denis): Logging
		}
		CloseHandle(FileHandle);
	}
	else
	{
		// TODO(Denis): Logging
	}
	
	return(Result);
}

PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
	void *Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	return Result;
}

PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, b32 IsDown)
{
	if(NewState->EndedDown != IsDown)
	{
		NewState->EndedDown = IsDown;
		++NewState->HalfTransitionCount;
	}
}

inline LARGE_INTEGER
Win32GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / 
					 (real32)GlobalPerfCountFrequency);
	
	return(Result);
}

internal void
ToggleFullscreen(HWND Window)
{
	// NOTE(Denis): Follows Raymond Chen's prescription 
	// for fullscreen toggling, see: 
	// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
	
	DWORD Style = GetWindowLong(Window, GWL_STYLE);
	if (Style & WS_OVERLAPPEDWINDOW)
	{
		MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
		if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
			GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) 
		{
			SetWindowLong(Window, GWL_STYLE,Style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(Window, HWND_TOP,
						 MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
						 MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
						 MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
						 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	} 
	else
	{
		SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(Window, &GlobalWindowPosition);
		SetWindowPos(Window, 0, 0, 0, 0, 0,
					 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
					 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController)
{
	MSG Message;
	while(PeekMessage(&Message,0,0,0,PM_REMOVE))
	{
		switch(Message.message)
		{
			case WM_QUIT:
			{
				GlobalRunning = false;
			} break;
			
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32 VKCode = (uint32)Message.wParam;
				bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
				if(WasDown != IsDown)
				{
					if(VKCode == 'W')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveUp,IsDown);
					}
					
					if(IsDown)
					{
						bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
						if((VKCode == VK_F4) && AltKeyWasDown)
						{
							GlobalRunning = false;
						}
						
						if((VKCode == VK_RETURN) && AltKeyWasDown)
						{
							ToggleFullscreen(Message.hwnd);
						}
					}
				}
			} break;
			
			default:
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			} break;
		}
	}
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
						UINT Message,
						WPARAM WParam,
						LPARAM LParam)
{
	LRESULT Result = 0;
	switch(Message)
	{
		case WM_CREATE:
		{
			OutputDebugStringA("WM_CREATE\n");
		} break;
		
		case WM_DESTROY:
		{
			GlobalRunning = false;
			OutputDebugStringA("WM_DESTROY\n");
		} break;
		
		case WM_CLOSE:
		{
			GlobalRunning = false;
			OutputDebugStringA("WM_DESTROY\n");
		} break;
		
		case WM_SETCURSOR:
		{
			//SetCursor(0);
			Result = DefWindowProc(Window, Message, WParam,LParam);
		} break;
		
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			OutputDebugStringA("WM_DESTROY\n");
		} break;
		
		case WM_SIZE:
		{
			FramebufferResized = true;
			OutputDebugStringA("WM_AAAAAAAAAAAA\n");
		} break;
		
		default:
		{
			Result = DefWindowProc(Window, Message, WParam,LParam);
		} break;
	}
	
	return Result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	
	Buffer->Width = Width;
	Buffer->Height = Height;
	
	s32 BytesPerPixel = 4;
	Buffer->BytesPerPixel = 4;
	
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	
	Buffer->Pitch = Align16(Width*BytesPerPixel);
	int BitmapMemorySize = (Buffer->Pitch * Buffer->Height);
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

internal u32
FindVsCount(char *Symbols)
{
	u32 Result = 0;
	if(Symbols[0] == 'v' &&
	   (IsWhitespace(Symbols[1])))
	{
		return FindVsCount(++Symbols) + 1;
	} 
	else if(Symbols[0] != '\0')
	{
		return FindVsCount(++Symbols);
	}
	
	return 0;
}

struct HashVertex
{
	u32 HashValue;
	u32 Count;
	u32 Index;
};

// TODO(Denis): Write better hash function
inline u32 
CreateHashValue(Vertex Vert)
{
	u32 Result = 0;
	char Buffer[128];
	s32 P = 31;
	u32 M = (u32)(1e9 + 9);
	u32 P_pow = 1;
	
	_snprintf_s(Buffer, sizeof(Buffer), "%f,%f,%f,%f,%f,%f,%f,%f,%f",
				Vert.Pos.x, Vert.Pos.y, Vert.Pos.z, Vert.TexCoord.x, Vert.TexCoord.y, 
				Vert.Color.x, Vert.Color.y, Vert.Color.z, Vert.Color.w);
	
	for(u32 CharacterIndex = 0;
		CharacterIndex < ArrayCount(Buffer);
		++CharacterIndex)
	{
		Result = (Result + (Buffer[CharacterIndex] - 'a' + 1) * P_pow) % M;
		P_pow = (P_pow * P) % M;
	}
	
	return Result;
}

inline u32
FindVertexByHash(u32 HashValue, HashVertex *HashVertices, u32 HashVerticesCount)
{
	u32 Result = 0;
	
	for(u32 Index = 0;
		Index < HashVerticesCount;
		++Index)
	{
		if(HashVertices[Index].HashValue == HashValue)
		{
			Result = Index;
			break;
		}
	}
	
	return Result;
}

inline u32
FindVerticesCountByHashValue(u32 HashValue, HashVertex *HashVertices, u32 HashVerticesCount)
{
	u32 Result = 0;
	
	for(u32 Index = 0;
		Index < HashVerticesCount;
		++Index)
	{
		if(HashVertices[Index].HashValue == HashValue)
		{
			++Result;
		}
	}
	
	return Result;
}

inline u32
FindUniqueVerticesCount(HashVertex *HashVertices, u32 HashVerticesCount)
{
	u32 Result = 0;
	
	for(u32 Index = 0;
		Index < HashVerticesCount;
		++Index)
	{
		u32 VertexRepeatedCount = FindVerticesCountByHashValue(HashVertices[Index].HashValue, HashVertices, HashVerticesCount);
		if(VertexRepeatedCount == 1)
		{
			++Result;
		}
	}
	
	return Result;
}

internal b32
LoadObj(char *Filepath, Array *OutVertices, Array *OutUVs, Array *OutNormals, Array *OutIndices,  memory_arena *TranArena)
{
	// NOTE(Denis): Load obj file and process it. Optimizes the repeated vertex by adding it to new array.
	tinyobj::attrib_t Attrib;
    std::vector<tinyobj::shape_t> Shapes;
    std::vector<tinyobj::material_t> Materials;
    std::string warn, err;
	
    if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &warn, &err, MODEL_PATH)) {
        throw std::runtime_error(warn + err);
    }
	
	// NOTE(Denis): How many indices in file
	u32 VertexIndicesCount = 0;
	for(u32 ShapeIndex = 0;
		ShapeIndex < Shapes.size();
		++ShapeIndex)
	{
		for(u32 Index = 0;
			Index < Shapes[ShapeIndex].mesh.indices.size();
			++Index)
		{
			++VertexIndicesCount;
		}
	}
	
	
	OutIndices->Count = VertexIndicesCount;
	OutIndices->Size = VertexIndicesCount * sizeof(u32);
	OutIndices->Base = PushArray(TranArena, VertexIndicesCount, u32);
	
#if STYR_LOAD_MODEL_OPTIMIZED
	temporary_memory TempMemory = BeginTemporaryMemory(TranArena);
	
	OutVertices->Size = VertexIndicesCount * sizeof(Vertex);
	OutVertices->Base = PushArray(TranArena, VertexIndicesCount, Vertex);
	
	// NOTE(Denis): Hash all the values from vertices array into hashes array
	HashVertex *Hashes = (HashVertex *)PushArray(TranArena, VertexIndicesCount, HashVertex);
	
	VertexIndicesCount = 0;
	for(u32 ShapeIndex = 0;
		ShapeIndex < Shapes.size();
		++ShapeIndex)
	{
		for(u32 Index = 0;
			Index < Shapes[ShapeIndex].mesh.indices.size();
			++Index)
		{
			Vertex Vert = {};
			Vert.Pos =
			{
				Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 0],
				Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 1],
				Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 2]
			};
			
			Vert.TexCoord =
			{
				Attrib.texcoords[2 * Shapes[ShapeIndex].mesh.indices[Index].texcoord_index + 0],
				1.0f - Attrib.texcoords[2 * Shapes[ShapeIndex].mesh.indices[Index].texcoord_index + 1]
			};
			
			Vert.Color = {1.0f, 1.0f, 1.0f};
			
			Hashes[VertexIndicesCount].HashValue = CreateHashValue(Vert);
			++VertexIndicesCount;
		}
	}
	
	Vertex *Vertices = (Vertex *)OutVertices->Base;
	u32 *Indices = (u32 *)OutIndices->Base; 
	
	u32 VerticesCount = VertexIndicesCount;
	u32 UniqueIndex = 0;
	u32 VertexIndex = 0;
	for(u32 ShapeIndex = 0;
		ShapeIndex < Shapes.size();
		++ShapeIndex)
	{
		for(u32 Index = 0;
			Index < Shapes[ShapeIndex].mesh.indices.size();
			++Index)
		{
			Vertex Vert = {};
			{
				Vert.Pos =
				{
					Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 0],
					Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 1],
					Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 2]
				};
				
				Vert.TexCoord =
				{
					Attrib.texcoords[2 * Shapes[ShapeIndex].mesh.indices[Index].texcoord_index + 0],
					1.0f - Attrib.texcoords[2 * Shapes[ShapeIndex].mesh.indices[Index].texcoord_index + 1]
				};
				
				Vert.Color = {1.0f, 1.0f, 1.0f};
			}
			
			u32 HashValue = CreateHashValue(Vert);
			u32 HashValueIndex = FindVertexByHash(HashValue, Hashes, VertexIndicesCount);
			u32 HashValueVertexCount = Hashes[HashValueIndex].Count;
			
			if(HashValueVertexCount == 0)
			{
				++Hashes[HashValueIndex].Count;
				Hashes[HashValueIndex].Index = UniqueIndex;
				Vertices[UniqueIndex] = Vert;
				++UniqueIndex;
			}
			
			Indices[VertexIndex] = Hashes[HashValueIndex].Index;
			++VertexIndex;
		}
	}
	
	EndTemporaryMemory(TempMemory);
	
	Vertex *ActualVerticesInUse = PushArray(TranArena, UniqueIndex, Vertex, AlignNoClear(4));
	OutVertices->Count = UniqueIndex;
	OutVertices->Base = ActualVerticesInUse;
#else
	OutVertices->Count = VertexIndicesCount;
	OutVertices->Size = VertexIndicesCount * sizeof(Vertex);
	OutVertices->Base = PushArray(TranArena, VertexIndicesCount, Vertex);
	
	Vertex *Vertices = (Vertex *)OutVertices->Base;
	u32 *Indices = (u32 *)OutIndices->Base;
	
	u32 VertexIndex = 0;
	for(u32 ShapeIndex = 0;
		ShapeIndex < Shapes.size();
		++ShapeIndex)
	{
		for(u32 Index = 0;
			Index < Shapes[ShapeIndex].mesh.indices.size();
			++Index)
		{
			Vertex Vert = {};
			{
				Vert.Pos =
				{
					Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 0],
					Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 1],
					Attrib.vertices[3 * Shapes[ShapeIndex].mesh.indices[Index].vertex_index + 2]
				};
				
				Vert.TexCoord =
				{
					Attrib.texcoords[2 * Shapes[ShapeIndex].mesh.indices[Index].texcoord_index + 0],
					1.0f - Attrib.texcoords[2 * Shapes[ShapeIndex].mesh.indices[Index].texcoord_index + 1]
				};
				
				Vert.Color = {1.0f, 1.0f, 1.0f};
			}
			
			Vertices[VertexIndex] = Vert;
			Indices[VertexIndex] = VertexIndex;
			++VertexIndex;
		}
	}
#endif
	
	return true;
}

internal void
LoadModel(Array *OutVerts, Array *OutIndices, memory_arena *TranArena)
{
	// TODO(Denis): Use these functions for separation
	// of Normals and UVs
	
	Array Normals = {};
	Array UVs = {};
	
	LoadObj(MODEL_PATH, OutVerts, &UVs, &Normals, OutIndices, TranArena);
}

int WinMain(HINSTANCE Instance,
			HINSTANCE PrevInstance,
			LPSTR CommandLine,
			int ShowCode)
{
#if STYR_INTERNAL
	AllocConsole();
    FILE *fpstdin = stdin, *fpstdout = stdout, *fpstderr = stderr;
	
	freopen_s(&fpstdin, "CONIN$", "r", stdin);
	freopen_s(&fpstdout, "CONOUT$", "w", stdout);
	freopen_s(&fpstderr, "CONOUT$", "w", stderr);
#endif
	
	win32_state Win32State = {};
	
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	
	UINT DesiredSchedulerMS = 1;
	b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
	
	game_input Input = {};
	game_input *NewInput = &Input;
	
	LARGE_INTEGER LastCounter = Win32GetWallClock();
	
	GlobalRunning = true;
	
	Win32ResizeDIBSection(&GlobalBackbuffer,800,600);
	
	game_offscreen_buffer GameBuffer = {};
	GameBuffer.Memory = &GlobalBackbuffer.Memory;
	GameBuffer.Width = GlobalBackbuffer.Width;
	GameBuffer.Height = GlobalBackbuffer.Height;
	GameBuffer.Pitch = GlobalBackbuffer.Pitch;
	
	WNDCLASSA WindowClass = {};
	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WindowClass.hInstance = Instance;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WindowClass.lpszClassName = "StyrEngineBytesWindowClass";
	
	if(RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowExA(0,
									  WindowClass.lpszClassName,
									  "Horror Game / Styrengine",
									  WS_OVERLAPPEDWINDOW,
									  CW_USEDEFAULT,
									  CW_USEDEFAULT,
									  CW_USEDEFAULT,
									  CW_USEDEFAULT,
									  0,
									  0,
									  Instance,
									  0);
		
		if(Window)
		{
			GlobalWindowHandle = Window;
			
			s32 MonitorRefreshHz = 60;
			HDC RefreshDC = GetDC(Window);
#if 0
			s32 Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
#else
			s32 Win32RefreshRate = MonitorRefreshHz;
#endif
			ReleaseDC(Window, RefreshDC);
			if(Win32RefreshRate > 1)
			{
				MonitorRefreshHz = Win32RefreshRate;
			}
			
			r32 GameUpdateHz = (r32)(MonitorRefreshHz / 2.0f);
			r32 TargetSecondsPerFrame = 1.0f / (r32)GameUpdateHz;
			
			// Game Memory Allocation
			
#if STYR_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
			LPVOID BaseAddress = 0;
#endif
			
			// Win32 Layer Memory Arena
			win32_transient_state Win32TranState = {};
			Win32TranState.StorageSize = Megabytes(256);
			
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(5);
			GameMemory.TransientStorageSize = Megabytes(5);//Gigabytes(1);
			
			GameMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
			GameMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;
			
			Win32State.TotalSize = (GameMemory.PermanentStorageSize +
									GameMemory.TransientStorageSize +
									Win32TranState.StorageSize);
			
			Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
													  MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			
			GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
			GameMemory.TransientStorage = ((u8 *)GameMemory.PermanentStorage + 
										   GameMemory.PermanentStorageSize);
			Win32TranState.Storage = ((u8 *)GameMemory.TransientStorage + 
									  GameMemory.TransientStorageSize);
			
			// Win 32 Storage Initialization
			Assert(sizeof(win32_transient_state) <= Win32TranState.StorageSize);
			memory_arena *TranArena = (memory_arena *)Win32TranState.Storage;
			InitializeArena(TranArena, Win32TranState.StorageSize - sizeof(memory_arena),
							(u8 *)Win32TranState.Storage + sizeof(memory_arena));
			
			// VULKAN Initialization
			//
			//
			
			// TODO
			// - Push constants
			// - Instanced Render
			// - Dynamic uniforms
			// - Separate images and sampler descriptors
			// - Pipeline cache
			// - Multi-threaded command buffer generation
			// - Multiple subpasses
			// - Compute shaders
			
			VkInstance VulkanInstance = 0;
			
			u32 ImageCount = 0;
			u32 CurrentFrame = 0;
			VkQueue PresentQueue = {};
			VkRenderPass RenderPass = {};
			VkSwapchainKHR SwapChain = {};
			VkExtent2D SwapChainExtent = {};
			VkFormat SwapChainImageFormat = {};
			VkPipelineLayout PipelineLayout = {};
			VkDescriptorSetLayout DescriptorSetLayout = {};
			VkDescriptorPool DescriptorPool = {};
			
			VkImage SwapChainImages[3];
			VkFramebuffer SwapChainFramebuffers[3];
			VkDescriptorSet *DescriptorSets = 0;
			VkBuffer *UniformBuffers = 0;
			VkDeviceMemory *UniformBuffersMemory = 0;
			
			u32 MipLevels = 0;
			VkImage TextureImage = {};
			VkBuffer TextureImageBuffer = {};
			VkDeviceMemory TextureImageBufferMemory = {};
			VkDeviceMemory TextureImageMemory = {};
			VkImageView TextureImageView = {};
			VkSampler TextureSampler = {};
			
			VkImage DepthImage = {};
			VkImageView DepthImageView = {};
			VkDeviceMemory DepthImageMemory = {};
			
			VkSampleCountFlagBits MSAA_Samples = VK_SAMPLE_COUNT_1_BIT;
			VkImage ColorImage = {};
			VkImageView ColorImageView = {};
			VkDeviceMemory ColorImageMemory = {};
			
			// NOTE(Denis): Create VULKAN Application Info
			VkApplicationInfo AppInfo = {};
			AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			AppInfo.pApplicationName = "Hello Triangle";
			AppInfo.applicationVersion = VK_MAKE_VERSION(0,0,1);
			AppInfo.pEngineName = "Styr";
			AppInfo.engineVersion = VK_MAKE_VERSION(0,0,1);
			AppInfo.apiVersion = VK_API_VERSION_1_2;
			
			// NOTE(Denis): Create VULKAN Instance Info
			char *Extensions[] = 
			{
				"VK_KHR_win32_surface",
				"VK_KHR_surface",
			};
			
			char *Layers[] = 
			{
				"VK_LAYER_KHRONOS_validation"
			};
			
			VkInstanceCreateInfo CreateInfo = {};
			CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			CreateInfo.pApplicationInfo = &AppInfo;
			CreateInfo.enabledExtensionCount = ArrayCount(Extensions);
			CreateInfo.ppEnabledExtensionNames = Extensions;
			
			CreateInfo.enabledLayerCount = 1;
#if STYR_VULKAN_VALIDATION
#else
			CreateInfo.enabledLayerCount = 0;
#endif
			CreateInfo.ppEnabledLayerNames = Layers;
			
			Assert_(vkCreateInstance(&CreateInfo, 0, &VulkanInstance),"Instance created %s");
			
			//Get VULKAN extensions count
			u32 ExtensionCount = 0;
			vkEnumerateInstanceExtensionProperties(0, &ExtensionCount, 0);
			
			VkExtensionProperties ExtensionProperties[13];
			
			//Get avialable VULKAN Extensions
			vkEnumerateInstanceExtensionProperties(0, &ExtensionCount, ExtensionProperties);
			
			// NOTE(Denis): Print out VULKAN avialable extensions.
			OutputDebugStringA("Available Extensions: ");
			_snprintf_s(TextBuffer,sizeof(TextBuffer),"%d\n",ExtensionCount);
			OutputDebugStringA(TextBuffer);
			
			for(u32 Index = 0;
				Index < ExtensionCount;
				++Index)
			{
				char TextBuffer[256];
				_snprintf_s(TextBuffer,sizeof(TextBuffer),"%s\n",ExtensionProperties[Index]);
				OutputDebugStringA(TextBuffer);
			}
			
#if 0
			// Get avilable VULKAN Layers
			VkLayerProperties SupportedLayers[20];
			u32 SupportedLayersCount = 0;
			vkEnumerateInstanceLayerProperties(&SupportedLayersCount, 0);
			vkEnumerateInstanceLayerProperties(&SupportedLayersCount, SupportedLayers);
			
			OutputDebugStringA("Available Layers: ");
			_snprintf_s(TextBuffer,sizeof(TextBuffer),"%d\n",SupportedLayersCount);
			OutputDebugStringA(TextBuffer);
			
			for(u32 Index = 0;
				Index < SupportedLayersCount;
				++Index)
			{
				char TextBuffer[256];
				_snprintf_s(TextBuffer,sizeof(TextBuffer),"%s\n",SupportedLayers[Index]);
				OutputDebugStringA(TextBuffer);
			}
#endif
			// NOTE(Denis): Selecting Physical Device
			VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
			VkPhysicalDeviceFeatures DeviceFeatures = {};
			PickPhysicalDevice(VulkanInstance, PhysicalDevice, DeviceFeatures, MSAA_Samples);
			DeviceFeatures.sampleRateShading = VK_TRUE;
			
			// NOTE(Denis): Queue Families - Initialize QUEUE
			QueueFamilyIndices QueueIndices = {};
			
			u32 QueueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, 0);
			
			VkQueueFamilyProperties QueueFamilies[5];
			//VkQueueFamilyProperties *QueueFamilies = (VkQueueFamilyProperties *)Win32AllocateMemory(sizeof(VkQueueFamilyProperties)*QueueFamilyCount);
			
			
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies);\
			Assert_NotVulkan(QueueFamilyCount, "Queue families : %d");
			
#if 1
			for(u32 QueueFamilyIndex = 0;
				QueueFamilyIndex < QueueFamilyCount;
				++QueueFamilyIndex)
			{
				if(QueueFamilies[QueueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					QueueIndices.GraphicsFamily = QueueFamilyIndex;
				}
			}
#else
			for(u32 QueueFamilyIndex = 0;
				QueueFamilyIndex < QueueFamilyCount;
				++QueueFamilyIndex)
			{
				if(QueueFamilies[QueueFamilyIndex].queueFlags & VK_QUEUE_TRANSFER_BIT && QueueFamilies[QueueFamilyIndex].queueFlags & ~VK_QUEUE_GRAPHICS_BIT)
				{
					QueueIndices.GraphicsFamily = QueueFamilyIndex;
				}
			}
#endif
			
			// Create Logical Device and Queues
			r32 QueuePriority = 1.0f;
			
			VkDevice LogicalDevice = {};
			VkDeviceQueueCreateInfo QueueCreateInfo = {};
			QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfo.queueFamilyIndex = QueueIndices.GraphicsFamily;
			QueueCreateInfo.queueCount = 1;
			QueueCreateInfo.pQueuePriorities = &QueuePriority;
			
			char *DeviceExtensions = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
			
			// Creating the logical device
			VkDeviceCreateInfo CreateDeviceInfo = {};
			CreateDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			CreateDeviceInfo.pQueueCreateInfos = &QueueCreateInfo;
			CreateDeviceInfo.queueCreateInfoCount = 1;
			CreateDeviceInfo.pEnabledFeatures = &DeviceFeatures;
			CreateDeviceInfo.enabledExtensionCount = 1;
			CreateDeviceInfo.ppEnabledExtensionNames = &DeviceExtensions;
			
			Assert_(vkCreateDevice(PhysicalDevice, &CreateDeviceInfo, 0, &LogicalDevice), "Physical Device Created: %s");
			
			// Retreiving queue handles
			VkQueue GraphicQueue;
			vkGetDeviceQueue(LogicalDevice, QueueIndices.GraphicsFamily, 0, &GraphicQueue);
			
			// Window surface creation
			VkSurfaceKHR VulkanWindowSurface = {};
			VkWin32SurfaceCreateInfoKHR CreateSurfaceInfo = {};
			CreateSurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			CreateSurfaceInfo.hwnd = Window;
			CreateSurfaceInfo.hinstance = Instance;
			
			Assert_(vkCreateWin32SurfaceKHR(VulkanInstance, &CreateSurfaceInfo, 0, &VulkanWindowSurface), "Surface created: %s");
			
			Assert_NotVulkan("Here was begin last error prone momen", "%s");
			Win32VkCreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice, VulkanWindowSurface, QueueIndices, QueuePriority,
								   CreateDeviceInfo, LogicalDevice, SwapChainImageFormat, SwapChain, 0, 0, SwapChainExtent, PresentQueue,
								   SwapChainImages, ImageCount);
			
			Assert_NotVulkan("Here is probaly program strarts without errors (Maybe shaders are busted...)", "%s");
			
			// CHAIN IMAGES HERE!!!
			VkImageView SwapChainImageViews[3];
			Win32VkCreateImageViews(SwapChainImageViews, SwapChainImages, SwapChainImageFormat, LogicalDevice, ImageCount);
			// Render passes
			//
			//
			
			// Attachment description
			VkAttachmentDescription ColorAttachment = {};
			ColorAttachment.format = SwapChainImageFormat;
			ColorAttachment.samples = MSAA_Samples;
			ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clearing to black buffer before using!
			ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Clearing to black buffer before using!
			ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			
			VkAttachmentDescription ColorAttachmentResolve = {};
			ColorAttachmentResolve.format = SwapChainImageFormat;
			ColorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
			ColorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Clearing to black buffer before using!
			ColorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			ColorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Clearing to black buffer before using!
			ColorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			ColorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			ColorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			// End of Attachment description
			
			// Subpasses and attachments references
			VkAttachmentReference ColorAttachmentReference = {};
			ColorAttachmentReference.attachment = 0;
			ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			
			VkAttachmentReference ColorAttachmentResolveReference = {};
			ColorAttachmentResolveReference.attachment = 2;
			ColorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			// End of Subpasses and attachments references
			
			// Depth Attachment description
			VkAttachmentDescription DepthAttachnment = {};
			DepthAttachnment.format = FindDepthFormat(PhysicalDevice);
			DepthAttachnment.samples = MSAA_Samples;
			DepthAttachnment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			DepthAttachnment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthAttachnment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			DepthAttachnment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthAttachnment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			DepthAttachnment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			// End of Depth Attachment description
			
			// Subpasses and attachments references
			VkAttachmentReference DepthAttachmentReference = {};
			DepthAttachmentReference.attachment = 1;
			DepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			// End of Subpasses and attachments references
			
			// Subpass
			VkSubpassDescription Subpass = {};
			Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			Subpass.colorAttachmentCount = 1;
			Subpass.pColorAttachments = &ColorAttachmentReference;
			Subpass.pDepthStencilAttachment = &DepthAttachmentReference;
			Subpass.pResolveAttachments = &ColorAttachmentResolveReference;
			// End of Subpass
			
			
			// Render Pass
			VkAttachmentDescription Attachments[] = {ColorAttachment, DepthAttachnment, ColorAttachmentResolve};
			VkCommandBuffer CommandBuffers[MAX_FRAMES_IN_FLIGHT];
			VkRenderPassCreateInfo RenderPassInfo = {};
			RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			RenderPassInfo.attachmentCount = ArrayCount(Attachments);
			RenderPassInfo.pAttachments = Attachments;
			RenderPassInfo.subpassCount = 1;
			RenderPassInfo.pSubpasses = &Subpass;
			
			Assert_(vkCreateRenderPass(LogicalDevice, &RenderPassInfo, 0, &RenderPass), "Render pass created : %s");
			
			// Subpass dependencies
			VkSubpassDependency SubpassDependency = {};
			SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			SubpassDependency.dstSubpass = 0;
			SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			SubpassDependency.srcAccessMask = 0;
			SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			RenderPassInfo.dependencyCount = 1;
			RenderPassInfo.pDependencies = &SubpassDependency;
			// End of Subpass dependencies
			// End of Render Pass
			// End of Subpasses and attachments references
			//
			//
			// End of Render Pases
			
			// Creation of the GRAPHICS PIPELINE
			//
			//
			
			// Shader Model Info Creation
			read_file_result VertexFile = PlatformReadEntireFile("shaders/vert.spv");
			read_file_result FragmentFile = PlatformReadEntireFile("shaders/frag.spv");
			
			char *VertShaderCode = (char *)VertexFile.Contents;
			char *FragShaderCode = (char *)FragmentFile.Contents;
			
			VkShaderModuleCreateInfo VertShaderModuleInfo = {};
			VertShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			VertShaderModuleInfo.codeSize = VertexFile.ContentsSize;
			VertShaderModuleInfo.pCode = (const u32 *)VertShaderCode;
			
			VkShaderModuleCreateInfo FragShaderModuleInfo = {};
			FragShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			FragShaderModuleInfo.codeSize = FragmentFile.ContentsSize;
			FragShaderModuleInfo.pCode = (const u32 *)FragShaderCode;
			// End of Shader Model Info Creation
			
			// Shader Module
			b32 IsShaderModuleCreated = false;
			VkShaderModule VertShaderModule = {};
			VkShaderModule FragShaderModule = {};
			Assert_(vkCreateShaderModule(LogicalDevice, &VertShaderModuleInfo, 0, &VertShaderModule), "Vert Shader Module : %s");
			Assert_(vkCreateShaderModule(LogicalDevice, &FragShaderModuleInfo, 0, &FragShaderModule) , "Frag Shader Module : %s");
			// End of Shader Module
			
			// Shader Stage Creation
			VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
			VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			VertShaderStageInfo.module = VertShaderModule;
			VertShaderStageInfo.pName = "main";
			
			VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
			FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			FragShaderStageInfo.module = FragShaderModule;
			FragShaderStageInfo.pName = "main";
			VkPipelineShaderStageCreateInfo ShaderStages[] = {VertShaderStageInfo, FragShaderStageInfo};
			//End of Shader Stage Creation
			
			// Dynamic State
			VkDynamicState DynamicStates[] = 
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			}; 
			
			VkPipelineDynamicStateCreateInfo DynamicState = {};
			DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			DynamicState.dynamicStateCount = ArrayCount(DynamicStates);
			DynamicState.pDynamicStates = DynamicStates;
			// End of Dynamic State
			
			// Vertex Input
			Array Arr_AttributeDescriptions = GetAttributeDescriptions(TranArena);
			VkVertexInputAttributeDescription *InputAttributeDescriptions = (VkVertexInputAttributeDescription *)Arr_AttributeDescriptions.Base;
			
			VkVertexInputBindingDescription BindingDescription = GetBindingDescription();
			
			VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
			VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			VertexInputInfo.vertexBindingDescriptionCount = 1;
			VertexInputInfo.pVertexBindingDescriptions = &BindingDescription;
			VertexInputInfo.vertexAttributeDescriptionCount = Arr_AttributeDescriptions.Count;
			VertexInputInfo.pVertexAttributeDescriptions = InputAttributeDescriptions;
			// End of Vertex Input
			
			// Input Assembly
			VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {};
			InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
			// End of Input Assembly
			
			// Viewports and scissors
			VkViewport Viewport = {};
			Viewport.x = 0.0f;
			Viewport.y = 0.0f;
			Viewport.width = (r32)SwapChainExtent.width;
			Viewport.height = (r32)SwapChainExtent.height;
			Viewport.minDepth = 0.0f;
			Viewport.maxDepth = 1.0f;
			
			VkRect2D Scissor = {};
			Scissor.offset = {0, 0};
			Scissor.extent = SwapChainExtent;
			
			VkPipelineViewportStateCreateInfo ViewportStateInfo = {};
			ViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			ViewportStateInfo.viewportCount = 1;
			ViewportStateInfo.scissorCount = 1;
			// End of Viewports and scissors
			
			// RASTERIZER
			VkPipelineRasterizationStateCreateInfo Rasterizer = {};
			Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			Rasterizer.depthClampEnable = VK_FALSE;
			Rasterizer.rasterizerDiscardEnable = VK_FALSE;
			Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			Rasterizer.lineWidth = 1.0f;
			Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			
			Rasterizer.depthBiasEnable = VK_FALSE;
			Rasterizer.depthBiasConstantFactor = 0.0f;
			Rasterizer.depthBiasClamp = 0.0f;
			Rasterizer.depthBiasSlopeFactor = 0.0f;
			// End of RASTERIZER
			
			// Multisampling
			VkPipelineMultisampleStateCreateInfo MultisamplingInfo = {};
			MultisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			MultisamplingInfo.sampleShadingEnable = VK_FALSE;
			MultisamplingInfo.rasterizationSamples = MSAA_Samples;
			MultisamplingInfo.minSampleShading = 1.0f;
			MultisamplingInfo.pSampleMask = 0;
			MultisamplingInfo.alphaToCoverageEnable = VK_FALSE;
			MultisamplingInfo.alphaToOneEnable = VK_FALSE;
			// End of Multisampling
			
			// Color blending
			VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
			ColorBlendAttachment.colorWriteMask = (VK_COLOR_COMPONENT_R_BIT| VK_COLOR_COMPONENT_G_BIT|
												   VK_COLOR_COMPONENT_B_BIT| VK_COLOR_COMPONENT_A_BIT);
			ColorBlendAttachment.blendEnable = VK_TRUE;
			ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
			
			VkPipelineColorBlendStateCreateInfo ColorBlendingInfo = {};
			ColorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			ColorBlendingInfo.logicOpEnable = VK_FALSE;
			ColorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
			ColorBlendingInfo.attachmentCount = 1;
			ColorBlendingInfo.pAttachments = &ColorBlendAttachment;
			ColorBlendingInfo.blendConstants[0] = 0.0f;
			ColorBlendingInfo.blendConstants[1] = 0.0f;
			ColorBlendingInfo.blendConstants[2] = 0.0f;
			ColorBlendingInfo.blendConstants[3] = 0.0f;
			// End of Color blending
			
			// Create Descriptor Set For Matrices
			CreateDescriptorSetLayout(LogicalDevice, &DescriptorSetLayout);
			// End of Crearte Descriptor Set For Matrices
			
			// Pipeline Layout
			VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
			PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			PipelineLayoutInfo.setLayoutCount = 1;
			PipelineLayoutInfo.pSetLayouts = &DescriptorSetLayout;
			PipelineLayoutInfo.pushConstantRangeCount = 0;
			PipelineLayoutInfo.pPushConstantRanges = 0;
			Assert_(vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutInfo, 0, &PipelineLayout), "Pipeline layout created : %s");
			// End of Pipeline Layout
			
			// Depth stencil pipeline info
			VkPipelineDepthStencilStateCreateInfo DepthStencilPipelineInfo = {};
			DepthStencilPipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			DepthStencilPipelineInfo.depthTestEnable = VK_TRUE;
			DepthStencilPipelineInfo.depthWriteEnable = VK_TRUE;
			DepthStencilPipelineInfo.depthCompareOp = VK_COMPARE_OP_LESS;
			DepthStencilPipelineInfo.depthBoundsTestEnable = VK_FALSE;
			DepthStencilPipelineInfo.minDepthBounds = 0.0f;
			DepthStencilPipelineInfo.maxDepthBounds = 1.0f;
			DepthStencilPipelineInfo.stencilTestEnable = VK_FALSE;
			DepthStencilPipelineInfo.front = {};
			DepthStencilPipelineInfo.back = {};
			// End of Depth stencil pipeline info
			
			// Graphics Pipeline Create Info
			VkGraphicsPipelineCreateInfo PipelineInfo = {};
			PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			PipelineInfo.stageCount = 2;
			PipelineInfo.pStages = ShaderStages;
			PipelineInfo.pVertexInputState = &VertexInputInfo;
			PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
			PipelineInfo.pViewportState = &ViewportStateInfo;
			PipelineInfo.pRasterizationState = &Rasterizer;
			PipelineInfo.pMultisampleState = &MultisamplingInfo;
			PipelineInfo.pDepthStencilState = &DepthStencilPipelineInfo;
			PipelineInfo.pColorBlendState = &ColorBlendingInfo;
			PipelineInfo.pDynamicState = &DynamicState;
			PipelineInfo.layout = PipelineLayout;
			PipelineInfo.renderPass = RenderPass;
			PipelineInfo.subpass = 0;
			PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			PipelineInfo.basePipelineIndex = -1;
			
			VkPipeline GraphicsPipeline = {};
			Assert_(vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, 1, &PipelineInfo, 0, &GraphicsPipeline), "Graphics Pipeline Created : %s");
			// End of Graphics Pipeline Create Info
			//
			//
			// End of Creation GRAPHICS PIPELINE
			
			// Command buffers
			//
			//
			
			// Command pools
			VkCommandPool CommandPool = {};
			VkCommandPoolCreateInfo PoolInfo = {};
			PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			PoolInfo.queueFamilyIndex = QueueIndices.GraphicsFamily;
			
			Assert_(vkCreateCommandPool(LogicalDevice, &PoolInfo, 0, &CommandPool), "Command Pool Created : %s");
			// End of Command pools
			
			CreateColorResources(PhysicalDevice, LogicalDevice, &CommandPool, GraphicQueue, SwapChainExtent, &ColorImage, &ColorImageMemory, &ColorImageView, SwapChainImageFormat, MipLevels, MSAA_Samples);
			
			CreateDepthResources(PhysicalDevice, LogicalDevice, &CommandPool, GraphicQueue, SwapChainExtent, &TextureImage, &TextureImageMemory, &DepthImageView, MSAA_Samples);
			
			Win32VkCreateFramebuffers(SwapChainImageViews, ArrayCount(SwapChainImageViews), DepthImageView, ColorImageView, SwapChainExtent, RenderPass, LogicalDevice, SwapChainFramebuffers);
			
			// Image textures creation
			CreateTextureImage(&TextureImage, &TextureImageMemory, PhysicalDevice, LogicalDevice, CommandPool, GraphicQueue, &TextureImageBuffer, &TextureImageBufferMemory, MipLevels);
			
			CreateTextureImageView(LogicalDevice, TextureImageView, TextureImage, MipLevels);
			
			CreateTextureSampler(PhysicalDevice, LogicalDevice, TextureSampler, MipLevels);
			
			// End of Image textures creation
			
			// Create Vertex Buffer
			VkBuffer VertexBuffer = {};
			VkDeviceMemory VertexBufferMemory = {};
			Vertex *Vertices = 0;
			u32 VerticesCount = 0;
			u32 *Indices = 0;
			
#if 0
			// NOTE(Denis): This will be moved in its own seperate function in the future.
			Vertices = PushArray(TranArena, VerticesCount, Vertex);
			Assert(Vertices);
			
			// Fill Vertices
			Vertices[0] = {{-0.5f, -0.5f, 0}, {0.1529385f, 0.603911f, 0.9450815f}, {1.0f, 0.0f}};
			Vertices[1] = {{0.5f, -0.5f, 0}, {0.1529385f, 0.603911f, 0.9450815f}, {0.0f, 0.0f}};
			Vertices[2] = {{0.5f, 0.5f, 0}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}};
			Vertices[3] = {{-0.5f, 0.5f, 0}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}};
			
			Vertices[4] = {{-0.5f, -0.5f, -0.5f}, {0.1529385f, 0.603911f, 0.9450815f}, {1.0f, 0.0f}};
			Vertices[5] = {{0.5f, -0.5f, -0.5f}, {0.1529385f, 0.603911f, 0.9450815f}, {0.0f, 0.0f}};
			Vertices[6] = {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}};
			Vertices[7] = {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}};
			
#endif
			Array VerticesArray = {};
			Array IndicesArray = {};
			LoadModel(&VerticesArray, &IndicesArray, TranArena);
			
			Vertices = (Vertex *)VerticesArray.Base;
			Indices = (u32 *)IndicesArray.Base;
			
			CreateVertexBuffer(&CommandPool, GraphicQueue, VertexBuffer, &VertexBufferMemory, PhysicalDevice, LogicalDevice, Vertices, VerticesArray.Count);
			
			// End of Create Vertex Buffer
			
			// Create Index Buffer
			
			VkBuffer IndexBuffer = {};
			VkDeviceMemory IndexBufferMemory = {};
			
			u32 IndicesCount = IndicesArray.Count;
			
			Assert(Indices);
			
			CreateIndexBuffer(&CommandPool, GraphicQueue, IndexBuffer, &IndexBufferMemory, PhysicalDevice,LogicalDevice, Indices, IndicesCount);
			
			// End of Create Index Buffer
			
			// Create Uniform Buffer
			UniformBuffers = PushArray(TranArena, MAX_FRAMES_IN_FLIGHT, VkBuffer);
			
			UniformBuffersMemory = PushArray(TranArena, MAX_FRAMES_IN_FLIGHT, VkDeviceMemory);
			CreateUniformBuffers(TranArena, PhysicalDevice, LogicalDevice, UniformBuffers, UniformBuffersMemory);
			
			// End of Create Uniform Buffer
			
			// Create Descriptor Pool
			
			DescriptorSets = PushArray(TranArena, MAX_FRAMES_IN_FLIGHT, VkDescriptorSet);
			CreateDescriptorPool(LogicalDevice, &DescriptorPool);
			CreateDescriptorSets(TranArena, LogicalDevice, UniformBuffers, &DescriptorPool, DescriptorSets, DescriptorSetLayout, TextureImageView, TextureSampler);
			
			// End of Create Descriptor Pool
			
			// Perspective Projection
			
			
			
			// End of Perspective Projection
			
			// Command buffer allocation
			
			VkCommandBufferAllocateInfo CommandBufferInfo = {};
			CommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			CommandBufferInfo.commandPool = CommandPool;
			CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			CommandBufferInfo.commandBufferCount = ArrayCount(CommandBuffers);
			
			Assert_(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferInfo, CommandBuffers), "Command Buffer Created : %s");
			
			// End of Command buffer allocation
			
			u32 ImageIndex = 0; // TODO(Denis): This later will be an argument to our function which image buffer to use
			
			//
			//
			// End of Command buffers
			
			// Semaphores
			//
			//
			
			VkSemaphore ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkSemaphore RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkFence InFlightFences[MAX_FRAMES_IN_FLIGHT];
			
			// Creating the synchronization objects
			
			VkSemaphoreCreateInfo SemaphoreInfo = {};
			SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			
			VkFenceCreateInfo FenceInfo = {};
			FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			
			for(u32 Index = 0;
				Index < MAX_FRAMES_IN_FLIGHT;
				++Index)
			{
				b32 IsSemaphoreAndFencesCreated = false;
				IsSemaphoreAndFencesCreated = (vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, 0, &ImageAvailableSemaphores[Index]) == VK_SUCCESS && vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, 0, &RenderFinishedSemaphores[Index]) == VK_SUCCESS && vkCreateFence(LogicalDevice, &FenceInfo, 0, &InFlightFences[Index]) == VK_SUCCESS);
				
				Assert_(IsSemaphoreAndFencesCreated, IsSemaphoreAndFencesCreated ? "Semaphores and Fences created : VK_SUCCESS" : "Semaphores and Fences created : VK_SUCCESS");
			}
			
			// End of Creating the synchronization objects
			
			//
			//
			// End of Semaphores
			
			//
			//
			// End of VULKAN Initialization
			
			ShowWindow(Window, SW_SHOW);
			
			while(GlobalRunning)
			{
#if 0
				u32 extensionCount = 0;
				//vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0);
				char TextBuffer[256];
				_snprintf_s(TextBuffer,sizeof(TextBuffer),"%d",extensionCount);
#endif
				// Here all our rendering loop is occur!
				//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				//HDC DeviceContext = GetDC(Window);
				//Win32DisplayBufferInWindow(&HighPriorityQueue, &RenderCommands,DeviceContext,Dimension.Width,Dimension.Height,SortMemory); // Here where we put our render buffer (pixel buffer) to our window!
				
				game_controller_input *NewKeyboardController = GetController(NewInput,0);
				
				Win32ProcessPendingMessages(&Win32State,NewKeyboardController);
				
				LARGE_INTEGER WorkCounter = Win32GetWallClock();
				r32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
				
				r32 SecondsElapsedForFrame = WorkSecondsElapsed;
				if(SecondsElapsedForFrame < TargetSecondsPerFrame)
				{
					if(SleepIsGranular)
					{
						DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
						if(SleepMS > 0)
						{
							Sleep(SleepMS);
						}
					}
					
					while(SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
					}
				}
				else
				{
					// TODO(Denis): MISSED FRAME RATE!
				}
				
				EngineUpdateAndRender(&GameMemory, &GameBuffer);
				//GameUpdateAndRender(&GameBuffer);
				
				// RENDERING FRAME
				//
				//
				
				vkWaitForFences(LogicalDevice, 1, &InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);
				
				// Acquire an image from the swap chain
				
				VkResult Result = vkAcquireNextImageKHR(LogicalDevice, SwapChain, UINT64_MAX, ImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);
				
				if(Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || FramebufferResized)
				{
					FramebufferResized = false;
					Win32VkRecreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice,
											 VulkanWindowSurface, QueueIndices, QueuePriority,
											 CreateDeviceInfo, LogicalDevice, &CommandPool, &GraphicQueue, SwapChainImageFormat, SwapChainFramebuffers, ArrayCount(SwapChainFramebuffers), SwapChainImageViews,ArrayCount(SwapChainImageViews), DepthImageView, &DepthImage, &DepthImageMemory, ColorImageView, &ColorImage, &ColorImageMemory, SwapChain, &Viewport, &Scissor, SwapChainExtent, PresentQueue, SwapChainImages, MipLevels, ArrayCount(SwapChainImages), RenderPass, ImageCount, MSAA_Samples);
				}
				
				vkResetFences(LogicalDevice, 1, &InFlightFences[CurrentFrame]);
				// End of Acquire an image from the swap chain
				
				// Recording the command buffer
				
				
				vkResetCommandBuffer(CommandBuffers[CurrentFrame], 0);
				Win32VkRecordCommandBuffer(&Viewport, &Scissor, CommandBuffers[CurrentFrame], RenderPass,GraphicsPipeline, PipelineLayout, DescriptorSets, SwapChainFramebuffers, SwapChainExtent, VertexBuffer, IndexBuffer, VerticesCount, IndicesCount, CurrentFrame, ImageIndex);
				
				// Update uniform buffer
				
				Win32UpdateUniformBuffer(LogicalDevice, SwapChainExtent, UniformBuffersMemory, CurrentFrame);
				
				// End of Update uniform buffer
				
				VkSubmitInfo SubmitInfo = {};
				SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				
				VkSemaphore WaitSemaphores[] = {ImageAvailableSemaphores[CurrentFrame]};
				VkPipelineStageFlags WaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
				SubmitInfo.waitSemaphoreCount = 1;
				SubmitInfo.pWaitSemaphores = WaitSemaphores;
				SubmitInfo.pWaitDstStageMask = WaitStages;
				SubmitInfo.commandBufferCount = 1;
				SubmitInfo.pCommandBuffers = &CommandBuffers[CurrentFrame];
				
				VkSemaphore SignalSemaphores[] = {RenderFinishedSemaphores[CurrentFrame]};
				SubmitInfo.signalSemaphoreCount = 1;
				SubmitInfo.pSignalSemaphores = SignalSemaphores;
				
				b32 IsQueueSubmitted = (vkQueueSubmit(GraphicQueue, 1, &SubmitInfo, InFlightFences[CurrentFrame]) == VK_SUCCESS);
				Assert(IsQueueSubmitted);
				
				VkPresentInfoKHR PresentInfo = {};
				PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				PresentInfo.waitSemaphoreCount = 1;
				PresentInfo.pWaitSemaphores = SignalSemaphores;
				
				VkSwapchainKHR SwapChains[] = {SwapChain};
				PresentInfo.swapchainCount = 1;
				PresentInfo.pSwapchains = SwapChains;
				PresentInfo.pImageIndices = &ImageIndex;
				
				Result = vkQueuePresentKHR(PresentQueue, &PresentInfo);
				
				if(Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
				{
					Win32VkRecreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice,
											 VulkanWindowSurface, QueueIndices, QueuePriority,
											 CreateDeviceInfo, LogicalDevice, &CommandPool, &GraphicQueue, SwapChainImageFormat, SwapChainFramebuffers, ArrayCount(SwapChainFramebuffers), SwapChainImageViews,ArrayCount(SwapChainImageViews), DepthImageView, &DepthImage, &DepthImageMemory, ColorImageView, &ColorImage, &ColorImageMemory, SwapChain, &Viewport, &Scissor, SwapChainExtent, PresentQueue, SwapChainImages, MipLevels, ArrayCount(SwapChainImages), RenderPass, ImageCount, MSAA_Samples);
				}
				
				// End of Recording the command buffer
				
				CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
				
				//
				//
				// END OF RENDERING FRAME
				
			}
			
#if 0
			for(u32 Index = 0;
				Index < MAX_FRAMES_IN_FLIGHT;
				++Index)
			{
				vkDestroySemaphore(LogicalDevice, ImageAvailableSemaphores[Index], 0);
				vkDestroySemaphore(LogicalDevice, RenderFinishedSemaphores[Index], 0);
				vkDestroyFence(LogicalDevice, InFlightFences[Index], 0);
			}
			
			vkDestroyShaderModule(LogicalDevice, FragShaderModule, 0);
			vkDestroyShaderModule(LogicalDevice, VertShaderModule, 0);
			vkDestroyCommandPool(LogicalDevice, CommandPool, 0);
			
			vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, 0);
			vkDestroyPipeline(LogicalDevice, GraphicsPipeline, 0);
			vkDestroyRenderPass(LogicalDevice, RenderPass, 0);
			Win32VkCleanupSwapChain(LogicalDevice, SwapChainFramebuffers, ArrayCount(SwapChainFramebuffers), SwapChainImageViews, ArrayCount(SwapChainImageViews), SwapChain);
			vkDestroyDescriptorPool(LogicalDevice, DescriptorPool, 0);
			vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorSetLayout, 0);
			
			for(u32 Index = 0;
				Index < MAX_FRAMES_IN_FLIGHT;
				++Index)
			{
				vkDestroyBuffer(LogicalDevice, UniformBuffers[Index], 0);
				vkFreeMemory(LogicalDevice, UniformBuffersMemory[Index], 0);
			}
			
			for(u32 Index = 0;
				Index < ImageCount;
				++Index)
			{
				vkDestroyImageView(LogicalDevice, SwapChainImageViews[Index], 0);
			}
			
			vkDestroySampler(LogicalDevice, TextureSampler, 0);
			vkDestroyImageView(LogicalDevice, TextureImageView, 0);
			
			vkDestroySwapchainKHR(LogicalDevice, SwapChain, 0);
			vkDestroyBuffer(LogicalDevice, VertexBuffer, 0);
			vkFreeMemory(LogicalDevice, VertexBufferMemory, 0);
			vkDestroyDevice(LogicalDevice, 0);
			vkDestroySurfaceKHR(VulkanInstance, VulkanWindowSurface, 0);
			vkDestroyInstance(VulkanInstance, 0);
#endif
		}
		else
		{
			// TODO(Denis): Logging
		}
	}
	
	VirtualFree(Win32State.GameMemoryBlock, 0, MEM_RELEASE);
	
	return 0;
	
}