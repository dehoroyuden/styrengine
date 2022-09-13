@echo off
pushd ..\data\shaders
call C:\VulkanSDK\1.3.216.0\Bin\glslc.exe ..\..\code\shaders\%*.vert -o %*_vert.spv
call C:\VulkanSDK\1.3.216.0\Bin\glslc.exe ..\..\code\shaders\%*.frag -o %*_frag.spv
popd