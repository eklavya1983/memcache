cmake_minimum_required(VERSION 3.2)
project("test")

macro(add_unit_test name)
  add_executable(${name}Test ${name}Test.cpp ${ARGN})
  target_link_libraries(${name}Test
                         memcache wangle folly zstd ssl ${COMMON_TEST_LIBS})
  add_test(${name} ${EXECUTABLE_OUTPUT_PATH}/${name}Test)
  install (TARGETS ${name}Test DESTINATION test)
endmacro()

include_directories(${GTEST_INCLUDE_DIRS})
link_directories(${GTEST_LIB_DIRS} ${ARTIFACTS_LIB_DIRS})

add_unit_test(MemcacheService)
