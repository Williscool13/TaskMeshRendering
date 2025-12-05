//
// Created by William on 2025-12-05.
//

#ifndef TASKMESHRENDERING_MESH_ONLY_PIPELINE_H
#define TASKMESHRENDERING_MESH_ONLY_PIPELINE_H

#include <volk/volk.h>
#include <glm/glm.hpp>

#include "src/vk_resources.h"

struct VulkanContext;

struct MeshOnlyPipelinePushConstant
{
    glm::mat4 modelMatrix;
    VkDeviceAddress sceneData;
    VkDeviceAddress vertexBuffer;
    VkDeviceAddress meshletVerticesBuffer;
    VkDeviceAddress meshletTrianglesBuffer;
    VkDeviceAddress meshletBuffer;
};


class MeshOnlyPipeline
{
public:
    MeshOnlyPipeline();

    ~MeshOnlyPipeline();

    explicit MeshOnlyPipeline(VulkanContext* context);

    MeshOnlyPipeline(const MeshOnlyPipeline&) = delete;

    MeshOnlyPipeline& operator=(const MeshOnlyPipeline&) = delete;

    MeshOnlyPipeline(MeshOnlyPipeline&& other) noexcept;

    MeshOnlyPipeline& operator=(MeshOnlyPipeline&& other) noexcept;

public:
    PipelineLayout pipelineLayout;
    Pipeline pipeline;

private:
    VulkanContext* context{nullptr};
};




#endif //TASKMESHRENDERING_MESH_ONLY_PIPELINE_H