@echo off
pushd ..\data\shaders
call C:\VulkanSDK\1.3.216.0\Bin\glslc.exe %*.vert -o vert.spv
call C:\VulkanSDK\1.3.216.0\Bin\glslc.exe %*.frag -o frag.spv
popd