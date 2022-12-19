//----------------------------------------------------------------
// Styr Engine -- author: Denis Hoida | 2022
//----------------------------------------------------------------

// NOTE(Denis): Win32 specific includes
#include <windows.h>
#include <stdio.h>
#include <chrono>
//#include <thread>
//#include <threads.h>

// NOTE(Denis): OpenGL open math library, in future will be replaced by our own library
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// NOTE(Denis): Image and File Importers
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "styr_platform.h"
#include "styr_obj_importer.h"
#include "styr_shared.h"
#include "win32_styr.h"

// TODO(Denis): Get rid of the global variables!!!
// NOTE(Denis): Mouse Movement variables, probably get rid of too!
global_variable b32 FirstStart = true;
global_variable f32 MouseDeltaX;
global_variable f32 MouseDeltaY;
global_variable f32 MouseWheelDelta;
global_variable f32 MouseOldPx;
global_variable f32 MouseOldPy;

global_variable bool GlobalRunning;
global_variable bool GlobalPause;
//global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable s64 GlobalPerfCountFrequency;
global_variable WINDOWPLACEMENT GlobalWindowPosition =  {sizeof(GlobalWindowPosition)};
global_variable const s32 MAX_FRAMES_IN_FLIGHT = 2;
global_variable b32 GlobalFramebufferResized = false;
global_variable HWND GlobalWindowHandle;

#include "styr_engine.cpp"
#include "styr_vulkan.cpp"

struct win32_platform_file_handle
{
	HANDLE Win32Handle;
};

struct win32_platform_file_group
{
	HANDLE FindHandle;
	WIN32_FIND_DATAW FindData;
};

internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
{
	platform_file_group Result = {};
	
	win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)VirtualAlloc(0, sizeof(win32_platform_file_group),
																						  MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	Result.Platform = Win32FileGroup;
	
	wchar_t *WildCard = L"*.*";
	switch(Type)
	{
		case PlatformFileType_AssetFile:
		{
			WildCard = L"*.sta";
		} break;
		
		case PlatformFileType_SavedGameFile:
		{
			WildCard = L"*.ses";
		} break;
		
		InvalidDefaultCase;
	}
	
	Result.FileCount = 0;
	
	WIN32_FIND_DATAW FindData;
	HANDLE FindHandle = FindFirstFileW(WildCard, &FindData);
	while(FindHandle != INVALID_HANDLE_VALUE)
	{
		++Result.FileCount;
		
		if(!FindNextFileW(FindHandle, &FindData))
		{
			break;
		}
	}
	
	FindClose(FindHandle);
	
	Win32FileGroup->FindHandle = FindFirstFileW(WildCard, &Win32FileGroup->FindData);
	
	FindFirstFileW(WildCard, &FindData);
	
	return(Result);
}

internal PLATFORM_GET_ALL_FILE_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
{
	win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup->Platform;
	if(Win32FileGroup)
	{
		FindClose(Win32FileGroup->FindHandle);
		
		VirtualFree(Win32FileGroup, 0, MEM_RELEASE);
	}
}

internal PLATFORM_OPEN_FILE(Win32OpenNextFile)
{
	win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup->Platform;
	platform_file_handle Result = {};
	
	if(Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
	{
		// TODO(Denis): If we want, someday, make an actual arena used by Win32
		win32_platform_file_handle *Win32Handle = (win32_platform_file_handle *)VirtualAlloc(0, sizeof(win32_platform_file_handle), 
																							 MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		Result.Platform = Win32Handle;
		if(Win32Handle)
		{
			wchar_t *FileName = Win32FileGroup->FindData.cFileName;
			Win32Handle->Win32Handle = CreateFileW(FileName, GENERIC_READ, FILE_SHARE_READ, 0,OPEN_EXISTING,0,0);
			Result.NoErrors = (Win32Handle->Win32Handle != INVALID_HANDLE_VALUE);
		}
		
		if(!FindNextFileW(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
		{
			FindClose(Win32FileGroup->FindHandle);
			Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
		}
	}
	
	return(Result);
}

internal PLATFORM_FILE_ERROR(Win32FileError)
{
#if HANDMADE_DENGINE_INTERNAL
	OutputDebugStringA("WIN32 FILE ERROR: ");
	OutputDebugStringA(Message);
	OutputDebugStringA("\n");
#endif
	Handle->NoErrors = false;
}

internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
{
	if(PlatformNoFileErrors(Source))
	{
		win32_platform_file_handle *Handle = (win32_platform_file_handle *)Source->Platform;
		OVERLAPPED Overlapped = {};
		Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
		Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);
		
		u32 FileSize32 = SafeTruncateUInt64(Size);
		
		DWORD BytesRead;
		if(ReadFile(Handle->Win32Handle, Dest, FileSize32, &BytesRead, &Overlapped) && 
		   (FileSize32 == BytesRead))
		{
			// NOTE(Denis): File read succeeded!
		}
		else
		{
			Win32FileError(Source, "Read file failed.");
		}
	}
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
					else if (VKCode == VK_CONTROL)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->LCtrl, IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ArrowLeft, IsDown);
					}
					else if (VKCode == VK_RIGHT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ArrowRight, IsDown);
					}
					else if (VKCode == VK_SHIFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->LShift, IsDown);
					}
					else if (VKCode == VK_BACK)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->BackSpace, IsDown);
					}
					else if (VKCode == 'R')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonR, IsDown);
					}
					else if (VKCode == 'T')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonT, IsDown);
					}
					else if (VKCode == 'Y')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonY, IsDown);
					}
					else if (VKCode == 'U')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonU, IsDown);
					}
					else if (VKCode == 'I')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonI, IsDown);
					}
					else if (VKCode == 'O')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonO, IsDown);
					}
					else if (VKCode == 'P')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonP, IsDown);
						/*if (IsDown)
						{
							GlobalPause = !GlobalPause;
						}*/
					}
					else if (VKCode == VK_OEM_4)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Button1afterP, IsDown);
					}
					else if (VKCode == VK_OEM_6)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Button2afterP, IsDown);
					}
					else if (VKCode == VK_OEM_5)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Button3afterP, IsDown);
					}
					else if (VKCode == 'F')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonF, IsDown);
					}
					else if (VKCode == 'G')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonG, IsDown);
					}
					else if (VKCode == 'H')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonH, IsDown);
					}
					else if (VKCode == 'J')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonJ, IsDown);
					}
					else if (VKCode == 'K')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonK, IsDown);
					}
					else if (VKCode == 'L')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonL, IsDown);
					}
					else if (VKCode == VK_OEM_1)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Button1afterL, IsDown);
					}
					else if (VKCode == VK_OEM_7)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Button2afterL, IsDown);
					}
					else if (VKCode == 'Z')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->ButtonZ, IsDown);
					}
					else if (VKCode == 'X')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->ButtonX, IsDown);
					}
					else if (VKCode == 'C')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->ButtonC, IsDown);
					}
					else if (VKCode == 'V')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->ButtonV, IsDown);
					}
					else if (VKCode == 'B')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->ButtonB, IsDown);
					}
					else if (VKCode == 'N')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonN, IsDown);
					}
					else if (VKCode == 'M')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ButtonM, IsDown);
					}
					else if (VKCode == VK_OEM_COMMA)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Button1afterM, IsDown);
					}
					else if (VKCode == VK_OEM_PERIOD)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Button2afterM, IsDown);
					}
					else if (VKCode == VK_OEM_2)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Button3afterM, IsDown);
					}
					else if (VKCode == '1')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button1, IsDown);
					}
					else if (VKCode == '2')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button2, IsDown);
					}
					else if (VKCode == '3')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button3, IsDown);
					}
					else if (VKCode == '4')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button4, IsDown);
					}
					else if (VKCode == '5')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button5, IsDown);
					}
					else if (VKCode == '6')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button6, IsDown);
					}
					else if (VKCode == '7')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button7, IsDown);
					}
					else if (VKCode == '8')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button8, IsDown);
					}
					else if (VKCode == '9')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button9, IsDown);
					}
					else if (VKCode == '0')
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button0, IsDown);
					}
					else if (VKCode == VK_OEM_MINUS)
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button1after0, IsDown);
					}
					else if (VKCode == VK_OEM_PLUS)
					{
					Win32ProcessKeyboardMessage(&KeyboardController->Button2after0, IsDown);
					}
					else if (VKCode == VK_SPACE)
					{
					Win32ProcessKeyboardMessage(&KeyboardController->ButtonSpace, IsDown);
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
		case WM_MOUSEWHEEL:
		{
			MouseWheelDelta = GET_WHEEL_DELTA_WPARAM(WParam) / (120.0f * 500.0f);
			printf("Mouse wheel delta: %f\n", MouseWheelDelta);
		} break;
		
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
		} break;
		
		case WM_SIZE:
		{
			GlobalFramebufferResized = true;
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
	
#if 0
	Win32ResizeDIBSection(&GlobalBackbuffer,800,600);
	game_offscreen_buffer GameBuffer = {};
	GameBuffer.Memory = &GlobalBackbuffer.Memory;
	GameBuffer.Width = GlobalBackbuffer.Width;
	GameBuffer.Height = GlobalBackbuffer.Height;
	GameBuffer.Pitch = GlobalBackbuffer.Pitch;
#endif
	
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
			
			s32 MonitorRefreshHz = 120;
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
			
			// Win32 Layer memory chunk
			win32_transient_state Win32TranState = {};
			Win32TranState.StorageSize = Megabytes(256);
			
			// Game engine memory chunk
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(32);
			GameMemory.TransientStorageSize = Megabytes(256);//Gigabytes(1);
			
			GameMemory.PlatformAPI.GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
			GameMemory.PlatformAPI.GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
			GameMemory.PlatformAPI.OpenNextFile = Win32OpenNextFile;
			GameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
			GameMemory.PlatformAPI.FileError = Win32FileError;
			
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
			
			vulkan_context vk_context = {};
			vulkan_device vk_device = {};
			
			u32 ImageCount = 0;
			u32 CurrentFrame = 0;
			VkQueue PresentQueue = {};
			VkSwapchainKHR SwapChain = {};
			VkExtent2D SwapChainExtent = {};
			VkFormat SwapChainImageFormat = {};
			VkPipelineLayout world_pipeline_layout = {};
			VkPipelineLayout infinine_grid_pipeline_layout = {};
			VkPipelineLayout ui_pipeline_layout = {};
			VkDescriptorSetLayout DescriptorSetLayout = {};
			VkDescriptorPool DescriptorPool = {};
			
			VkImage SwapChainImages[3];
			VkFramebuffer world_framebuffers[3];
			VkFramebuffer swap_chain_framebuffers[3];
			VkDescriptorSet *world_descriptor_sets = {};
			u32 max_material_count = MaterialType_Count;
			u32 world_descriptor_set_count = max_material_count * MAX_FRAMES_IN_FLIGHT;
			VkDescriptorSet *ui_DescriptorSets = 0;
			VkDescriptorPool ui_DescriptorPool = 0;
			u32 max_world_model_count = 1024;
			u32 models_count_in_use = 0;
			vulkan_buffer WorldVertexBuffer = {};
			vulkan_buffer WorldIndexBuffer = {};
			vulkan_buffer UniformBuffer = {};
			u32 *world_vertices_offsets = {};
			u32 *world_indices_offsets = {};
			u32 max_material_pipelines = MaterialType_Count;
			VkPipeline material_pipelines[MaterialType_Count];
			
			u32 MipLevels = 0;
			u32 TextureImageCount = 0;
			u32 MaxTextureImages = 32;
			// NOTE(Denis): This is the array of indices for textures to match
			// properly objects that being rendered (because we may create entities
			// in different order than we create and push to GPU meshes)!!!
			u32 *entity_texture_indices = nullptr;
			VkImage TextureImages[32];
			VkBuffer TextureImageBuffers[32] = {};
			VkDeviceMemory TextureImageBufferMemories[32] = {};
			VkDeviceMemory TextureImageMemories[32];
			VkImageView TextureImageViews[32];
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
			
#if STYR_VULKAN_VALIDATION
			CreateInfo.enabledLayerCount = 1;
#else
			CreateInfo.enabledLayerCount = 0;
#endif
			CreateInfo.ppEnabledLayerNames = Layers;
			
			ShowVulkanResult(vkCreateInstance(&CreateInfo, 0, &VulkanInstance),"Instance created %s");
			
			// NOTE(Denis): Selecting Physical Device
			VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
			VkPhysicalDeviceFeatures DeviceFeatures = {};
			vk_PickPhysicalDevice(VulkanInstance, PhysicalDevice, DeviceFeatures, MSAA_Samples);
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
			
			vk_CreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice, VulkanWindowSurface, QueueIndices, QueuePriority,
							   CreateDeviceInfo, LogicalDevice, SwapChainImageFormat, SwapChain, SwapChainExtent, PresentQueue,
							   SwapChainImages, ImageCount);
			
			// CHAIN IMAGES HERE!!!
			VkImageView SwapChainImageViews[3];
			vk_CreateImageViews(SwapChainImageViews, SwapChainImages, SwapChainImageFormat, LogicalDevice, ImageCount);
			
			// Render passes
			//
			//
			VkRenderPass WorldRenderPass = vk_CreateRenderPass_world(PhysicalDevice, LogicalDevice, SwapChainImageFormat, MSAA_Samples);
			VkRenderPass UIRenderPass = vk_CreateRenderPass_ui(PhysicalDevice, LogicalDevice, SwapChainImageFormat, MSAA_Samples);
			
			VkCommandBuffer CommandBuffersWorld[MAX_FRAMES_IN_FLIGHT];
			VkCommandBuffer CommandBuffersUI[MAX_FRAMES_IN_FLIGHT];
			VkCommandBuffer CommandBuffers_ui_text[MAX_FRAMES_IN_FLIGHT];
			
			//
			//
			// End of Render Pases
			
			// Command buffers
			//
			//
			
			// Command pools
			VkCommandPool CommandPool = {};
			VkCommandPoolCreateInfo CommandPoolInfo = {};
			CommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			CommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			CommandPoolInfo.queueFamilyIndex = QueueIndices.GraphicsFamily;
			ShowVulkanResult(vkCreateCommandPool(LogicalDevice, &CommandPoolInfo, 0, &CommandPool), "Command Pool Created : %s");
			// End of Command pools
			
			vk_CreateColorResources(PhysicalDevice, LogicalDevice, &CommandPool, GraphicQueue, SwapChainExtent, &ColorImage, &ColorImageMemory, &ColorImageView, SwapChainImageFormat, MipLevels, MSAA_Samples);
			
			vk_CreateDepthResources(PhysicalDevice, LogicalDevice, &CommandPool, GraphicQueue, SwapChainExtent, &TextureImages[TextureImageCount], &TextureImageMemories[TextureImageCount], &DepthImageView, MSAA_Samples);
			
			vk_CreateFramebuffers(LogicalDevice, SwapChainImageViews, ArrayCount(SwapChainImageViews), DepthImageView, ColorImageView, SwapChainExtent, WorldRenderPass, UIRenderPass, world_framebuffers, swap_chain_framebuffers);
			
			
			// NOTE(Denis): Create UI buffers
			u32 ui_vertices_count = 4 * 512;
			u32 ui_indices_count = (ui_vertices_count / 4) * 6;
			vulkan_buffer ui_vertex_buffer = vk_initialize_buffer(TranArena, ui_vertices_count);
			vulkan_buffer ui_index_buffer = vk_initialize_buffer(TranArena, ui_indices_count);
			
			u32 BufferSize = ui_vertices_count * sizeof(vertex_3d);
			vk_CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, ui_vertex_buffer.data, ui_vertex_buffer.memory);
			
			BufferSize = ui_indices_count * sizeof(u32);
			vk_CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, ui_index_buffer.data, ui_index_buffer.memory);
			
			VkBuffer ui_text_vertex_buffer = {};
			VkBuffer ui_text_index_buffer = {};
			
			VkDeviceMemory ui_text_vertex_buffer_memory = {};
			VkDeviceMemory ui_text_index_buffer_memory = {};
			
			u32 text_ui_vertices_count = 4 * 16384;
			u32 text_indices_count = (text_ui_vertices_count / 4) * 6;
			
			BufferSize = text_ui_vertices_count * sizeof(vertex_3d);
			vk_CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &ui_text_vertex_buffer, &ui_text_vertex_buffer_memory);
			
			BufferSize = text_indices_count * sizeof(u32);
			vk_CreateBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &ui_text_index_buffer, &ui_text_index_buffer_memory);
			
			// Command buffer allocation
			VkCommandBufferAllocateInfo CommandBufferInfo = {};
			CommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			CommandBufferInfo.commandPool = CommandPool;
			CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			CommandBufferInfo.commandBufferCount = ArrayCount(CommandBuffersWorld);
			ShowVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferInfo, CommandBuffersWorld), "Command Buffer Created : %s");
			CommandBufferInfo.commandBufferCount = ArrayCount(CommandBuffersUI);
			ShowVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferInfo, CommandBuffersUI), "Command Buffer Created : %s");
			
			CommandBufferInfo.commandBufferCount = ArrayCount(CommandBuffers_ui_text);
			ShowVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferInfo, CommandBuffers_ui_text), "Command Buffer Created : %s");
			// End of Command buffer allocation
			
			u32 ImageIndex = 0; // TODO(Denis): This later will be an argument to our function which image buffer to use
			
			// NOTE(Denis): Semaphores for GPU
			VkSemaphore ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkSemaphore RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
			VkFence InFlightFences[MAX_FRAMES_IN_FLIGHT];
			
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
			
			vk_CreateTextureSampler(PhysicalDevice, LogicalDevice, TextureSampler, MipLevels);
			
			u32 EngineCommandsBufferSize = Megabytes(1);
			void *EngineRenderCommandsBufferBase = Win32AllocateMemory(EngineCommandsBufferSize);
			engine_render_commands EngineRenderCommands = {&SwapChainExtent.width, &SwapChainExtent.height, EngineCommandsBufferSize, 0, (u8 *)EngineRenderCommandsBufferBase, 0};
			
			// TODO(Denis): Delete this temporary solution and make proper calls from engine update!!!
			EngineInitializeAtStart(&GameMemory, NewInput, &EngineRenderCommands);
			
			read_file_result VertexFile = {};
			read_file_result FragmentFile = {};
			
			VertexFile = PlatformReadEntireFile("shaders/simple_shader_vert.spv");
			FragmentFile = PlatformReadEntireFile("shaders/simple_shader_frag.spv");
			material_pipelines[MaterialType_Default] = vk_CreatePipeline(LogicalDevice, WorldRenderPass, SwapChainExtent, MSAA_Samples, &world_pipeline_layout, &DescriptorSetLayout, VertexFile, FragmentFile, false);;
			
			VertexFile = PlatformReadEntireFile("shaders/ui_vert.spv");
			FragmentFile = PlatformReadEntireFile("shaders/ui_frag.spv");
			material_pipelines[MaterialType_UI] = vk_CreatePipeline(LogicalDevice, UIRenderPass, SwapChainExtent, MSAA_Samples, &ui_pipeline_layout, &DescriptorSetLayout, VertexFile, FragmentFile,true);
			
			VertexFile = PlatformReadEntireFile("shaders/ui_text_vert.spv");
			FragmentFile = PlatformReadEntireFile("shaders/ui_text_frag.spv");
			material_pipelines[MaterialType_UIText] = vk_CreatePipeline(LogicalDevice, UIRenderPass, SwapChainExtent, MSAA_Samples, &ui_pipeline_layout, &DescriptorSetLayout, VertexFile, FragmentFile,true);
			
			VertexFile = PlatformReadEntireFile("shaders/infinite_grid_vert.spv");
			FragmentFile = PlatformReadEntireFile("shaders/infinite_grid_frag.spv");
			material_pipelines[MaterialType_WorldGrid] = vk_CreatePipeline(LogicalDevice, WorldRenderPass, SwapChainExtent, MSAA_Samples, &world_pipeline_layout, &DescriptorSetLayout, VertexFile, FragmentFile, false);
			
			PlatformFreeFileMemory(VertexFile.Contents);
			PlatformFreeFileMemory(FragmentFile.Contents);
			
			// Create Descriptor Pool
			{
				VkDescriptorPoolSize DescriptorPoolSizes[2] = {{},{}};
				DescriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				DescriptorPoolSizes[0].descriptorCount = world_descriptor_set_count;
				DescriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				DescriptorPoolSizes[1].descriptorCount = world_descriptor_set_count;
				
				VkDescriptorPoolCreateInfo DescriptorPoolInfo = {};
				DescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				DescriptorPoolInfo.poolSizeCount = ArrayCount(DescriptorPoolSizes);
				DescriptorPoolInfo.pPoolSizes = DescriptorPoolSizes;
				DescriptorPoolInfo.maxSets = world_descriptor_set_count;
				
				b32 IsDescriptorPoolCreated = false;
				IsDescriptorPoolCreated = (vkCreateDescriptorPool(LogicalDevice, &DescriptorPoolInfo, 0, &DescriptorPool) == VK_SUCCESS);
				Assert(IsDescriptorPoolCreated);
			}
			// End of Create Descriptor Pool
			
			// Create world descriptor set
			{
				world_descriptor_sets = PushArray(TranArena, world_descriptor_set_count, VkDescriptorSet);
				
				// Create Descriptor Sets
				VkDescriptorSetLayout *Layouts = PushArray(TranArena, world_descriptor_set_count, VkDescriptorSetLayout);
				for(u32 i = 0; i < world_descriptor_set_count; ++i)
				{
					Layouts[i] = DescriptorSetLayout;
				}
				
				VkDescriptorSetAllocateInfo AllocInfo = {};
				AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				AllocInfo.descriptorPool = DescriptorPool;
				AllocInfo.descriptorSetCount = world_descriptor_set_count;
				AllocInfo.pSetLayouts = Layouts;
				
				b32 IsDescriptorSetAllocated = false;
				IsDescriptorSetAllocated = (vkAllocateDescriptorSets(LogicalDevice, &AllocInfo, world_descriptor_sets) == VK_SUCCESS);
				Assert(IsDescriptorSetAllocated);
			}
			
			// Create UI Descriptor Pool
			{
				VkDescriptorPoolSize DescriptorPoolSizes[2] = {{},{}};
				DescriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				DescriptorPoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
				DescriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				DescriptorPoolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;
				
				VkDescriptorPoolCreateInfo DescriptorPoolInfo = {};
				DescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				DescriptorPoolInfo.poolSizeCount = ArrayCount(DescriptorPoolSizes);
				DescriptorPoolInfo.pPoolSizes = DescriptorPoolSizes;
				DescriptorPoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
				
				b32 IsDescriptorPoolCreated = false;
				IsDescriptorPoolCreated = (vkCreateDescriptorPool(LogicalDevice, &DescriptorPoolInfo, 0, &ui_DescriptorPool) == VK_SUCCESS);
				Assert(IsDescriptorPoolCreated);
			}
			// End of Create Descriptor Pool
			
			// Create UI Descriptor Sets
			if(!ui_DescriptorSets)
			{
				ui_DescriptorSets = PushArray(TranArena, MAX_FRAMES_IN_FLIGHT, VkDescriptorSet);
				
				VkDescriptorSetLayout Layouts[MAX_FRAMES_IN_FLIGHT];
				Layouts[0] = DescriptorSetLayout;
				Layouts[1] = DescriptorSetLayout;
				VkDescriptorSetAllocateInfo AllocInfo = {};
				AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				AllocInfo.descriptorPool = ui_DescriptorPool;
				AllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
				AllocInfo.pSetLayouts = Layouts;
				
				b32 IsDescriptorSetAllocated = false;
				IsDescriptorSetAllocated = (vkAllocateDescriptorSets(LogicalDevice, &AllocInfo, ui_DescriptorSets) == VK_SUCCESS);
				Assert(IsDescriptorSetAllocated);
			}
			
			engine_render_commands Commands = EngineRenderCommands;
			engine_render_entry_header *Header = 0;
			u32 offset = 0;
			for(u32 index = 0;
				index < Commands.CommandBufferElementCount;
				index++)
			{
				Header = (engine_render_entry_header *)(Commands.CommandBufferBase + offset);
				
				switch(Header->Type)
				{
					case EngineRenderCommandType_PushAllMeshes:
					{
						void *vertex_data = (void *)Header->command_push_all_meshes.vertices;
						u32 vertex_count = Header->command_push_all_meshes.vertex_count;
						WorldVertexBuffer = vk_initialize_buffer(TranArena, vertex_count);
						
						vk_CreateVertexBuffer(&CommandPool, GraphicQueue, *WorldVertexBuffer.data, WorldVertexBuffer.memory, PhysicalDevice, LogicalDevice, vertex_data, sizeof(vertex_3d), vertex_count);
						
						void *index_data = (void *)Header->command_push_all_meshes.indices;
						u32 indices_count = Header->command_push_all_meshes.index_count;
						WorldIndexBuffer = vk_initialize_buffer(TranArena, indices_count);
						
						vk_CreateIndexBuffer(&CommandPool, GraphicQueue, *WorldIndexBuffer.data, WorldIndexBuffer.memory, PhysicalDevice, LogicalDevice, index_data, sizeof(u32), indices_count);
						
						models_count_in_use = Header->command_push_all_meshes.mesh_entry_offsets_count;
						world_vertices_offsets = PushArray(TranArena, models_count_in_use, u32);
						world_indices_offsets = PushArray(TranArena, models_count_in_use, u32);
						
						// Create Uniform Buffer
						UniformBuffer.data = PushArray(TranArena, models_count_in_use * MAX_FRAMES_IN_FLIGHT, VkBuffer);
						UniformBuffer.memory = PushArray(TranArena, models_count_in_use* MAX_FRAMES_IN_FLIGHT, VkDeviceMemory);
						
						for(u32 i = 0; i < models_count_in_use; ++i)
						{
							world_vertices_offsets[i] = Header->command_push_all_meshes.mesh_entry_vertex_offsets[i];
							world_indices_offsets[i] = Header->command_push_all_meshes.mesh_entry_index_offsets[i];
						}
						
						for(u32 i = 0; i < models_count_in_use * MAX_FRAMES_IN_FLIGHT; ++i)
						{
							vk_CreateUniformBuffers(TranArena, PhysicalDevice, LogicalDevice, &UniformBuffer.data[i], &UniformBuffer.memory[i]);
						}
						
						entity_texture_indices = Header->command_push_all_meshes.mesh_entry_texture_indices;
						
					} break;
					
					case EngineRenderCommandType_PushTexture:
					{
						u32 TextureWidth = Header->command_push_texture.width;
						u32 TextureHeight = Header->command_push_texture.height;
						u32 MipLevels = Header->command_push_texture.mip_maps_count;
						void *Pixels = (void *)Header->command_push_texture.data;
						
						vk_CreateTextureImage(&TextureImages[TextureImageCount], TextureWidth, TextureHeight, MipLevels, Pixels,  &TextureImageMemories[TextureImageCount], PhysicalDevice, LogicalDevice, CommandPool, GraphicQueue);
						
						TextureImageViews[TextureImageCount] = vk_CreateImageView(LogicalDevice, TextureImages[TextureImageCount], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, MipLevels);
						
						++TextureImageCount;
					} break;
				}
				
				offset += sizeof(engine_render_entry_header);
			}
			
			// Create World Descriptors
			u32 world_descriptors_count = models_count_in_use * MAX_FRAMES_IN_FLIGHT;
			for(u32 Index = 0;
				Index < world_descriptors_count;
				++Index)
			{
				VkDescriptorBufferInfo BufferInfo = {};
				BufferInfo.buffer = UniformBuffer.data[Index];
				BufferInfo.offset = 0;
				BufferInfo.range = sizeof(UniformBufferObject);
				
				u32 texture_idx = entity_texture_indices[Index % models_count_in_use];
				VkDescriptorImageInfo ImageInfo = {};
				ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				ImageInfo.imageView = TextureImageViews[texture_idx];
				ImageInfo.sampler = TextureSampler;
				
				VkWriteDescriptorSet DescriptorWrites[2] = {{},{}};
				
				DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				DescriptorWrites[0].dstSet = world_descriptor_sets[Index];
				DescriptorWrites[0].dstBinding = 0;
				DescriptorWrites[0].dstArrayElement = 0;
				DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				DescriptorWrites[0].descriptorCount = 1;
				DescriptorWrites[0].pBufferInfo = &BufferInfo;
				
				DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				DescriptorWrites[1].dstSet = world_descriptor_sets[Index];
				DescriptorWrites[1].dstBinding = 1;
				DescriptorWrites[1].dstArrayElement = 0;
				DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				DescriptorWrites[1].descriptorCount = 1;
				DescriptorWrites[1].pImageInfo = &ImageInfo;
				
				u32 ArraySize = ArrayCount(DescriptorWrites);
				vkUpdateDescriptorSets(LogicalDevice, ArraySize, DescriptorWrites, 0, 0);
			}
			// End of Create World Descriptors
			
			// Create UI Descriptors
			for(u32 Index = 0;
				Index < MAX_FRAMES_IN_FLIGHT;
				++Index)
			{
				VkDescriptorBufferInfo BufferInfo = {};
				BufferInfo.buffer = UniformBuffer.data[Index];
				BufferInfo.offset = 0;
				BufferInfo.range = sizeof(UniformBufferObject);
				
				VkDescriptorImageInfo ImageInfo = {};
				ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				ImageInfo.imageView = TextureImageViews[0];
				ImageInfo.sampler = TextureSampler;
				
				VkWriteDescriptorSet DescriptorWrites[2] = {{},{}};
				
				DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				DescriptorWrites[0].dstSet = ui_DescriptorSets[Index];
				DescriptorWrites[0].dstBinding = 0;
				DescriptorWrites[0].dstArrayElement = 0;
				DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				DescriptorWrites[0].descriptorCount = 1;
				DescriptorWrites[0].pBufferInfo = &BufferInfo;
				
				DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				DescriptorWrites[1].dstSet = ui_DescriptorSets[Index];
				DescriptorWrites[1].dstBinding = 1;
				DescriptorWrites[1].dstArrayElement = 0;
				DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				DescriptorWrites[1].descriptorCount = 1;
				DescriptorWrites[1].pImageInfo = &ImageInfo;
				
				u32 ArraySize = ArrayCount(DescriptorWrites);
				vkUpdateDescriptorSets(LogicalDevice, ArraySize, DescriptorWrites, 0, 0);
			}
			// End of Create UI Descriptors
			
			//
			//
			// End of VULKAN Initialization
			
			ShowWindow(Window, SW_SHOW);
			
			// NOTE(Denis): Main engine loop
			while(GlobalRunning)
			{
				NewInput->dtForFrame = TargetSecondsPerFrame;
				game_controller_input *OldKeyboardController = GetController(OldInput,0);
				game_controller_input *NewKeyboardController = GetController(NewInput,0);
				*NewKeyboardController = {};
				NewKeyboardController->IsConntected = true;
				
				// NOTE(Denis): Process keyboard input
				for(int ButtonIndex=  0;
					ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
					++ButtonIndex)
				{
					NewKeyboardController->Buttons[ButtonIndex].EndedDown =
						OldKeyboardController->Buttons[ButtonIndex].EndedDown;
				}
				Win32ProcessPendingMessages(&Win32State,NewKeyboardController);
				
				// NOTE(Denis): Process mouse input
				DWORD WinButtonID[PlatformMouseButton_Count] = 
				{
					VK_LBUTTON,
					VK_MBUTTON,
					VK_RBUTTON,
					VK_XBUTTON1,
					VK_XBUTTON2,
				};
				for(u32 ButtonIndex = 0;
					ButtonIndex < PlatformMouseButton_Count;
					++ButtonIndex)
				{
					NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
					NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
					Win32ProcessKeyboardMessage(&NewInput->MouseButtons[ButtonIndex],
												GetKeyState(WinButtonID[ButtonIndex]) & (1 << 15));
				}
				
				if(!GlobalPause)
				{
					// NOTE(Denis): Update mouse position inside Input struct
					POINT MouseP;
					GetCursorPos(&MouseP);
					ScreenToClient(Window,&MouseP);
					NewInput->MouseX = (f32)MouseP.x;
					NewInput->MouseY = (f32)(((f32)SwapChainExtent.height - 1) - MouseP.y);
					NewInput->MouseZ = 0; // TODO(Denis): Support mouse whell ?
					NewInput->MouseZ = MouseWheelDelta; // TODO(Denis): Support mouse whell ?
					MouseWheelDelta = 0; // NOTE(Denis): Reset mouse wheel delta to be 0 after we use it
					
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
						SetCursorPos((u32)SwapChainExtent.width - 2 + WindowP.x, WindowP.y);
						ScreenWrapOffsetX = (f32)SwapChainExtent.width - 2;
					} 
					else if(NewInput->MouseY < 1)
					{
						SetCursorPos(WindowP.x, WindowP.y - SwapChainExtent.height - 2);
						ScreenWrapOffsetY = (f32)SwapChainExtent.height - 2;
					} 
					else if(NewInput->MouseX > SwapChainExtent.width - 2)
					{
						SetCursorPos(WindowP.x - (SwapChainExtent.width - 2), WindowP.y);
						ScreenWrapOffsetX = -(f32)SwapChainExtent.width + 2;
					} 
					else if(NewInput->MouseY > SwapChainExtent.height - 2)
					{
						SetCursorPos(WindowP.x, WindowP.y + (SwapChainExtent.height - 2));
						ScreenWrapOffsetY = -(f32)SwapChainExtent.height + 2;
					}
					
					RECT window_rect = {};
					GetWindowRect(Window, &window_rect);
					
					ClipCursor(&window_rect);
					
					f32 x_offset = MouseOldPx - NewInput->MouseX;
					f32 y_offset = MouseOldPy - NewInput->MouseY;
					
					MouseOldPx = NewInput->MouseX + ScreenWrapOffsetX;
					MouseOldPy = NewInput->MouseY + ScreenWrapOffsetY;
					
					NewInput->MouseDeltaX = -x_offset / (f32)SwapChainExtent.width;
					NewInput->MouseDeltaY = y_offset / (f32)SwapChainExtent.height;
					
					NewInput->ShiftDown = (GetKeyState(VK_SHIFT) & (1 << 15));
					NewInput->AltDown = (GetKeyState(VK_MENU) & (1 << 15));
					NewInput->ControlDown = (GetKeyState(VK_CONTROL) & (1 << 15));
					
					// TODO(Denis): Create loading this function from our endgine dll
					EngineUpdateAndRender(&GameMemory, NewInput, &EngineRenderCommands);
					
					if(GameMemory.QuitRequested)
					{
						GlobalRunning = false;
					}
					
					// RENDERING FRAME
					//
					//
					
					vkWaitForFences(LogicalDevice, 1, &InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);
					
					// Acquire an image from the swap chain
					VkResult Result = vkAcquireNextImageKHR(LogicalDevice, SwapChain, UINT64_MAX, ImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);
					
					if(Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || GlobalFramebufferResized)
					{
						GlobalFramebufferResized = false;
						vk_RecreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice,
											 VulkanWindowSurface, QueueIndices, QueuePriority,
											 CreateDeviceInfo, LogicalDevice, &CommandPool, &GraphicQueue, SwapChainImageFormat, world_framebuffers, swap_chain_framebuffers, ArrayCount(world_framebuffers), ArrayCount(swap_chain_framebuffers), SwapChainImageViews,ArrayCount(SwapChainImageViews), DepthImageView, &DepthImage, &DepthImageMemory, ColorImageView, &ColorImage, &ColorImageMemory, SwapChain, SwapChainExtent, PresentQueue, SwapChainImages, MipLevels, ArrayCount(SwapChainImages), WorldRenderPass, UIRenderPass, ImageCount, MSAA_Samples);
						FirstStart = true; // We need these for mouse delta to be reset every time we resize window
					}
					
					vkResetFences(LogicalDevice, 1, &InFlightFences[CurrentFrame]);
					// End of Acquire an image from the swap chain
					
					// Reset command buffer!
					vkResetCommandBuffer(CommandBuffersWorld[CurrentFrame], 0);
					vkResetCommandBuffer(CommandBuffersUI[CurrentFrame], 0);
					vkResetCommandBuffer(CommandBuffers_ui_text[CurrentFrame], 0);
					
					// NOTE(Denis): Unpack and process render commands from engine update
					engine_render_commands Commands = EngineRenderCommands;
					engine_render_entry_header *Header = 0;
					u32 offset = 0;
					for(u32 index = 0;
						index < Commands.CommandBufferElementCount;
						index++)
					{
						Header = (engine_render_entry_header *)(Commands.CommandBufferBase + offset);
						
						switch(Header->Type)
						{
							case EngineRenderCommandType_Quad:
							{
								// COMMAND BUFFER RECORDING
								vk_CommandBuffersBegin(&CommandBuffersUI[CurrentFrame], 1);
								
								vertex_3d *Vertices = (vertex_3d *)Header->command_quad.verts_array;
								u32 *Indices = (u32 *)Header->command_quad.indices_array;
								
								VkDeviceSize VertexBufferSize = Header->command_quad.vertex_count * sizeof(vertex_3d);
								VkDeviceSize IndexBufferSize = Header->command_quad.indices_count * sizeof(u32);
								
								VkDeviceSize WholeVertSize = ui_vertices_count * sizeof(vertex_3d);
								VkDeviceSize WholeIndexSize = ui_indices_count * sizeof(u32);
								
								void *Data;
								vkMapMemory(LogicalDevice, *ui_vertex_buffer.memory, 0, VertexBufferSize, 0, &Data);
								memset(Data, 0, WholeVertSize); // Reset GPU memory data to zeroes
								memcpy(Data, Vertices, VertexBufferSize);
								
								vkMapMemory(LogicalDevice, *ui_index_buffer.memory, 0, WholeIndexSize, 0, &Data);
								memset(Data, 0, WholeIndexSize); // Reset GPU memory data to zeroes
								memcpy(Data, Indices, IndexBufferSize);
								
								vk_RecordCommandBuffer(CommandBuffersUI[CurrentFrame], UIRenderPass, material_pipelines[MaterialType_UI], ui_pipeline_layout, world_descriptor_sets, swap_chain_framebuffers, SwapChainExtent, *ui_vertex_buffer.data, *ui_index_buffer.data, ui_indices_count, CurrentFrame, ImageIndex, true);
								
								vk_CommandBuffersEnd(&CommandBuffersUI[CurrentFrame], 1);
								
							} break;
							
							case EngineRenderCommandType_TextRect:
							{
								// COMMAND BUFFER RECORDING
								vk_CommandBuffersBegin(&CommandBuffers_ui_text[CurrentFrame], 1);
								
								vertex_3d *Vertices = (vertex_3d *)Header->command_text_rect.verts_array;
								u32 *Indices = (u32 *)Header->command_text_rect.indices_array;
								
								VkDeviceSize VertexBufferSize = Header->command_text_rect.vertex_count * sizeof(vertex_3d);
								VkDeviceSize IndexBufferSize = Header->command_text_rect.indices_count * sizeof(u32);
								
								VkDeviceSize WholeVertSize = text_ui_vertices_count * sizeof(vertex_3d);
								VkDeviceSize WholeIndexSize = text_indices_count * sizeof(u32);
								
								void *Data;
								vkMapMemory(LogicalDevice, ui_text_vertex_buffer_memory, 0, VertexBufferSize, 0, &Data);
								memset(Data, 0, WholeVertSize); // Reset GPU memory data to zeroes
								memcpy(Data, Vertices, VertexBufferSize);
								
								vkMapMemory(LogicalDevice, ui_text_index_buffer_memory, 0, IndexBufferSize, 0, &Data);
								memset(Data, 0, WholeIndexSize); // Reset GPU memory data to zeroes
								memcpy(Data, Indices, IndexBufferSize);
								
								if(!TextureImageViews[0])
								{
									TextureImageViews[0] = vk_CreateImageView(LogicalDevice, TextureImages[0], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, MipLevels);
								}
								
								vk_RecordCommandBuffer(CommandBuffers_ui_text[CurrentFrame], UIRenderPass, material_pipelines[MaterialType_UIText], ui_pipeline_layout, ui_DescriptorSets, swap_chain_framebuffers, SwapChainExtent, ui_text_vertex_buffer, ui_text_index_buffer, text_indices_count, CurrentFrame, ImageIndex, true);
								
								vk_CommandBuffersEnd(&CommandBuffers_ui_text[CurrentFrame], 1);
								
							} break;
							
							case EngineRenderCommandType_UpdateCamera:
							{
								// COMMAND BUFFER RECORDING
								vk_CommandBuffersBegin(&CommandBuffersWorld[CurrentFrame], 1);
								
								// Starting a Render Pass
								VkRenderPassBeginInfo RenderPassBeginInfo = {};
								RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
								RenderPassBeginInfo.renderPass = WorldRenderPass;
								RenderPassBeginInfo.framebuffer = world_framebuffers[ImageIndex];
								RenderPassBeginInfo.renderArea.offset = {0, 0};
								RenderPassBeginInfo.renderArea.extent = SwapChainExtent;
								
								// NOTE(Denis): Note that the order of clearValues should be identical to the order of our attachments.
								VkClearValue ClearValues[3];
								ClearValues[0].color = {{0.245f, 0.339f, 0.378f, 1.0f}};
								ClearValues[1].depthStencil = {1.0f,0};
								ClearValues[2].color = {{0.245f, 0.339f, 0.378f, 1.0f}};
								
								// Basic drawing commands
								VkViewport Viewport = {};
								Viewport.x = 0.0f;
								Viewport.y = 0.0f;
								Viewport.width = (f32)SwapChainExtent.width;
								Viewport.height = (f32)SwapChainExtent.height;
								Viewport.minDepth = 0;
								Viewport.maxDepth = 1.0f;
								
								VkRect2D Scissor = {};
								Scissor.offset = {0, 0};
								Scissor.extent = SwapChainExtent;
								
								vkCmdSetViewport(CommandBuffersWorld[CurrentFrame], 0, 1, &Viewport);
								vkCmdSetScissor(CommandBuffersWorld[CurrentFrame], 0, 1, &Scissor);
								RenderPassBeginInfo.clearValueCount = ArrayCount(ClearValues);
								RenderPassBeginInfo.pClearValues = ClearValues;
								
								vkCmdBeginRenderPass(CommandBuffersWorld[CurrentFrame], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
								vkCmdBindPipeline(CommandBuffersWorld[CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, material_pipelines[MaterialType_Default]);
								
								VkDeviceSize Offsets[] = {0};
								vkCmdBindVertexBuffers(CommandBuffersWorld[CurrentFrame], 0, 1, WorldVertexBuffer.data, Offsets);
								vkCmdBindIndexBuffer(CommandBuffersWorld[CurrentFrame], *WorldIndexBuffer.data, 0, VK_INDEX_TYPE_UINT32);
								
								// Rendering world models
								
								// Need to make optimization about which model matrices to update (some kind of list of indices)
								// Alghorithm like this: when engine changes transform of any model, we need to add index of 
								// this transform to array and write count of objects in list, which later i will use as 
								// count of objects to update in this one frame.
								{
									u32 last_offset = 0;
									for(u32 i = 0; i < models_count_in_use; ++i)
									{
										mat4 View = Header->command_update_camera.view_mat4x4;
										mat4 Proj = Header->command_update_camera.proj_mat4x4;
										mat4 Model = Header->command_update_camera.model_mats4x4[i];
										mat4 MVP_Matrix = Proj * View * Model;
										
										UniformBufferObject Ubo = {};
										Ubo.MVP_Final = MVP_Matrix;
										Ubo.Model = Model;
										Ubo.View = View;
										Ubo.Proj = Proj;
										
										u32 current_frame_data_idx = i + CurrentFrame * models_count_in_use;
										
										void *Data;
										vkMapMemory(LogicalDevice, UniformBuffer.memory[current_frame_data_idx], 0, sizeof(Ubo), 0, &Data);
										memcpy(Data, &Ubo, sizeof(Ubo));
										
										vkCmdBindDescriptorSets(CommandBuffersWorld[CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, world_pipeline_layout, 0, 1, &world_descriptor_sets[current_frame_data_idx], 0, nullptr);
										vkCmdDrawIndexed(CommandBuffersWorld[CurrentFrame], world_indices_offsets[i], 1, last_offset, 0, 0);
										
										last_offset += world_indices_offsets[i];
									}
								}
								
								// Rendering world grid
								{
									vkCmdBindPipeline(CommandBuffersWorld[CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, material_pipelines[MaterialType_WorldGrid]);
									vkCmdBindDescriptorSets(CommandBuffersWorld[CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, world_pipeline_layout, 0, 1, &world_descriptor_sets[CurrentFrame * models_count_in_use], 0, 0);
									vkCmdDraw(CommandBuffersWorld[CurrentFrame], 6, 1, 0, 0);
								}
								
								vkCmdEndRenderPass(CommandBuffersWorld[CurrentFrame]);
								vk_CommandBuffersEnd(&CommandBuffersWorld[CurrentFrame], 1);
							} break;
							
							case EngineRenderCommandType_None:
							{
								
							} break;
							
							InvalidDefaultCase;
						}
						
						offset += sizeof(engine_render_entry_header);
					}
					
					VkCommandBuffer CommandBuffers[] = {CommandBuffersWorld[CurrentFrame], CommandBuffersUI[CurrentFrame], CommandBuffers_ui_text[CurrentFrame]};
					VkSubmitInfo SubmitInfo = {};
					SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					
					VkSemaphore WaitSemaphores[] = {ImageAvailableSemaphores[CurrentFrame]};
					VkPipelineStageFlags WaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
					SubmitInfo.waitSemaphoreCount = 1;
					SubmitInfo.pWaitSemaphores = WaitSemaphores;
					SubmitInfo.pWaitDstStageMask = WaitStages;
					SubmitInfo.commandBufferCount = ArrayCount(CommandBuffers);
					SubmitInfo.pCommandBuffers = CommandBuffers;
					
					VkSemaphore SignalSemaphores[] = {RenderFinishedSemaphores[CurrentFrame]};
					SubmitInfo.signalSemaphoreCount = 1;
					SubmitInfo.pSignalSemaphores = SignalSemaphores;
					
					b32 IsQueueSubmited = vkQueueSubmit(GraphicQueue, 1, &SubmitInfo, InFlightFences[CurrentFrame]) == VK_SUCCESS;
					Assert(IsQueueSubmited);
					
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
						vk_RecreateSwapChain(QueueFamilyCount, QueueFamilies, PhysicalDevice,
											 VulkanWindowSurface, QueueIndices, QueuePriority,
											 CreateDeviceInfo, LogicalDevice, &CommandPool, &GraphicQueue, SwapChainImageFormat, world_framebuffers, swap_chain_framebuffers, ArrayCount(world_framebuffers), ArrayCount(swap_chain_framebuffers), SwapChainImageViews,ArrayCount(SwapChainImageViews), DepthImageView, &DepthImage, &DepthImageMemory, ColorImageView, &ColorImage, &ColorImageMemory, SwapChain, SwapChainExtent, PresentQueue, SwapChainImages, MipLevels, ArrayCount(SwapChainImages), WorldRenderPass, UIRenderPass, ImageCount, MSAA_Samples);
						FirstStart = true; // We need these for mouse delta to be reset every time we resize window
					}
					
					// End of Recording the command buffer
					
					CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
					
					//
					//
					// END OF RENDERING FRAME
					
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
					
					// NOTE(Denis): Record old input
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
					
					// NOTE(Denis): Frame time elapsed
					LARGE_INTEGER EndCounter = Win32GetWallClock();
					f32 SecondsElapsed = Win32GetSecondsElapsed(LastCounter, EndCounter);
					LastCounter = EndCounter;
					
					GameMemory.FrameSecondsElapsed = SecondsElapsed;
				}
				else
				{
					FirstStart = true;
				}
			}
		}
	}
	
	VirtualFree(Win32State.GameMemoryBlock, 0, MEM_RELEASE);
	
	return 0;
	
}