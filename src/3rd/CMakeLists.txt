add_library(trde_murmurhash smhasher/src/MurmurHash3.cpp smhasher/src/MurmurHash2.cpp smhasher/src/MurmurHash1.cpp)
target_include_directories(trde_murmurhash PUBLIC smhasher/src/)
IF(LINUX)
target_compile_options(trde_murmurhash PUBLIC -march=native)
ENDIF()
add_library(trde::murmurhash ALIAS trde_murmurhash)


add_subdirectory(Catch2)