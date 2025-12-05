//
// Created by William on 2025-12-05.
//

#ifndef TASKMESHRENDERING_TASK_MESH_SAMPLE_PIPELINE_H
#define TASKMESHRENDERING_TASK_MESH_SAMPLE_PIPELINE_H


#include <volk/volk.h>
#include <glm/glm.hpp>

#include "src/vk_resources.h"

struct VulkanContext;

struct TaskMeshSamplePipelinePushConstant
{
    glm::mat4 modelMatrix;
    VkDeviceAddress sceneData;
};


class TaskMeshSamplePipeline
{
public:
    TaskMeshSamplePipeline();

    ~TaskMeshSamplePipeline();

    explicit TaskMeshSamplePipeline(VulkanContext* context);

    TaskMeshSamplePipeline(const TaskMeshSamplePipeline&) = delete;

    TaskMeshSamplePipeline& operator=(const TaskMeshSamplePipeline&) = delete;

    TaskMeshSamplePipeline(TaskMeshSamplePipeline&& other) noexcept;

    TaskMeshSamplePipeline& operator=(TaskMeshSamplePipeline&& other) noexcept;

public:
    PipelineLayout pipelineLayout;
    Pipeline pipeline;

private:
    VulkanContext* context{nullptr};
};




#endif //TASKMESHRENDERING_TASK_MESH_SAMPLE_PIPELINE_H