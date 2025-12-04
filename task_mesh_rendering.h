//
// Created by William on 2025-12-04.
//

#ifndef TASKMESHRENDERING_TASK_MESH_RENDERING_H
#define TASKMESHRENDERING_TASK_MESH_RENDERING_H
#include <memory>

#include <SDL3/SDL.h>
#include <volk/volk.h>
#include <offsetAllocator.hpp>

#include "vk_resources.h"

class TaskMeshRendering
{
public:
    TaskMeshRendering();
    ~TaskMeshRendering();

    void Initialize();
    void Run();
    void Cleanup();

private:
    SDL_Window* window{nullptr};
    std::unique_ptr<VulkanContext> context;



private: // Immediate to simplify asset upload
    VkFence immFence{VK_NULL_HANDLE};
    VkCommandPool immCommandPool{VK_NULL_HANDLE};
    VkCommandBuffer immCommandBuffer{VK_NULL_HANDLE};
    AllocatedBuffer imageStagingBuffer{};
    OffsetAllocator::Allocator stagingAllocator{1024 * 32};
};


#endif //TASKMESHRENDERING_TASK_MESH_RENDERING_H