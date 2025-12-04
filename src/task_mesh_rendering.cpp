//
// Created by William on 2025-12-04.
//

#include "task_mesh_rendering.h"

#include <volk/volk.h>
#include <fmt/format.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "input.h"
#include "time.h"
#include "vk_context.h"
#include "vk_helpers.h"
#include "vk_imgui_wrapper.h"
#include "vk_render_targets.h"
#include "vk_swapchain.h"
#include "vk_types.h"
#include "vk_utils.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"

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
    swapchain = std::make_unique<Swapchain>(context.get(), w, h);
    assert(swapchain->imageCount == 3);
    renderTargets = std::make_unique<RenderTargets>(context.get(), w, h);
    imgui = std::make_unique<ImguiWrapper>(context.get(), window, 3, DRAW_IMAGE_FORMAT);

    VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.pNext = nullptr;
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    bufferInfo.usage = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.size = sizeof(SceneData);

    for (int32_t i = 0; i < 3; ++i) {
        frameSynchronization[i] = FrameSynchronization(context.get());
        frameSynchronization[i].Initialize();
        sceneDataBuffers[i] = std::move(VkResources::CreateAllocatedBuffer(context.get(), bufferInfo, vmaAllocInfo));
    }


    // Immediate
    const VkFenceCreateInfo fenceInfo = VkHelpers::FenceCreateInfo();
    VK_CHECK(vkCreateFence(context->device, &fenceInfo, nullptr, &immFence));
    const VkCommandPoolCreateInfo poolInfo = VkHelpers::CommandPoolCreateInfo(context->graphicsQueueFamily);
    VK_CHECK(vkCreateCommandPool(context->device, &poolInfo, nullptr, &immCommandPool));
    const VkCommandBufferAllocateInfo allocInfo = VkHelpers::CommandBufferAllocateInfo(1, immCommandPool);
    VK_CHECK(vkAllocateCommandBuffers(context->device, &allocInfo, &immCommandBuffer));
    stagingBuffer = VkResources::CreateAllocatedStagingBuffer(context.get(), 1024 * 32);
}

void TaskMeshRendering::Run()
{
    Input& input = Input::Get();
    Time& time = Time::Get();

    SDL_Event e;
    bool exit = false;
    while (true) {
        input.FrameReset();
        while (SDL_PollEvent(&e) != 0) {
            input.ProcessEvent(e);
            imgui->HandleInput(e);
            if (e.type == SDL_EVENT_QUIT) { exit = true; }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) { exit = true; }
        }

        if (exit) {
            bShouldExit = true;
            break;
        }

        input.UpdateFocus(SDL_GetWindowFlags(window));
        time.Update();

        const float deltaTime = Time::Get().GetDeltaTime();
        // freeCamera.Update(deltaTime);

        const uint32_t currentFrameInFlight = frameNumber % swapchain->imageCount;

        FrameSynchronization& currentFrameSync = frameSynchronization[currentFrameInFlight];
        ImDrawDataSnapshot& currentImguiFrameBuffer = imguiFrameBuffers[currentFrameInFlight];

        DrawImgui();
        currentImguiFrameBuffer.SnapUsingSwap(ImGui::GetDrawData(), ImGui::GetTime());

        Render(deltaTime, currentFrameInFlight, currentFrameSync, currentImguiFrameBuffer);
        frameNumber++;
    }
}

void TaskMeshRendering::DrawImgui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Main")) {
        ImGui::Text("Hello!");
    }

    ImGui::End();
    ImGui::Render();
}

void TaskMeshRendering::Render(float dt, uint32_t currentFrameInFlight, const FrameSynchronization& frameSync, ImDrawDataSnapshot& imDrawDataSnapshot)
{
    VK_CHECK(vkWaitForFences(context->device, 1, &frameSync.renderFence, true, UINT64_MAX));
    VK_CHECK(vkResetFences(context->device, 1, &frameSync.renderFence));

    VkExtent2D extents = swapchain->extent;

    //
    {
        constexpr glm::vec3 cameraPos{0, 0, -4};
        constexpr glm::vec3 cameraLookAt{};
        constexpr glm::vec3 up{0, 1, 0};

        glm::mat4 view = glm::lookAt(cameraPos, cameraLookAt, up);

        glm::mat4 proj = glm::perspective(
            glm::radians(75.0f),
            static_cast<float>(extents.width) / static_cast<float>(extents.height),
            1000.0f,
            0.1f
        );

        sceneData.view = view;
        sceneData.proj = proj;
        sceneData.viewProj = proj * view;
        sceneData.renderTargetSize.x = static_cast<float>(extents.width);
        sceneData.renderTargetSize.y = static_cast<float>(extents.height);
        sceneData.deltaTime = dt;

        AllocatedBuffer& currentSceneDataBuffer = sceneDataBuffers[currentFrameInFlight];
        auto currentSceneData = static_cast<SceneData*>(currentSceneDataBuffer.allocationInfo.pMappedData);
        *currentSceneData = sceneData;
    }

    VkCommandBuffer cmd = frameSync.commandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo commandBufferBeginInfo = VkHelpers::CommandBufferBeginInfo();
    VK_CHECK(vkBeginCommandBuffer(cmd, &commandBufferBeginInfo));

    //
    {
        auto subresource = VkHelpers::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        auto barrier = VkHelpers::ImageMemoryBarrier(
            renderTargets->drawImage.handle,
            subresource,
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, renderTargets->drawImage.layout,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
        renderTargets->drawImage.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        auto dependencyInfo = VkHelpers::DependencyInfo(&barrier);
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    // Imgui Draw
    {
        VkRenderingAttachmentInfo imguiAttachment{};
        imguiAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        imguiAttachment.pNext = nullptr;
        imguiAttachment.imageView = renderTargets->drawImageView.handle;
        imguiAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imguiAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        imguiAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.pNext = nullptr;
        renderInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, swapchain->extent};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &imguiAttachment;
        renderInfo.pDepthAttachment = nullptr;
        renderInfo.pStencilAttachment = nullptr;
        vkCmdBeginRendering(cmd, &renderInfo);
        ImGui_ImplVulkan_RenderDrawData(&imDrawDataSnapshot.DrawData, cmd);

        vkCmdEndRendering(cmd);
    }


    uint32_t swapchainImageIndex;
    VkResult e = vkAcquireNextImageKHR(context->device, swapchain->handle, UINT64_MAX, frameSync.swapchainSemaphore, nullptr, &swapchainImageIndex);
    if (e == VK_ERROR_OUT_OF_DATE_KHR || e == VK_SUBOPTIMAL_KHR) {
        fmt::println("Swapchain out of date or suboptimal (Acquire)");
        exit(1);
    }
    VkImage currentSwapchainImage = swapchain->swapchainImages[swapchainImageIndex];


    // Prepare for blit
    {
        VkImageMemoryBarrier2 barriers[2];
        barriers[0] = VkHelpers::ImageMemoryBarrier(
            renderTargets->drawImage.handle,
            VkHelpers::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT),
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, renderTargets->drawImage.layout,
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        );
        renderTargets->drawImage.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[1] = VkHelpers::ImageMemoryBarrier(
            currentSwapchainImage,
            VkHelpers::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT),
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        VkDependencyInfo depInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        depInfo.imageMemoryBarrierCount = 2;
        depInfo.pImageMemoryBarriers = barriers;
        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    // Blit
    {
        VkOffset3D renderOffset = {static_cast<int32_t>(extents.width), static_cast<int32_t>(extents.height), 1};
        VkOffset3D swapchainOffset = {static_cast<int32_t>(swapchain->extent.width), static_cast<int32_t>(swapchain->extent.height), 1};
        VkImageBlit2 blitRegion{};
        blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.layerCount = 1;
        blitRegion.srcOffsets[0] = {0, 0, 0};
        blitRegion.srcOffsets[1] = renderOffset;
        blitRegion.dstOffsets[0] = {0, 0, 0};
        blitRegion.dstOffsets[1] = swapchainOffset;

        VkBlitImageInfo2 blitInfo{};
        blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
        blitInfo.srcImage = renderTargets->drawImage.handle;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.dstImage = currentSwapchainImage;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &blitRegion;
        blitInfo.filter = VK_FILTER_LINEAR;

        vkCmdBlitImage2(cmd, &blitInfo);
    }

    //
    {
        auto subresource = VkHelpers::SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        auto barrier = VkHelpers::ImageMemoryBarrier(
            currentSwapchainImage,
            subresource,
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );
        auto dependencyInfo = VkHelpers::DependencyInfo(&barrier);
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    VK_CHECK(vkEndCommandBuffer(cmd));


    VkCommandBufferSubmitInfo commandBufferSubmitInfo = VkHelpers::CommandBufferSubmitInfo(frameSync.commandBuffer);
    VkSemaphoreSubmitInfo swapchainSemaphoreWaitInfo = VkHelpers::SemaphoreSubmitInfo(frameSync.swapchainSemaphore, VK_PIPELINE_STAGE_2_BLIT_BIT);
    VkSemaphoreSubmitInfo renderSemaphoreSignalInfo = VkHelpers::SemaphoreSubmitInfo(frameSync.renderSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    VkSubmitInfo2 submitInfo = VkHelpers::SubmitInfo(&commandBufferSubmitInfo, &swapchainSemaphoreWaitInfo, &renderSemaphoreSignalInfo);
    VK_CHECK(vkQueueSubmit2(context->graphicsQueue, 1, &submitInfo, frameSync.renderFence));

    VkPresentInfoKHR presentInfo = VkHelpers::PresentInfo(&swapchain->handle, nullptr, &swapchainImageIndex);
    presentInfo.pWaitSemaphores = &frameSync.renderSemaphore;
    const VkResult presentResult = vkQueuePresentKHR(context->graphicsQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        fmt::println("Swapchain out of date or suboptimal (Present)\n");
        exit(1);
    }
}

void TaskMeshRendering::Cleanup()
{
    vkDeviceWaitIdle(context->device);

    vkDestroyCommandPool(context->device, immCommandPool, nullptr);
    vkDestroyFence(context->device, immFence, nullptr);

    SDL_DestroyWindow(window);
}

void TaskMeshRendering::LoadMeshletModel() {}
