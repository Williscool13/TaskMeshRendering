//
// Created by William on 2025-10-09.
//

#ifndef WILLENGINETESTBED_IMGUI_H
#define WILLENGINETESTBED_IMGUI_H

#include <volk.h>
#include <SDL3/SDL.h>

struct ImguiWrapper
{
public:
    ImguiWrapper() = default;

    ImguiWrapper(struct VulkanContext* context, SDL_Window* window, int32_t swapchainImageCount, VkFormat renderAttachmentFormat);

    ~ImguiWrapper();

    static void HandleInput(const SDL_Event& e);

private:
    VulkanContext* context{nullptr};
    VkDescriptorPool imguiPool{VK_NULL_HANDLE};
};



#endif //WILLENGINETESTBED_IMGUI_H
