@echo off
REM Build the Star Trail LVGL PC simulator with MSVC + vcpkg SDL2.
REM SDL2 comes from the local vcpkg install; LVGL v8.3.9 is fetched by CMake.
set "SIM=C:\Users\varsh\OneDrive\Documents\Benz\CrowPanel_Repo\.claude\worktrees\compassionate-moser-ec21ce\simulator"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -S "%SIM%" -B "%SIM%\build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows || exit /b 1
cmake --build "%SIM%\build" --config Release || exit /b 1
echo BUILD_OK
