# cmake/toolchains/clang.cmake
set(CMAKE_C_COMPILER   clang   CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)

# lld 고정(원하면)
set(CMAKE_LINKER       ld.lld  CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER_LINKER   ld.lld CACHE STRING "" FORCE)
set(CMAKE_C_COMPILER_LINKER     ld.lld CACHE STRING "" FORCE)