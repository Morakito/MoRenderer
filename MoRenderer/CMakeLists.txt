# CMakeList.txt: MoRenderer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。

# 将源代码添加到此项目的可执行文件。
add_executable (MoRenderer  "MoRenderer.cpp" "MoRenderer.h" 
"vector.h"  "matrix.h" "math.h"  
"Window.h" "Window.cpp" 
"Texture.h" "Texture.cpp"
"Model.h" "Model.cpp"
"Camera.h" "Camera.cpp"
  
"main.cpp"  "Shader.h" "Shader.cpp")

set_target_properties(
    MoRenderer
    PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
               LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
)


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MoRenderer PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
