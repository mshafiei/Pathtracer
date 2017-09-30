set artydir=%~dp0
mkdir build
cd build
cmake -G "Visual Studio 15 Win64" -D PNG_LIBRARY="%artydir%contrib/lib/libpng16.lib" -D SDL2_LIBRARY="%artydir%contrib/lib/SDL2.lib;%artydir%contrib/lib/SDL2main.lib" -D SDL2_INCLUDE_DIR="%artydir%contrib/include/SDL2" -D ZLIB_LIBRARY="%artydir%contrib/lib/zlib.lib" -D ZLIB_INCLUDE_DIR="%artydir%contrib/include/zlib" -D PNG_PNG_INCLUDE_DIR="%artydir%contrib/include/lpng1629" -D PNG_LIBRARIES="%artydir%contrib/lib/libpng16.lib" -D YAML_CPP_LIBRARIES="%artydir%contrib/lib/yaml-cpp.lib" -D YAML_CPP_INCLUDE_DIR="%artydir%contrib/include/yaml" ..
