//
// Created by William on 2025-12-04.
//

#include "task_mesh_rendering.h"

#include <volk/volk.h>
#include <fmt/format.h>

#include "vk_context.h"

TaskMeshRendering::TaskMeshRendering() = default;

TaskMeshRendering::~TaskMeshRendering() = default;

void TaskMeshRendering::Initialize()
{
    bool sdlInitSuccess = SDL_Init(SDL_INIT_VIDEO);
    if (!sdlInitSuccess) {
        fmt::println("[Error] SDL_Init failed: {}", SDL_GetError());
        exit(1);
    }

    constexpr auto window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;

    window = SDL_CreateWindow(
        "Template",
        800,
        600,
        window_flags);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);
    int32_t w;
    int32_t h;
    SDL_GetWindowSize(window, &w, &h);
    // Input::Get().Init(window, w, h);
    context = std::make_unique<VulkanContext>(window);
}
void TaskMeshRendering::Run() {}
void TaskMeshRendering::Cleanup()
{
    vkDeviceWaitIdle(context->device);

    vkDestroyCommandPool(context->device, immCommandPool, nullptr);
    vkDestroyFence(context->device, immFence, nullptr);

    SDL_DestroyWindow(window);
}
