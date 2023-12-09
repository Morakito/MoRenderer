# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MoRenderer")


# 复制资源文件夹到输出路径
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory 
${PROJECT_SOURCE_DIR}/assets
${CMAKE_BINARY_DIR}/assets)

 execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory 
${PROJECT_SOURCE_DIR}/tools
${CMAKE_BINARY_DIR}/tools)

include_directories ("Library")

# 包含子项目。
add_subdirectory ("MoRenderer")

