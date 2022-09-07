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
// NOTE(Denis): Mouse Movement variables, probably get rid of too!
global_variable b32 FirstStart = true;
global_variable f32 MouseDeltaX;
global_variable f32 MouseDeltaY;
global_variable f32 MouseOldPx;
global_variable f32 MouseOldPy;

global_variable bool GlobalRunning;
global_variable bool GlobalPause;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable s64 GlobalPerfCountFrequency;
global_variable WINDOWPLACEMENT GlobalWindowPosition =  {sizeof(GlobalWindowPosition)};
global_variable const s32 MAX_FRAMES_IN_FLIGHT = 2;
global_variable b32 FramebufferResized = false;
global_variable HWND GlobalWindowHandle;

global_variable char *MODEL_PATH = "../data/models/landcruiser.obj";
global_variable char *TEXTURE_PATH = "../data/textures/landcruiser_d.png";

#include "styr.cpp"
#include "styr_vulkan.cpp"

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

inline f32
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
					else if(VKCode == 'A')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft,IsDown);
					}
					else if(VKCode == 'S')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveDown,IsDown);
					}
					else if(VKCode == 'D')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveRight,IsDown);
					}
					else if(VKCode == 'E')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->RotateLeft,IsDown);
					}
					else if(VKCode == 'Q')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->RotateRight,IsDown);
					}
					else if(VKCode == VK_ESCAPE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Back,IsDown);
					}
					else if(VKCode == 'P')
					{
						if(IsDown)
						{
							GlobalPause = !GlobalPause;
						}
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
							FirstStart = true; // We need these for mouse delta to be reset every time we resize window
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
		} break;
		
		case WM_CLOSE:
		{
			GlobalRunning = false;
		} break;
		
		case WM_SETCURSOR:
		{
			//SetCursor(0);
			Result = DefWindowProc(Window, Message, WParam,LParam);
		} break;
#if 0
		case WM_ACTIVATEAPP:
		{
			if(WParam == TRUE)
			{
				GlobalPause = false;
			}
			else
			{
				GlobalPause = true;
			}
		} break;
#endif
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
		} break;
		
		case WM_SIZE:
		{
			FramebufferResized = true;
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

PLATFORM_ASSIGN_STYR_STRING(styr_string_create)
{
	styr_string Result = {};
	
	
	for(u32 i = 0; 
		A[i] != '\0';
		++i)
	{
		++Result.Count;
	}
	
	Result.Data = (char *)Win32AllocateMemory(Result.Count * sizeof(char));
	
	for(u32 i = 0; 
		i < Result.Count;
		++i)
	{
		Result.Data[i] = A[i];
	}
	
	return Result;
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
CreateHashValue(vertex Vert)
{
	u32 Result = 0;
	char Buffer[64];
	s32 P = 31;
	u32 M = (u32)(1e9 + 9);
	u32 P_pow = 1;
	
	_snprintf_s(Buffer, sizeof(Buffer), "%f%f%f%f%f",
				Vert.Pos.x, Vert.Pos.y, Vert.Pos.z, Vert.TexCoord.x, Vert.TexCoord.y);
	
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
LoadObj(char *Filepath, win32_styr_array *OutVertices, win32_styr_array *OutUVs, win32_styr_array *OutNormals, win32_styr_array *OutIndices,  memory_chunk *TranArena)
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
	
	InitializeArray(OutIndices, TranArena, VertexIndicesCount, u32);
	
#if STYR_LOAD_MODEL_OPTIMIZED
	chunk_temporary_memory TempMemory = BeginTemporaryMemory(TranArena);
	
	InitializeArray(OutVertices, TranArena, VertexIndicesCount, vertex);
	
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
			vertex Vert = {};
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
	
	vertex *Vertices = (vertex *)OutVertices->Data;
	u32 *Indices = (u32 *)OutIndices->Data;
	
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
			vertex Vert = {};
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
				PushArrayElement(OutVertices, Vert, vertex);
				++UniqueIndex;
			}
			
			PushArrayElement(OutIndices, Hashes[HashValueIndex].Index, u32);
			++VertexIndex;
		}
	}
	
	EndTemporaryMemory(TempMemory);
	
	vertex *ActualVerticesInUse = PushArray(TranArena, UniqueIndex, vertex, AlignNoClear(4));
	OutVertices->Count = UniqueIndex;
	OutVertices->Data = ActualVerticesInUse;
#else
	InitializeArray(OutVertices, TranArena, VertexIndicesCount, vertex);
	
	vertex *Vertices = (vertex *)OutVertices->Data;
	u32 *Indices = (u32 *)OutIndices->Data;
	
	u32 VertexIndex = 0;
	for(u32 ShapeIndex = 0;
		ShapeIndex < Shapes.size();
		++ShapeIndex)
	{
		for(u32 Index = 0;
			Index < Shapes[ShapeIndex].mesh.indices.size();
			++Index)
		{
			vertex Vert = {};
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
LoadModel(win32_styr_array *OutVerts, win32_styr_array *OutIndices, memory_chunk *TranArena)
{
	// TODO(Denis): Use these functions for separation
	// of Normals and UVs
	
	win32_styr_array Normals = {};
	win32_styr_array UVs = {};
	
	LoadObj(MODEL_PATH, OutVerts, &UVs, &Normals, OutIndices, TranArena);
}

int WinMain(HINSTANCE Instance,
			HINSTANCE PrevInstance,
			LPSTR CommandLine,
			int ShowCode)
{
	styr_string Papar = styr_string_create("De ya buv?");
	
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
	
	game_input Input[2] = {};
	game_input *NewInput = &Input[0];
	game_input *OldInput = &Input[1];
	
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
#if 0
			// Register for RAW INPUT DATA
			RAWINPUTDEVICE Rid;
			Rid.usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
			Rid.usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
			Rid.dwFlags = RIDEV_INPUTSINK;
			Rid.hwndTarget = Window;
			
			if(RegisterRawInputDevices(&Rid, 1, sizeof(Rid)) == -1)
			{
				Assert(!"Oh hi Mark!");
			}
			// End of register for RAW INPUT DATA
#endif
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
			
			f32 GameUpdateHz = (f32)(MonitorRefreshHz / 2.0f);
			f32 TargetSecondsPerFrame = 1.0f / (f32)GameUpdateHz;
			
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
			memory_chunk *TranArena = (memory_chunk *)Win32TranState.Storage;
			InitializeArena(TranArena, Win32TranState.StorageSize - sizeof(memory_chunk),
							(u8 *)Win32TranState.Storage + sizeof(memory_chunk));
			
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
			
			ShowVulkanResult(vkCreateInstance(&CreateInfo, 0, &VulkanInstance),"Instance created %s");
			
#if 0
			//Get VULKAN extensions count
			u32 ExtensionCount = 0;
			vkEnumerateInstanceExtensionProperties(0, &ExtensionCount, 0);
			
			VkExtensionProperties *ExtensionProperties = (VkExtensionProperties *)PushArray(TranArena, ExtensionCount, VkExtensionProperties);
			
			//Get avialable VULKAN Extensions
			vkEnumerateInstanceExtensionProperties(0, &ExtensionCount, ExtensionProperties);
			
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
			u32 QueueFamilyCount = 0;
			queue_family_indices QueueIndices = {};
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, 0);
			VkQueueFamilyProperties *QueueFamilies = PushArray(TranArena, QueueFamilyCount,VkQueueFamilyProperties);
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies);\
			PrintMessage(QueueFamilyCount, "Queue families : %d\n");
			
			for(u32 QueueFamilyIndex = 0;
				QueueFamilyIndex < QueueFamilyCount;
				++QueueFamilyIndex)
			{
				if(QueueFamilies[QueueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					QueueIndices.GraphicsFamily = QueueFamilyIndex;
				}
			}
			
			// Create Logical Device and Queues
			f32 QueuePriority = 1.0f;
			
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
			
			ShowVulkanResult(vkCreateDevice(PhysicalDevice, &CreateDeviceInfo, 0, &LogicalDevice), "Physical Device Created: %s");
			
			// Retreiving queue handles
			VkQueue GraphicQueue;
			vkGetDeviceQueue(LogicalDevice, QueueIndices.GraphicsFamily, 0, &GraphicQueue);
			
			// Window surface creation
			VkSurfaceKHR VulkanWindowSurface = {};
			VkWin32SurfaceCreateInfoKHR CreateSurfaceInfo = {};
			CreateSurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			CreateSurfaceInfo.hwnd = Window;
			CreateSurfaceInfo.hinstance = Instance;
			
			ShowVulkanResult(vkCreateWin32SurfaceKHR(VulkanInstance, &CreateSurfaceInfo, 0, &VulkanWindowSurface), "Surface created: %s");
			
			CreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice, VulkanWindowSurface, QueueIndices, QueuePriority,
							CreateDeviceInfo, LogicalDevice, SwapChainImageFormat, SwapChain, SwapChainExtent, PresentQueue,
							SwapChainImages, ImageCount);
			
			// CHAIN IMAGES HERE!!!
			VkImageView SwapChainImageViews[3];
			CreateImageViews(SwapChainImageViews, SwapChainImages, SwapChainImageFormat, LogicalDevice, ImageCount);
			// Render passes
			//
			//
			
			VkRenderPass WorldRenderPass = CreateRenderPass(PhysicalDevice, LogicalDevice, SwapChainImageFormat, MSAA_Samples, false);
			
			VkCommandBuffer CommandBuffers[MAX_FRAMES_IN_FLIGHT];
			
			VkPipeline GraphicsPipeline = VkCreatePipeline(LogicalDevice, WorldRenderPass, SwapChainExtent, MSAA_Samples, &PipelineLayout, &DescriptorSetLayout);
			
			// End of Subpasses and attachments references
			//
			//
			// End of Render Pases
			
			// Command buffers
			//
			//
			
			// Command pools
			VkCommandPool CommandPool = {};
			VkCommandPoolCreateInfo PoolInfo = {};
			PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			PoolInfo.queueFamilyIndex = QueueIndices.GraphicsFamily;
			
			ShowVulkanResult(vkCreateCommandPool(LogicalDevice, &PoolInfo, 0, &CommandPool), "Command Pool Created : %s");
			// End of Command pools
			
			CreateColorResources(PhysicalDevice, LogicalDevice, &CommandPool, GraphicQueue, SwapChainExtent, &ColorImage, &ColorImageMemory, &ColorImageView, SwapChainImageFormat, MipLevels, MSAA_Samples);
			
			CreateDepthResources(PhysicalDevice, LogicalDevice, &CommandPool, GraphicQueue, SwapChainExtent, &TextureImage, &TextureImageMemory, &DepthImageView, MSAA_Samples);
			
			CreateFramebuffers(SwapChainImageViews, ArrayCount(SwapChainImageViews), DepthImageView, ColorImageView, SwapChainExtent, WorldRenderPass, LogicalDevice, SwapChainFramebuffers);
			
			// Image textures creation
			CreateTextureImage(&TextureImage, &TextureImageMemory, PhysicalDevice, LogicalDevice, CommandPool, GraphicQueue, &TextureImageBuffer, &TextureImageBufferMemory, MipLevels);
			
			CreateTextureImageView(LogicalDevice, TextureImageView, TextureImage, MipLevels);
			
			CreateTextureSampler(PhysicalDevice, LogicalDevice, TextureSampler, MipLevels);
			
			// End of Image textures creation
			
			// Create Vertex Buffer
			VkBuffer VertexBuffer = {};
			VkDeviceMemory VertexBufferMemory = {};
			vertex *Vertices = 0;
			u32 *Indices = 0;
			u32 VerticesCount = 0;
			
			win32_styr_array VerticesArray = {};
			win32_styr_array IndicesArray = {};
			LoadModel(&VerticesArray, &IndicesArray, TranArena);
			
			Vertices = (vertex *)VerticesArray.Data;
			Indices = (u32 *)IndicesArray.Data;
			
			VerticesCount = VerticesArray.Count;
			CreateVertexBuffer(&CommandPool, GraphicQueue, VertexBuffer, &VertexBufferMemory, PhysicalDevice, LogicalDevice, Vertices, VerticesArray.Count);
			//CreateVertexBuffer(&CommandPool, GraphicQueue, VertexBuffer, &VertexBufferMemory, PhysicalDevice, LogicalDevice, Vertices, VerticesCount);
			// End of Create Vertex Buffer
			
			// Create Index Buffer
			
			VkBuffer IndexBuffer = {};
			VkDeviceMemory IndexBufferMemory = {};
			
			//u32 IndicesCount = 6;
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
			ShowVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferInfo, CommandBuffers), "Command Buffer Created : %s");
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
				
				PrintMessage(IsSemaphoreAndFencesCreated, IsSemaphoreAndFencesCreated ? "Semaphores and Fences created : VK_SUCCESS\n" : "Semaphores and Fences created : VK_SUCCESS\n");
			}
			
			// End of Creating the synchronization objects
			
			//
			//
			// End of Semaphores
			
			//
			//
			// End of VULKAN Initialization
			
			u32 CommandsBufferSize = Megabytes(4);
			void *EngineRenderCommandsBufferBase = Win32AllocateMemory(CommandsBufferSize);
			engine_render_commands EngineRenderCommands = {&SwapChainExtent.width, &SwapChainExtent.height, CommandsBufferSize, 0, (u8 *)EngineRenderCommandsBufferBase, 0};
			
			ShowWindow(Window, SW_SHOW);
			
			while(GlobalRunning)
			{
				NewInput->dtForFrame = TargetSecondsPerFrame;
				game_controller_input *OldKeyboardController = GetController(OldInput,0);
				game_controller_input *NewKeyboardController = GetController(NewInput,0);
				*NewKeyboardController = {};
				NewKeyboardController->IsConntected = true;
				for(int ButtonIndex=  0;
					ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
					++ButtonIndex)
				{
					NewKeyboardController->Buttons[ButtonIndex].EndedDown =
						OldKeyboardController->Buttons[ButtonIndex].EndedDown;
				}
				
				Win32ProcessPendingMessages(&Win32State,NewKeyboardController);
				
				if(!GlobalPause)
				{
					// NOTE(Denis): Update mouse position inside Input struct
					POINT MouseP;
					GetCursorPos(&MouseP);
					ScreenToClient(Window,&MouseP);
					NewInput->MouseX = (f32)MouseP.x;
					NewInput->MouseY = (f32)(((f32)SwapChainExtent.height - 1) - MouseP.y);
					NewInput->MouseZ = 0; // TODO(Denis): Support mouse whell ?
					
					f32 WinHalfHeight = ((f32)MouseP.y);
					f32 WinHalfWidth = ((f32)MouseP.x);
					
					POINT WindowP = {(LONG)WinHalfWidth, (LONG)WinHalfHeight};
					ClientToScreen(Window, &WindowP);
					
					f32 ScreenWrapOffsetX = 0;
					f32 ScreenWrapOffsetY = 0;
					
					// NOTE(Denis): We need this for mouse delta from start to 
					// be equal to 0, to do this, we simply update MouseOldPos
					// to mouse positions, and later we subtract it from new 
					// mouse position.
					if(FirstStart)
					{
						MouseOldPx = NewInput->MouseX;
						MouseOldPy = NewInput->MouseY;
						FirstStart = false;
					}
					
					if(NewInput->MouseX < 1)
					{
						SetCursorPos((u32)SwapChainExtent.width + WindowP.x, WindowP.y);
						ScreenWrapOffsetX = (f32)SwapChainExtent.width - 1;
						//SetCursor(0);
					}
					
					if(NewInput->MouseY < 1)
					{
						SetCursorPos(WindowP.x, WindowP.y - SwapChainExtent.height);
						ScreenWrapOffsetY = (f32)SwapChainExtent.height - 1;
						//SetCursor(0);
					}
					
					if(NewInput->MouseX > SwapChainExtent.width - 1)
					{
						SetCursorPos(WindowP.x - (SwapChainExtent.width - 1), WindowP.y);
						ScreenWrapOffsetX = -(f32)SwapChainExtent.width + 1;
						//SetCursor(0);
					}
					
					if(NewInput->MouseY > SwapChainExtent.height - 2)
					{
						SetCursorPos(WindowP.x, WindowP.y + (SwapChainExtent.height - 1));
						ScreenWrapOffsetY = -(f32)SwapChainExtent.height + 1;
						//SetCursor(0);
					}
					
					f32 x_offset = MouseOldPx - NewInput->MouseX;
					f32 y_offset = MouseOldPy - NewInput->MouseY;
					
					MouseOldPx = NewInput->MouseX + ScreenWrapOffsetX;
					MouseOldPy = NewInput->MouseY + ScreenWrapOffsetY;
					
					NewInput->MouseDeltaX = -x_offset / (f32)SwapChainExtent.width;
					NewInput->MouseDeltaY = y_offset / (f32)SwapChainExtent.height;
					
					NewInput->ShiftDown = (GetKeyState(VK_SHIFT) & (1 << 15));
					NewInput->AltDown = (GetKeyState(VK_MENU) & (1 << 15));
					NewInput->ControlDown = (GetKeyState(VK_CONTROL) & (1 << 15));
					
					LARGE_INTEGER WorkCounter = Win32GetWallClock();
					f32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
					
					f32 SecondsElapsedForFrame = WorkSecondsElapsed;
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
					
					
					// TODO(Denis): Create loading this function from our endgine dll
					EngineUpdateAndRender(&GameMemory, NewInput, &EngineRenderCommands);
					
					// RENDERING FRAME
					//
					//
					
					vkWaitForFences(LogicalDevice, 1, &InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);
					
					// Acquire an image from the swap chain
					
					VkResult Result = vkAcquireNextImageKHR(LogicalDevice, SwapChain, UINT64_MAX, ImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);
					
					if(Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || FramebufferResized)
					{
						FramebufferResized = false;
						RecreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice,
										  VulkanWindowSurface, QueueIndices, QueuePriority,
										  CreateDeviceInfo, LogicalDevice, &CommandPool, &GraphicQueue, SwapChainImageFormat, SwapChainFramebuffers, ArrayCount(SwapChainFramebuffers), SwapChainImageViews,ArrayCount(SwapChainImageViews), DepthImageView, &DepthImage, &DepthImageMemory, ColorImageView, &ColorImage, &ColorImageMemory, SwapChain, SwapChainExtent, PresentQueue, SwapChainImages, MipLevels, ArrayCount(SwapChainImages), WorldRenderPass, ImageCount, MSAA_Samples);
						FirstStart = true; // We need these for mouse delta to be reset every time we resize window
					}
					
					vkResetFences(LogicalDevice, 1, &InFlightFences[CurrentFrame]);
					// End of Acquire an image from the swap chain
					
					// Recording the command buffer
					vkResetCommandBuffer(CommandBuffers[CurrentFrame], 0);
					RecordCommandBuffer(CommandBuffers[CurrentFrame], WorldRenderPass, GraphicsPipeline, PipelineLayout, DescriptorSets, SwapChainFramebuffers, SwapChainExtent, VertexBuffer, IndexBuffer, VerticesCount, IndicesCount, CurrentFrame, ImageIndex);
					
					// Update uniform buffer. // NOTE(Denis): Here is the render commands from the engine function will be update
					// the game matrices for now.
					Win32UpdateUniformBuffer(LogicalDevice, SwapChainExtent, UniformBuffersMemory, EngineRenderCommands, CurrentFrame);
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
						RecreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice,
										  VulkanWindowSurface, QueueIndices, QueuePriority,
										  CreateDeviceInfo, LogicalDevice, &CommandPool, &GraphicQueue, SwapChainImageFormat, SwapChainFramebuffers, ArrayCount(SwapChainFramebuffers), SwapChainImageViews,ArrayCount(SwapChainImageViews), DepthImageView, &DepthImage, &DepthImageMemory, ColorImageView, &ColorImage, &ColorImageMemory, SwapChain, SwapChainExtent, PresentQueue, SwapChainImages, MipLevels, ArrayCount(SwapChainImages), WorldRenderPass, ImageCount, MSAA_Samples);
						FirstStart = true; // We need these for mouse delta to be reset every time we resize window
					}
					
					// End of Recording the command buffer
					
					CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
					
					//
					//
					// END OF RENDERING FRAME
					
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
					
					// NOTE(Denis): Frame time elapsed
					LARGE_INTEGER EndCounter = Win32GetWallClock();
					f32 SecondsElapsed = Win32GetSecondsElapsed(LastCounter, EndCounter);
					LastCounter = EndCounter;
				}
				else
				{
					FirstStart = true;
				}
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
	}
	
	VirtualFree(Win32State.GameMemoryBlock, 0, MEM_RELEASE);
	
	return 0;
	
}