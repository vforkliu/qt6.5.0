cmake_minimum_required(VERSION 3.4.1)

# Validation layers could be built with code from
#    github repo   OR
#    ndk's snapshot
# The file structure for glslang spir-V is different, adding BUILD_IN_NDK variable
set(BUILD_IN_NDK OFF)
if (CMAKE_CURRENT_SOURCE_DIR MATCHES "sources/third_party/vulkan/src")
    set(BUILD_IN_NDK ON)
endif()

# Enable beta Vulkan extensions
add_definitions(-DVK_ENABLE_BETA_EXTENSIONS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror \
        -Wno-unused-function -Wno-unused-const-variable \
        -Wno-c++98-compat-pedantic -DVK_PROTOTYPES \
        -DVK_USE_PLATFORM_ANDROID_KHR")

if (BUILD_IN_NDK)
    # NDK directory structure:
    #  $ANDROID_NDK/sources/third_party/
    #        vulkan/src
    #        shaderc/third_party (for glslang & spirv-tools )
    # this build uses combined lib libshaderc.a for libSPIRV-Tools.a purpose
    get_filename_component(SRC_DIR
        "${ANDROID_NDK}/sources/third_party/vulkan/src" ABSOLUTE)
    get_filename_component(EXTERNAL_DIR
        "${SRC_DIR}/../../shaderc/third_party" ABSOLUTE)
    get_filename_component(SPIRV_LIB
        "${SRC_DIR}/../../shaderc/libs/${ANDROID_STL}/${ANDROID_ABI}/libshaderc.a"
        ABSOLUTE)
else ()
    # github directory structure:
    #   ${SRC_DIR}
    #   ${SRC_DIR}/build-android/external (for glslang, spirv-tools & shaderc )
    get_filename_component(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.."  ABSOLUTE)
    set(EXTERNAL_DIR "${SRC_DIR}/build-android/third_party/shaderc/third_party")
    set(SPIRV_LIB
        "${SRC_DIR}/build-android/third_party/shaderc/android_test/obj/local/${ANDROID_ABI}/libSPIRV-Tools.a")
endif()
set(COMMON_DIR "${SRC_DIR}/build-android/generated")

set(CMAKE_CXX_clone "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR \
        -fvisibility=hidden")
include_directories(${SRC_DIR}/include
                    ${COMMON_DIR}/include
                    ${SRC_DIR}/layers)
add_library(layer_utils STATIC
        ${SRC_DIR}/layers/vk_layer_config.cpp
        ${SRC_DIR}/layers/vk_layer_extension_utils.cpp
        ${SRC_DIR}/layers/vk_layer_utils.cpp
        ${SRC_DIR}/layers/generated/vk_format_utils.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone}")

# assume shaderc already built externally
add_library(SPIRV-Tools-prebuilt STATIC IMPORTED)
set_target_properties(SPIRV-Tools-prebuilt PROPERTIES IMPORTED_LOCATION
    ${SPIRV_LIB})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR \
                     -fvisibility=hidden -DXXH_NO_LONG_LONG")
add_library(VkLayer_khronos_validation SHARED
        ${SRC_DIR}/layers/state_tracker.cpp
        ${SRC_DIR}/layers/base_node.cpp
        ${SRC_DIR}/layers/buffer_state.cpp
        ${SRC_DIR}/layers/cmd_buffer_state.cpp
        ${SRC_DIR}/layers/device_memory_state.cpp
        ${SRC_DIR}/layers/image_state.cpp
        ${SRC_DIR}/layers/pipeline_state.cpp
        ${SRC_DIR}/layers/queue_state.cpp
        ${SRC_DIR}/layers/render_pass_state.cpp
        ${SRC_DIR}/layers/core_validation.cpp
        ${SRC_DIR}/layers/drawdispatch.cpp
        ${SRC_DIR}/layers/convert_to_renderpass2.cpp
        ${SRC_DIR}/layers/descriptor_sets.cpp
        ${SRC_DIR}/layers/descriptor_validation.cpp
        ${SRC_DIR}/layers/buffer_validation.cpp
        ${SRC_DIR}/layers/shader_module.cpp
        ${SRC_DIR}/layers/shader_validation.cpp
        ${SRC_DIR}/layers/generated/spirv_validation_helper.cpp
        ${SRC_DIR}/layers/generated/spirv_grammar_helper.cpp
        ${SRC_DIR}/layers/generated/command_validation.cpp
        ${SRC_DIR}/layers/gpu_validation.cpp
        ${SRC_DIR}/layers/gpu_utils.cpp
        ${SRC_DIR}/layers/debug_printf.cpp
        ${SRC_DIR}/layers/best_practices_utils.cpp
        ${SRC_DIR}/layers/sync_utils.cpp
        ${SRC_DIR}/layers/sync_vuid_maps.cpp
        ${SRC_DIR}/layers/core_error_location.cpp
        ${SRC_DIR}/layers/generated/synchronization_validation_types.cpp
        ${SRC_DIR}/layers/synchronization_validation.cpp
        ${COMMON_DIR}/include/layer_chassis_dispatch.cpp
        ${COMMON_DIR}/include/chassis.cpp
        ${COMMON_DIR}/include/parameter_validation.cpp
        ${SRC_DIR}/layers/parameter_validation_utils.cpp
        ${COMMON_DIR}/include/object_tracker.cpp
        ${SRC_DIR}/layers/object_tracker_utils.cpp
        ${COMMON_DIR}/include/thread_safety.cpp
        ${SRC_DIR}/layers/image_layout_map.cpp
        ${SRC_DIR}/layers/subresource_adapter.cpp
        ${SRC_DIR}/layers/xxhash.c)
target_include_directories(VkLayer_khronos_validation PRIVATE
        ${SRC_DIR}/include
        ${SRC_DIR}/layers
        ${COMMON_DIR}/include
        ${EXTERNAL_DIR}/glslang
        ${EXTERNAL_DIR}/spirv-tools/include
        ${EXTERNAL_DIR}/robin-hood-hashing/src/include)
target_link_libraries(VkLayer_khronos_validation PRIVATE
        log layer_utils SPIRV-Tools-prebuilt)
if (NOT BUILD_IN_NDK)
    set(SPIRV_OPT_LIB
        "${SRC_DIR}/build-android/third_party/shaderc/android_test/obj/local/${ANDROID_ABI}/libSPIRV-Tools-opt.a")
    add_library(SPIRV-Tools-opt-prebuilt STATIC IMPORTED)
    set_target_properties(SPIRV-Tools-opt-prebuilt PROPERTIES IMPORTED_LOCATION
        ${SPIRV_OPT_LIB})
    target_link_libraries(VkLayer_khronos_validation PRIVATE
        SPIRV-Tools-opt-prebuilt)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone}")
