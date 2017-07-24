cmake_minimum_required (VERSION 3.1.3)
project (ShaderPreviewV2)






add_executable(ShaderPreviewV2 
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${PROJECT_SOURCE_DIR}/src/ShaderWatcher.cpp
	${PROJECT_SOURCE_DIR}/src/opengl/ShaderProgram.cpp
	${PROJECT_SOURCE_DIR}/src/opengl/Mesh.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/libs/SDL2-2.0.5/include/)
include_directories(${PROJECT_SOURCE_DIR}/libs/glew2.0.0/include/)
include_directories(${PROJECT_SOURCE_DIR}/libs/glm/glm)
find_package(OPENGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/opengl/)

target_link_libraries(ShaderPreviewV2 ${PROJECT_SOURCE_DIR}/libs/SDL2-2.0.5/lib/x64/SDL2main.lib)
target_link_libraries(ShaderPreviewV2 ${PROJECT_SOURCE_DIR}/libs/SDL2-2.0.5/lib/x64/SDL2.lib)
target_link_libraries(ShaderPreviewV2 ${PROJECT_SOURCE_DIR}/libs/glew2.0.0/lib/x64/glew32.lib)
target_link_libraries(ShaderPreviewV2 ${OPENGL_LIBRARY})
