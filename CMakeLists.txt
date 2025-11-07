cmake_minimum_required(VERSION 4.0)

project(Vortex)

file(GLOB_RECURSE SRC_LIST Engine/*.cpp)
file(GLOB_RECURSE INC_LIST Engine/*.hpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine PREFIX "Source Files" FILES ${SRC_LIST})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine PREFIX "Header Files" FILES ${INC_LIST})

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_executable(Vortex
	${SRC_LIST}
	${INC_LIST}
)

target_include_directories(Vortex
	PRIVATE Engine
)

target_compile_definitions(Vortex
    PRIVATE GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(Vortex
	Vulkan::Vulkan
	SDL3::SDL3
	glm::glm
	imgui::imgui
	spdlog::spdlog
	nlohmann_json::nlohmann_json
)

set(SHADER_LIST
	Forward/*.slang
	Deferred/*.slang
)

add_custom_target(build_shaders ALL
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/compile_shaders.py ${SHADER_LIST}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Shaders
	COMMENT "Compiling shaders..."
)

add_custom_target(copy_resources ALL
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/copy_resources.py ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS build_shaders
	COMMENT "Copying resources..."
)

add_dependencies(Vortex
	build_shaders
	copy_resources
)

install(TARGETS Vortex
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(DIRECTORY
	${CMAKE_BINARY_DIR}/Assets 
	${CMAKE_BINARY_DIR}/Configs
	${CMAKE_BINARY_DIR}/Shaders
	DESTINATION ${CMAKE_INSTALL_PREFIX}
)
