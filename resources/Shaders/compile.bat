
for %%i in (*.vert *.frag *.comp) do "C:/VulkanSDK/1.3.275.0/Bin/glslangValidator.exe" -V "%%~i" -o "%%~i.spv"
pause