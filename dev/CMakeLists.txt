
cmake_minimum_required(VERSION 3.15)
project(PackageTest C)

find_package(libgit2 REQUIRED)

# examples: code usage examples of libgit2

file(GLOB SRC_EXAMPLES *.c *.h)

add_executable(lgtest ${SRC_EXAMPLES})
set_target_properties(lgtest PROPERTIES C_STANDARD 90)

# Ensure that we do not use deprecated functions internally
add_definitions(-DGIT_DEPRECATE_HARD)

target_include_directories(lgtest PRIVATE ${LIBGIT2_INCLUDES} ${LIBGIT2_DEPENDENCY_INCLUDES})
target_include_directories(lgtest SYSTEM PRIVATE ${LIBGIT2_SYSTEM_INCLUDES})

target_link_libraries(lgtest libgit2::libgit2 pthread)
