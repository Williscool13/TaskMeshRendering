cmake_minimum_required(VERSION 4.0)
project(TaskMeshRendering)

set(CMAKE_CXX_STANDARD 20)

# Scroll all the way down for the important part
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
add_subdirectory(extern/fmt)
add_subdirectory(extern/SDL EXCLUDE_FROM_ALL)
add_subdirectory(extern/volk)
add_subdirectory(extern/vma)
add_subdirectory(extern/vk-bootstrap)
add_subdirectory(extern/fastgltf)
add_subdirectory(extern/OffsetAllocator)
add_subdirectory(extern/meshoptimizer)

# ===== Dear Imgui ===================================
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui)
add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_threaded_rendering.h
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_compile_definitions(imgui PUBLIC
        IMGUI_IMPL_VULKAN_USE_VOLK
        VK_NO_PROTOTYPES
)
target_include_directories(imgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/volk
        ${CMAKE_SOURCE_DIR}/extern/SDL/include
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(imgui PUBLIC
        volk
)
# ====================================================
# ====== Shader Compilation ==========================
find_program(SLANGC slangc REQUIRED)

function(compile_shader TARGET SHADER_SOURCE STAGE ENTRY)
    # Parse optional OUTPUT_NAME argument
    set(options "")
    set(oneValueArgs OUTPUT_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Use custom output name if provided, otherwise generate from source filename
    if(ARG_OUTPUT_NAME)
        set(SHADER_NAME ${ARG_OUTPUT_NAME})
    else()
        get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)
    endif()

    set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}_${STAGE}.spv)
    set(COMMON_SHADER_INCLUDES ${CMAKE_SOURCE_DIR}/shaders/common.slang)

    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${SLANGC} ${SHADER_SOURCE} -target spirv -entry ${ENTRY} -stage ${STAGE} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_SOURCE} ${COMMON_SHADER_INCLUDES}
            COMMENT "Compiling ${STAGE} shader from ${SHADER_SOURCE} -> ${SHADER_NAME}_${STAGE}.spv"
    )

    target_sources(${TARGET} PRIVATE ${SHADER_OUTPUT})
endfunction()


function(compile_shaders TARGET)
    set(ARGS ${ARGN})
    list(LENGTH ARGS NUM_ARGS)

    math(EXPR NUM_TRIPLETS "${NUM_ARGS} / 3")

    foreach (i RANGE 0 ${NUM_TRIPLETS})
        math(EXPR SOURCE_IDX "${i} * 3")
        math(EXPR STAGE_IDX "${i} * 3 + 1")
        math(EXPR ENTRY_IDX "${i} * 3 + 2")

        if (SOURCE_IDX LESS NUM_ARGS)
            list(GET ARGS ${SOURCE_IDX} SHADER_SOURCE)
            list(GET ARGS ${STAGE_IDX} STAGE)
            list(GET ARGS ${ENTRY_IDX} ENTRY)
            compile_shader(${TARGET} ${SHADER_SOURCE} ${STAGE} ${ENTRY})
        endif ()
    endforeach ()
endfunction()
# ====================================================
# ===== The Important Bits ===========================
add_executable(TaskMeshRendering
        main.cpp
        src/task_mesh_rendering.cpp
        src/task_mesh_rendering.h
        src/vk_context.cpp
        src/vk_context.h
        src/vk_swapchain.cpp
        src/vk_swapchain.h
        src/vk_synchronization.cpp
        src/vk_synchronization.h
        src/vk_imgui_wrapper.cpp
        src/vk_imgui_wrapper.h
        src/vk_render_targets.cpp
        src/vk_render_targets.h
        src/vk_resources.cpp
        src/vk_resources.h
        src/vk_pipelines.cpp
        src/vk_pipelines.h
        src/vk_utils.cpp
        src/vk_utils.h
        src/vk_helpers.cpp
        src/vk_helpers.h
        src/vk_types.cpp
        src/vk_types.h
        src/input.cpp
        src/input.h
        src/time.cpp
        src/time.h
        src/pipelines/mesh_only_pipeline.cpp
        src/pipelines/mesh_only_pipeline.h
        src/pipelines/task_mesh_pipeline.cpp
        src/pipelines/task_mesh_pipeline.h
        src/pipelines/task_mesh_indirect_pipeline.cpp
        src/pipelines/task_mesh_indirect_pipeline.h
        src/pipelines/task_mesh_sample_pipeline.cpp
        src/pipelines/task_mesh_sample_pipeline.h
)

target_include_directories(TaskMeshRendering PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/extern
        ${CMAKE_SOURCE_DIR}/fmt/include
)

target_link_libraries(TaskMeshRendering PUBLIC
        fmt::fmt
        volk
        imgui
        VulkanMemoryAllocator
        vk-bootstrap::vk-bootstrap
        SDL3::SDL3
        fastgltf
        offsetAllocator
        meshoptimizer
)

compile_shaders(TaskMeshRendering
        ${CMAKE_SOURCE_DIR}/shaders/meshOnly.slang mesh meshMain
        ${CMAKE_SOURCE_DIR}/shaders/meshOnly.slang fragment fragmentMain
)

add_custom_command(TARGET TaskMeshRendering POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:TaskMeshRendering>
)
