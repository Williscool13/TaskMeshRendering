//
// Created by William on 2025-12-04.
//

#include "task_mesh_rendering.h"

#include <volk/volk.h>
#include <fmt/format.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include "input.h"
#include "meshoptimizer.h"
#include "time.h"
#include "vk_context.h"
#include "vk_helpers.h"
#include "vk_imgui_wrapper.h"
#include "vk_render_targets.h"
#include "vk_swapchain.h"
#include "vk_types.h"
#include "vk_utils.h"

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

    stanfordBunny = LoadStanfordBunny();
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

ExtractedMeshletModel TaskMeshRendering::LoadStanfordBunny()
{
    ExtractedMeshletModel meshletModel{};
    fastgltf::Parser parser{fastgltf::Extensions::KHR_texture_basisu | fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_texture_transform};
    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember
                                 | fastgltf::Options::AllowDouble
                                 | fastgltf::Options::LoadExternalBuffers
                                 | fastgltf::Options::LoadExternalImages;

    const std::filesystem::path path = "asset/stanford_bunny/stanford_bunny.gltf";
    auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
    if (!static_cast<bool>(gltfFile)) {
        fmt::println("[Error] Failed to open glTF file ({}): {}\n", path.filename().string(), getErrorMessage(gltfFile.error()));
        exit(1);
    }

    auto load = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
    if (!load) {
        fmt::println("[Error] Failed to load glTF: {}\n", to_underlying(load.error()));
        exit(1);
    }

    fastgltf::Asset gltf = std::move(load.get());
    assert(gltf.meshes.size() == 1);
    assert(gltf.meshes[0].primitives.size() == 1);
    assert(gltf.nodes.size() == 1);

    meshletModel.bSuccessfullyLoaded = true;
    meshletModel.name = path.filename().string();

    fastgltf::Primitive& bunnyPrimitive = gltf.meshes[0].primitives[0];
    meshletModel.primitive.materialIndex = 0;


    std::vector<Vertex> primitiveVertices{};
    std::vector<uint32_t> primitiveIndices{};

    // INDICES
    const fastgltf::Accessor& indexAccessor = gltf.accessors[bunnyPrimitive.indicesAccessor.value()];
    primitiveIndices.clear();
    primitiveIndices.reserve(indexAccessor.count);

    fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor, [&](const std::uint32_t idx) {
        primitiveIndices.push_back(idx);
    });

    // POSITION (REQUIRED)
    const fastgltf::Attribute* positionIt = bunnyPrimitive.findAttribute("POSITION");
    const fastgltf::Accessor& posAccessor = gltf.accessors[positionIt->accessorIndex];
    primitiveVertices.clear();
    primitiveVertices.resize(posAccessor.count);

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(gltf, posAccessor, [&](fastgltf::math::fvec3 v, const size_t index) {
        primitiveVertices[index] = {};
        primitiveVertices[index].position = {v.x(), v.y(), v.z()};
    });

    const size_t maxVertices = 64;
    const size_t maxTriangles = 64;

    // build clusters (meshlets) out of the mesh
    size_t max_meshlets = meshopt_buildMeshletsBound(primitiveIndices.size(), maxVertices, maxTriangles);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    std::vector<unsigned int> meshletVertices(primitiveIndices.size());
    std::vector<unsigned char> meshletTriangles(primitiveIndices.size());

    std::vector<uint32_t> primitiveVertexPositions;
    meshlets.resize(meshopt_buildMeshlets(&meshlets[0], &meshletVertices[0], &meshletTriangles[0],
                                          primitiveIndices.data(), primitiveIndices.size(),
                                          reinterpret_cast<const float*>(primitiveVertices.data()), primitiveVertices.size(), sizeof(Vertex),
                                          maxVertices, maxTriangles, 0.f));

    // Optimize each meshlet's micro index buffer/vertex layout individually
    for (auto& meshlet : meshlets) {
        meshopt_optimizeMeshlet(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, meshlet.vertex_count);
    }

    // Trim the meshlet data to minimize waste for meshletVertices/meshletTriangles
    {
        // this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
        const meshopt_Meshlet& last = meshlets.back();
        meshletVertices.resize(last.vertex_offset + last.vertex_count);
        meshletTriangles.resize(last.triangle_offset + last.triangle_count * 3);
    }

    auto generateBoundingSphere = [](const std::vector<Vertex>& vertices) {
        glm::vec3 center = {0, 0, 0};

        for (auto&& vertex : vertices) {
            center += vertex.position;
        }
        center /= static_cast<float>(vertices.size());


        float radius = glm::dot(vertices[0].position - center, vertices[0].position - center);
        for (size_t i = 1; i < vertices.size(); ++i) {
            radius = std::max(radius, glm::dot(vertices[i].position - center, vertices[i].position - center));
        }
        radius = std::nextafter(sqrtf(radius), std::numeric_limits<float>::max());

        return glm::vec4(center, radius);
    };


    meshletModel.primitive.meshletOffset = meshletModel.meshlets.size();
    meshletModel.primitive.meshletCount = meshlets.size();
    meshletModel.primitive.boundingSphere = generateBoundingSphere(primitiveVertices);

    uint32_t vertexOffset = meshletModel.vertices.size();
    uint32_t meshletVertexOffset = meshletModel.meshletVertices.size();
    uint32_t meshletTrianglesOffset = meshletModel.meshletTriangles.size();

    meshletModel.vertices.insert(meshletModel.vertices.end(), primitiveVertices.begin(), primitiveVertices.end());
    meshletModel.meshletVertices.insert(meshletModel.meshletVertices.end(), meshletVertices.begin(), meshletVertices.end());
    meshletModel.meshletTriangles.insert(meshletModel.meshletTriangles.end(), meshletTriangles.begin(), meshletTriangles.end());

    for (meshopt_Meshlet& meshlet : meshlets) {
        meshopt_Bounds bounds = meshopt_computeMeshletBounds(
            &meshletVertices[meshlet.vertex_offset],
            &meshletTriangles[meshlet.triangle_offset],
            meshlet.triangle_count,
            reinterpret_cast<const float*>(primitiveVertices.data()),
            primitiveVertices.size(),
            sizeof(Vertex)
        );

        meshletModel.meshlets.push_back({
            .meshletBoundingSphere = glm::vec4(
                bounds.center[0], bounds.center[1], bounds.center[2],
                bounds.radius
            ),
            .coneApex = glm::vec3(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2]),
            .coneCutoff = bounds.cone_cutoff,

            .coneAxis = glm::vec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]),
            .vertexOffset = vertexOffset,

            .meshletVerticesOffset = meshletVertexOffset + meshlet.vertex_offset,
            .meshletTriangleOffset = meshletTrianglesOffset + meshlet.triangle_offset,
            .meshletVerticesCount = meshlet.vertex_count,
            .meshletTriangleCount = meshlet.triangle_count,
        });
    }


    fastgltf::Node& node = gltf.nodes[0];
    glm::vec3 localTranslation{};
    glm::quat localRotation{};
    glm::vec3 localScale{};
    std::visit(
        fastgltf::visitor{
            [&](fastgltf::math::fmat4x4 matrix) {
                glm::mat4 glmMatrix;
                for (int i = 0; i < 4; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        glmMatrix[i][j] = matrix[i][j];
                    }
                }

                localTranslation = glm::vec3(glmMatrix[3]);
                localRotation = glm::quat_cast(glmMatrix);
                localScale = glm::vec3(
                    glm::length(glm::vec3(glmMatrix[0])),
                    glm::length(glm::vec3(glmMatrix[1])),
                    glm::length(glm::vec3(glmMatrix[2]))
                );
            },
            [&](fastgltf::TRS transform) {
                localTranslation = {transform.translation[0], transform.translation[1], transform.translation[2]};
                localRotation = {transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]};
                localScale = {transform.scale[0], transform.scale[1], transform.scale[2]};
            }
        }
        , node.transform
    );

    meshletModel.transform = {localTranslation, localRotation, localScale};

    return meshletModel;
}
