cmake_minimum_required(VERSION 3.2)
project("src")

include_directories(${ARTIFACTS_INCLUDE_DIRS})
link_directories(${GTEST_LIB_DIRS} ${ARTIFACTS_LIB_DIRS})

#Library
add_library(memcache Types.cpp MemcacheService.cpp Shard.cpp)
install (TARGETS memcache DESTINATION lib)

#Executable
add_executable(memcache-main MemcacheMain.cpp)
target_link_libraries(memcache-main memcache wangle folly zstd ssl ${COMMON_LIBS})

add_subdirectory(test)

