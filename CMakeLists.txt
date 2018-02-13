cmake_minimum_required (VERSION 3.1.3)
project (ShaderPreviewV2)

set (SOURCE_FILES 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${PROJECT_SOURCE_DIR}/src/ResourceManager.cpp
	${PROJECT_SOURCE_DIR}/src/Renderer.cpp
	${PROJECT_SOURCE_DIR}/src/Input.cpp
	${PROJECT_SOURCE_DIR}/src/opengl/ShaderProgram.cpp
	${PROJECT_SOURCE_DIR}/src/opengl/Mesh.cpp
	${PROJECT_SOURCE_DIR}/src/utils/Math.cpp
)
	add_executable(ShaderPreviewV2 WIN32 ${SOURCE_FILES} )

include_directories(${PROJECT_SOURCE_DIR}/libs/glew2.0.0/include/)
find_package(OPENGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/opengl/)
include_directories(${PROJECT_SOURCE_DIR}/utils/)


target_link_libraries(ShaderPreviewV2 ${PROJECT_SOURCE_DIR}/libs/glew2.0.0/lib/x64/glew32.lib)
target_link_libraries(ShaderPreviewV2 ${OPENGL_LIBRARY})
