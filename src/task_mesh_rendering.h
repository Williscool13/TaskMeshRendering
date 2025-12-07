//
// Created by William on 2025-12-04.
//

#ifndef TASKMESHRENDERING_TASK_MESH_RENDERING_H
#define TASKMESHRENDERING_TASK_MESH_RENDERING_H
#include <array>
#include <memory>

#include <SDL3/SDL.h>
#include <volk/volk.h>
#include <offsetAllocator.hpp>
#include <vector>

#include "imgui_threaded_rendering.h"
#include "vk_resources.h"
#include "vk_synchronization.h"
#include "vk_types.h"
#include "camera/free_camera.h"
#include "pipelines/mesh_only_pipeline.h"
#include "pipelines/task_mesh_pipeline.h"
#include "pipelines/task_mesh_sample_pipeline.h"

class TaskMeshRendering
{
public:
    TaskMeshRendering();

    ~TaskMeshRendering();

    void Initialize();

    void Run();

    void GenerateImgui();

    void DrawTaskMeshSample(VkExtent2D extents, AllocatedBuffer& currentSceneDataBuffer, VkCommandBuffer cmd) const;

    void DrawMeshOnly(VkExtent2D extents, AllocatedBuffer& currentSceneDataBuffer, VkCommandBuffer cmd) const;

    void DrawTaskMesh(VkExtent2D extents, AllocatedBuffer& currentSceneDataBuffer, AllocatedBuffer& currentCulledDataBuffer, VkCommandBuffer cmd) const;

    void Render(float dt, uint32_t currentFrameInFlight, const FrameSynchronization& frameSync, ImDrawDataSnapshot& imDrawDataSnapshot);

    void Cleanup();

    static ExtractedMeshletModel LoadStanfordBunny();

    enum class RenderMode
    {
        Basic,
        MeshOnly,
        TaskMesh
    };

    RenderMode currentRenderMode = RenderMode::TaskMesh;

private:
    SDL_Window* window{nullptr};
    std::unique_ptr<struct VulkanContext> context;
    std::unique_ptr<struct Swapchain> swapchain{};
    std::unique_ptr<struct ImguiWrapper> imgui{};
    std::unique_ptr<struct RenderTargets> renderTargets{};

    std::array<FrameSynchronization, 3> frameSynchronization;
    std::array<AllocatedBuffer, 3> sceneDataBuffers;
    std::array<AllocatedBuffer, 3> culledReadbackBuffers;
    std::array<ImDrawDataSnapshot, 3> imguiFrameBuffers{};

    bool bShouldExit{false};
    uint64_t frameNumber{0};
    SceneData sceneData{};
    bool frozen = false;
    SceneData frozenSceneData{};
    FreeCamera freeCamera{{2.0f, 0.5f, 3.0f}, {0.0f, 0.0f, 0.0f}};

    AllocatedBuffer vertexBuffer;
    AllocatedBuffer meshletVerticesBuffer;
    AllocatedBuffer meshletTrianglesBuffer;
    AllocatedBuffer meshletBuffer;
    AllocatedBuffer primitiveBuffer;

    ExtractedMeshletModel stanfordBunny;

    MeshOnlyPipeline meshOnlyPipeline;
    TaskMeshSamplePipeline taskMeshSamplePipeline;
    TaskMeshPipeline taskMeshPipeline;
    CulledData culledData;

private: // Immediate to simplify asset upload
    VkFence immFence{VK_NULL_HANDLE};
    VkCommandPool immCommandPool{VK_NULL_HANDLE};
    VkCommandBuffer immCommandBuffer{VK_NULL_HANDLE};
    AllocatedBuffer stagingBuffer{};
    OffsetAllocator::Allocator stagingAllocator{1024 * 32};
};


#endif //TASKMESHRENDERING_TASK_MESH_RENDERING_H
