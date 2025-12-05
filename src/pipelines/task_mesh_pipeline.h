//
// Created by William on 2025-12-05.
//

#ifndef TASKMESHRENDERING_TASK_MESH_PIPELINE_H
#define TASKMESHRENDERING_TASK_MESH_PIPELINE_H

#include <volk/volk.h>
#include <glm/glm.hpp>

#include "src/vk_resources.h"

struct VulkanContext;

struct TaskMeshPipelinePushConstant
{
    glm::mat4 modelMatrix;
    VkDeviceAddress sceneData;
    VkDeviceAddress vertexBuffer;
    VkDeviceAddress meshletVerticesBuffer;
    VkDeviceAddress meshletTrianglesBuffer;
    VkDeviceAddress meshletBuffer;
    uint32_t meshletCount;
};


class TaskMeshPipeline
{
public:
    TaskMeshPipeline();

    ~TaskMeshPipeline();

    explicit TaskMeshPipeline(VulkanContext* context);

    TaskMeshPipeline(const TaskMeshPipeline&) = delete;

    TaskMeshPipeline& operator=(const TaskMeshPipeline&) = delete;

    TaskMeshPipeline(TaskMeshPipeline&& other) noexcept;

    TaskMeshPipeline& operator=(TaskMeshPipeline&& other) noexcept;

public:
    PipelineLayout pipelineLayout;
    Pipeline pipeline;

private:
    VulkanContext* context{nullptr};
};


#endif //TASKMESHRENDERING_TASK_MESH_PIPELINE_H