@echo off

REM -wd4456
REM -wd4459
REM -wd4457
REM -wd4477
REM -wd4312

set CommonCompilerFlags=-Od -MTd -nologo -fp:fast -Gm- -GR- -EHa- -WX /EHsc -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4456 -wd4459 -wd4505 -wd4457 -wd4311 -wd4302 -wd4127 -wd4477 -wd4312 -FC -Z7 /I "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133\include" /I "C:\VulkanSDK\1.3.216.0\Include"
set CommonCompilerFlags= -DSTYR_SLOW=1 -DSTYR_INTERNAL=1 -DSTYR_VULKAN_VALIDATION=1 -DSTYR_LOAD_MODEL_OPTIMIZED=0 %CommonCompilerFlags%
set CommonLinkerFlags= /libpath:"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133\lib\x64" /libpath:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.19041.0\um\x64" /libpath:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.19041.0\ucrt\x64" /libpath:"C:\VulkanSDK\1.3.216.0\Lib" /libpath:"C:\VulkanSDK\1.3.216.0\Lib32" /libpath:"C:\glfw-3.3.8.bin.WIN64\lib-vc2022" -opt:ref user32.lib Gdi32.lib winmm.lib vulkan-1.lib

set CommonLinkerFlagsDLL= /libpath:"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133\lib\x64" /libpath:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.19041.0\um\x64" /libpath:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.19041.0\ucrt\x64"

IF NOT EXIST ..\..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL

echo WAITING FOR PDB > lock.tmp

del lock.tmp

cl %CommonCompilerFlags% -Fmwin32_styr.map ..\code\win32_styr.cpp /link %CommonLinkerFlags% 

popd