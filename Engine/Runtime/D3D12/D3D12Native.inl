#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#undef GetMessage
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <pix.h>

#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#include <D3D12MemAlloc.h>
#undef D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED

namespace D3D12NativeConstants {

inline constexpr auto cp_utf8 = CP_UTF8;
inline constexpr auto infinite = INFINITE;
inline constexpr auto wait_object_0 = WAIT_OBJECT_0;
inline constexpr auto d3d12_constant_buffer_data_placement_alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
inline constexpr auto d3d12_default_shader_4_component_mapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
inline constexpr auto d3d12_descriptor_range_offset_append = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
inline constexpr auto d3d12_float32_max = D3D12_FLOAT32_MAX;
inline constexpr auto d3d12_resource_barrier_all_subresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
inline constexpr auto dxgi_create_factory_debug = DXGI_CREATE_FACTORY_DEBUG;
inline constexpr auto dxgi_error_not_found = DXGI_ERROR_NOT_FOUND;
inline constexpr auto dxgi_mwa_no_alt_enter = DXGI_MWA_NO_ALT_ENTER;
inline constexpr auto dxgi_usage_render_target_output = DXGI_USAGE_RENDER_TARGET_OUTPUT;

}        // namespace D3D12NativeConstants

#undef CP_UTF8
#undef INFINITE
#undef WAIT_OBJECT_0
#undef D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
#undef D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
#undef D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
#undef D3D12_FLOAT32_MAX
#undef D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
#undef DXGI_CREATE_FACTORY_DEBUG
#undef DXGI_ERROR_NOT_FOUND
#undef DXGI_MWA_NO_ALT_ENTER
#undef DXGI_USAGE_RENDER_TARGET_OUTPUT
#undef SUCCEEDED
#undef ERROR

inline constexpr auto CP_UTF8 = D3D12NativeConstants::cp_utf8;
inline constexpr auto INFINITE = D3D12NativeConstants::infinite;
inline constexpr auto WAIT_OBJECT_0 = D3D12NativeConstants::wait_object_0;
inline constexpr auto D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT = D3D12NativeConstants::d3d12_constant_buffer_data_placement_alignment;
inline constexpr auto D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING = D3D12NativeConstants::d3d12_default_shader_4_component_mapping;
inline constexpr auto D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND = D3D12NativeConstants::d3d12_descriptor_range_offset_append;
inline constexpr auto D3D12_FLOAT32_MAX = D3D12NativeConstants::d3d12_float32_max;
inline constexpr auto D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES = D3D12NativeConstants::d3d12_resource_barrier_all_subresources;
inline constexpr auto DXGI_CREATE_FACTORY_DEBUG = D3D12NativeConstants::dxgi_create_factory_debug;
inline constexpr auto DXGI_ERROR_NOT_FOUND = D3D12NativeConstants::dxgi_error_not_found;
inline constexpr auto DXGI_MWA_NO_ALT_ENTER = D3D12NativeConstants::dxgi_mwa_no_alt_enter;
inline constexpr auto DXGI_USAGE_RENDER_TARGET_OUTPUT = D3D12NativeConstants::dxgi_usage_render_target_output;

inline constexpr bool SUCCEEDED(HRESULT result) noexcept { return result >= 0; }
