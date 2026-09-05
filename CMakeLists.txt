cmake_minimum_required(VERSION 4.2)

project(Vortex LANGUAGES CXX)

file(GLOB_RECURSE SOURCE_LIST CONFIGURE_DEPENDS Engine/*.cpp)
file(GLOB_RECURSE HEADER_LIST CONFIGURE_DEPENDS Engine/*.hpp)
file(GLOB_RECURSE MODULE_LIST CONFIGURE_DEPENDS Engine/*.cppm)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine PREFIX "Source Files" FILES ${SOURCE_LIST})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine PREFIX "Header Files" FILES ${HEADER_LIST})

set_source_files_properties(${HEADER_LIST} PROPERTIES
	LANGUAGE CXX
	HEADER_FILE_ONLY OFF
)

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

get_target_property(GLM_MODULE_ROOT glm::glm-header-only INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(VULKAN_MODULE_ROOT Vulkan::Headers INTERFACE_INCLUDE_DIRECTORIES)

add_executable(Vortex ${SOURCE_LIST})

target_sources(Vortex PRIVATE
	FILE_SET vortex_modules TYPE CXX_MODULES
	FILES ${HEADER_LIST} ${MODULE_LIST}
)

target_sources(Vortex PRIVATE
	FILE_SET dependency_modules TYPE CXX_MODULES
	BASE_DIRS ${GLM_MODULE_ROOT} ${VULKAN_MODULE_ROOT}
	FILES  ${GLM_MODULE_ROOT}/glm/glm.cppm ${VULKAN_MODULE_ROOT}/vulkan/vulkan.cppm
)

target_compile_definitions(Vortex PRIVATE 
	GLM_ENABLE_EXPERIMENTAL
	VULKAN_HPP_CXX_MODULE_EXPERIMENTAL_WARNING
	VDEBUG=$<IF:$<CONFIG:Debug>,1,0>
)

target_link_libraries(Vortex PRIVATE
	Vulkan::Vulkan
	SDL3::SDL3
	glm::glm
	imgui::imgui
	yaml-cpp::yaml-cpp
	vk-bootstrap::vk-bootstrap
	GPUOpen::VulkanMemoryAllocator
)

add_custom_target(build_shaders ALL
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/compile_shaders.py --output-dir "$<TARGET_FILE_DIR:Vortex>/Shaders"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Engine/Shaders
	COMMENT "Compiling shaders..."
)

add_custom_target(copy_resources ALL
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/copy_resources.py ${CMAKE_SOURCE_DIR} "$<TARGET_FILE_DIR:Vortex>"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Copying resources..."
)

add_dependencies(Vortex build_shaders copy_resources)
