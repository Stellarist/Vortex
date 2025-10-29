cmake_minimum_required(VERSION 4.0)

project(Vortex)

file(GLOB_RECURSE SRC_LIST Engine/*.cpp)
file(GLOB_RECURSE INC_LIST Engine/*.hpp)
file(GLOB_RECURSE SHADER_LIST Shaders/*.slang)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine PREFIX "Source Files" FILES ${SRC_LIST})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine PREFIX "Header Files" FILES ${INC_LIST})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Shaders PREFIX "Shader Files" FILES ${SHADER_LIST})

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_executable(Vortex
	${SRC_LIST}
	${INC_LIST}
)

target_include_directories(Vortex
	PRIVATE Engine
)

target_compile_definitions(Vortex
    PRIVATE
        SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        SHADER_DIR="${CMAKE_SOURCE_DIR}/Shaders"
        ASSETS_DIR="${CMAKE_SOURCE_DIR}/Assets"
        SCRIPTS_DIR="${CMAKE_SOURCE_DIR}/Scripts"
        GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(Vortex
	Vulkan::Vulkan
	SDL3::SDL3
	glm::glm
	imgui::imgui
)

add_custom_target(compile_shaders
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/compile_shaders.py
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS ${SHADER_LIST}
)

add_dependencies(Vortex 
	compile_shaders
)
