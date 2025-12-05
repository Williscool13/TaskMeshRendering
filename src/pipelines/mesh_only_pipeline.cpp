//
// Created by William on 2025-12-05.
//

#include "mesh_only_pipeline.h"

#include <fmt/format.h>

#include "src/vk_helpers.h"
#include "src/vk_pipelines.h"
#include "src/vk_render_targets.h"

MeshOnlyPipeline::MeshOnlyPipeline() = default;

MeshOnlyPipeline::~MeshOnlyPipeline() = default;

MeshOnlyPipeline::MeshOnlyPipeline(VulkanContext* context) : context(context)
{
    VkPushConstantRange renderPushConstantRange{};
    renderPushConstantRange.offset = 0;
    renderPushConstantRange.size = sizeof(MeshOnlyPipelinePushConstant);
    renderPushConstantRange.stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = &renderPushConstantRange;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

    pipelineLayout = VkResources::CreatePipelineLayout(context, pipelineLayoutCreateInfo);

    VkShaderModule meshShader;
    VkShaderModule fragShader;
    if (!VkHelpers::LoadShaderModule("shaders\\meshOnly_mesh.spv", context->device, &meshShader)) {
        fmt::println("Couldn't create shader module (meshOnly_mesh)");
        exit(1);
    }
    if (!VkHelpers::LoadShaderModule("shaders\\meshOnly_fragment.spv", context->device, &fragShader)) {
        fmt::println("Couldn't create shader module (meshOnly_fragment)");
        exit(1);
    }

    RenderPipelineBuilder pipelineBuilder;

    pipelineBuilder.SetMeshShader(meshShader, fragShader);
    pipelineBuilder.SetupInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.SetupRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.DisableMultisampling();
    pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineBuilder.SetupRenderer({DRAW_IMAGE_FORMAT}, DEPTH_IMAGE_FORMAT);
    pipelineBuilder.SetupPipelineLayout(pipelineLayout.handle);
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = pipelineBuilder.GeneratePipelineCreateInfo();
    pipeline = VkResources::CreateGraphicsPipeline(context, pipelineCreateInfo);

    vkDestroyShaderModule(context->device, meshShader, nullptr);
    vkDestroyShaderModule(context->device, fragShader, nullptr);
}

MeshOnlyPipeline::MeshOnlyPipeline(MeshOnlyPipeline&& other) noexcept
{
    pipelineLayout = std::move(other.pipelineLayout);
    pipeline = std::move(other.pipeline);
    context = other.context;
    other.context = nullptr;
}

MeshOnlyPipeline& MeshOnlyPipeline::operator=(MeshOnlyPipeline&& other) noexcept
{
    if (this != &other) {
        pipelineLayout = std::move(other.pipelineLayout);
        pipeline = std::move(other.pipeline);
        context = other.context;
        other.context = nullptr;
    }
    return *this;
}
