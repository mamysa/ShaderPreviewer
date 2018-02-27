cmake_minimum_required (VERSION 3.1.3)
project (ShaderPreviewV2)
set (SOURCE_FILES 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${PROJECT_SOURCE_DIR}/src/ResourceManager.cpp
	${PROJECT_SOURCE_DIR}/src/Renderer.cpp
	${PROJECT_SOURCE_DIR}/src/Input.cpp
	${PROJECT_SOURCE_DIR}/src/Logger.cpp
	${PROJECT_SOURCE_DIR}/src/opengl/ShaderProgram.cpp
	${PROJECT_SOURCE_DIR}/src/opengl/Mesh.cpp
	${PROJECT_SOURCE_DIR}/src/opengl/Texture.cpp
	${PROJECT_SOURCE_DIR}/src/utils/Math.cpp
	${PROJECT_SOURCE_DIR}/src/ui/UI.cpp
	${PROJECT_SOURCE_DIR}/src/ui/UIRender.cpp
	${PROJECT_SOURCE_DIR}/libs/imgui/imgui_draw.cpp
	${PROJECT_SOURCE_DIR}/libs/imgui/imgui.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/opengl/)
include_directories(${PROJECT_SOURCE_DIR}/utils/)
include_directories(${PROJECT_SOURCE_DIR}/ui/)
include_directories(${PROJECT_SOURCE_DIR}/libs/imgui/)


add_executable(ShaderPreviewV2 ${SOURCE_FILES})

if (WIN32)

	add_definitions(-DIS_WINDOWS) 
	include_directories(${PROJECT_SOURCE_DIR}/libs/SDL2-2.0.5/include/)
	include_directories(${PROJECT_SOURCE_DIR}/libs/glew2.0.0/include/)
	find_package(OPENGL REQUIRED)
	include_directories(${OPENGL_INCLUDE_DIRS})
	
	target_link_libraries(ShaderPreviewV2 ${PROJECT_SOURCE_DIR}/libs/glew2.0.0/lib/x64/glew32.lib)
	target_link_libraries(ShaderPreviewV2 ${OPENGL_LIBRARY})
	target_link_libraries(ShaderPreviewV2 ${PROJECT_SOURCE_DIR}/libs/SDL2-2.0.5/lib/x64/SDL2main.lib)
	target_link_libraries(ShaderPreviewV2 ${PROJECT_SOURCE_DIR}/libs/SDL2-2.0.5/lib/x64/SDL2.lib)
endif(WIN32)
	
if (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
	add_definitions(-DIS_OSX) 
	find_package(SDL2 REQUIRED)
	#find_package(GLEW REQUIRED)
	find_package(OPENGL REQUIRED)
	include_directories(${SDL2_INCLUDE_DIR})
	#include_directories(${GLEW_INCLUDE_DIRS})
	include_directories(${OPENGL_INCLUDE_DIR})

	target_link_libraries(ShaderPreviewV2 ${SDL2_LIBRARIES})
	#target_link_libraries(ShaderPreviewV2 ${GLEW_LIBRARY})
	target_link_libraries(ShaderPreviewV2 ${OPENGL_LIBRARY})
endif(APPLE)
