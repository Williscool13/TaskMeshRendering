//
// Created by William on 2025-12-05.
//

#include "task_mesh_pipeline.h"

#include <fmt/format.h>

#include "src/vk_helpers.h"
#include "src/vk_pipelines.h"
#include "src/vk_render_targets.h"

TaskMeshPipeline::TaskMeshPipeline() = default;

TaskMeshPipeline::~TaskMeshPipeline() = default;

TaskMeshPipeline::TaskMeshPipeline(VulkanContext* context) : context(context)
{
    VkPushConstantRange renderPushConstantRange{};
    renderPushConstantRange.offset = 0;
    renderPushConstantRange.size = sizeof(TaskMeshPipelinePushConstant);
    renderPushConstantRange.stageFlags = VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = &renderPushConstantRange;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

    pipelineLayout = VkResources::CreatePipelineLayout(context, pipelineLayoutCreateInfo);

    VkShaderModule taskShader;
    VkShaderModule meshShader;
    VkShaderModule fragShader;
    if (!VkHelpers::LoadShaderModule("shaders\\taskMesh_task.spv", context->device, &taskShader)) {
        fmt::println("Couldn't create shader module (taskMesh_task)");
        exit(1);
    }
    if (!VkHelpers::LoadShaderModule("shaders\\taskMesh_mesh.spv", context->device, &meshShader)) {
        fmt::println("Couldn't create shader module (taskMesh_mesh)");
        exit(1);
    }
    if (!VkHelpers::LoadShaderModule("shaders\\taskMesh_fragment.spv", context->device, &fragShader)) {
        fmt::println("Couldn't create shader module (taskMesh_fragment)");
        exit(1);
    }

    RenderPipelineBuilder pipelineBuilder;
    pipelineBuilder.SetTaskMeshShaders(taskShader, meshShader, fragShader);
    pipelineBuilder.SetupInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.SetupRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.DisableMultisampling();
    pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineBuilder.SetupRenderer({DRAW_IMAGE_FORMAT}, DEPTH_IMAGE_FORMAT);
    pipelineBuilder.SetupPipelineLayout(pipelineLayout.handle);
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = pipelineBuilder.GeneratePipelineCreateInfo();
    pipeline = VkResources::CreateGraphicsPipeline(context, pipelineCreateInfo);

    vkDestroyShaderModule(context->device, taskShader, nullptr);
    vkDestroyShaderModule(context->device, meshShader, nullptr);
    vkDestroyShaderModule(context->device, fragShader, nullptr);
}

TaskMeshPipeline::TaskMeshPipeline(TaskMeshPipeline&& other) noexcept
{
    pipelineLayout = std::move(other.pipelineLayout);
    pipeline = std::move(other.pipeline);
    context = other.context;
    other.context = nullptr;
}

TaskMeshPipeline& TaskMeshPipeline::operator=(TaskMeshPipeline&& other) noexcept
{
    if (this != &other) {
        pipelineLayout = std::move(other.pipelineLayout);
        pipeline = std::move(other.pipeline);
        context = other.context;
        other.context = nullptr;
    }
    return *this;
}
