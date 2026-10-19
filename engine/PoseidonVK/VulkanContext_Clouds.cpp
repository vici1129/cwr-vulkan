#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/Shaders/gamma.vert.hpp>
#include <PoseidonVK/Shaders/clouds.frag.hpp>
#include <PoseidonVK/Shaders/clouds_composite.frag.hpp>
#include <PoseidonVK/Shaders/clouds_composite_temporal.frag.hpp>
#include <stdexcept>

namespace Poseidon::vk
{
namespace
{
void Require(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string("Vulkan volumetric clouds: ") + operation + " failed (" +
                                 std::to_string(result) + ")");
}
} // namespace
std::string VulkanContext::SetVolumetricClouds(std::string_view mode)
{
    if (mode == "status")
        return std::string("requested=") + (_cloudsRequested ? "on" : "off") +
               " active=" + (_weatherLighting.options[3] > 0 && _cloudsDrew ? "on" : "off") +
               " half=" + std::to_string(_cloudsExtent.width) + "x" + std::to_string(_cloudsExtent.height) +
               " layers=3 steps=32x3 lightSteps=2 history=world-AA position=" +
               std::to_string(_weatherLighting.options[2]) + " fallback=" + _cloudsFailure + _weatherFailure;
    if (mode != "on" && mode != "off")
        return "INVALID: triVolumetricClouds on, off, status";
    if (_cloudsRequested != (mode == "on"))
    {
        _cloudsRequested = mode == "on";
        _cloudsFailure.clear();
        _recreate = true;
        ResetTemporalHistory();
    }
    return "OK: volumetric clouds " + std::string(mode) + " (next frame)";
}

void VulkanContext::CreateCloudResources()
{
    _cloudsEnabled = _cloudsRequested;
    if (!_cloudsEnabled)
        return;
    _cloudsFailure.clear();
    _cloudsExtent = {std::max(1u, (_worldExtent.width + 1) / 2), std::max(1u, (_worldExtent.height + 1) / 2)};
    const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(_images.size())};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = size.descriptorCount;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    Require(vkCreateDescriptorPool(_device, &pool, nullptr, &_cloudsPool), "create pool");
    _cloudsTargets.resize(_images.size());
    for (auto& target : _cloudsTargets)
    {
        CreateAAImage(target.color, SceneFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, _cloudsExtent);
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = _aaPass;
        fb.attachmentCount = 1;
        fb.pAttachments = &target.color.view;
        fb.width = _cloudsExtent.width;
        fb.height = _cloudsExtent.height;
        fb.layers = 1;
        Require(vkCreateFramebuffer(_device, &fb, nullptr, &target.framebuffer), "create framebuffer");
        VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = _cloudsPool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &_textureLayout;
        Require(vkAllocateDescriptorSets(_device, &alloc, &target.set), "allocate descriptor");
        const VkDescriptorImageInfo image{_aaSampler, target.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = target.set;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image;
        vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
    }
    CreateWeatherLayout();
    const VkDescriptorSetLayout sets[]{_textureLayout, _weatherLayout};
    const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 96};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 2;
    layout.pSetLayouts = sets;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    Require(vkCreatePipelineLayout(_device, &layout, nullptr, &_cloudsLayout), "create layout");
    VkShaderModule vertex{}, fragment{};
    try
    {
        VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        module.codeSize = sizeof(Cwrgamma_vert);
        module.pCode = Cwrgamma_vert;
        Require(vkCreateShaderModule(_device, &module, nullptr, &vertex), "create vertex shader");

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[0].module = vertex;
        stages[1].module = fragment;
        stages[0].pName = stages[1].pName = "main";
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState output{};
        output.colorWriteMask = 15;
        output.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        output.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        output.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        output.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &output;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = states;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline.stageCount = 2;
        pipeline.pStages = stages;
        pipeline.pVertexInputState = &input;
        pipeline.pInputAssemblyState = &assembly;
        pipeline.pViewportState = &viewport;
        pipeline.pRasterizationState = &raster;
        pipeline.pMultisampleState = &samples;
        pipeline.pColorBlendState = &blend;
        pipeline.pDepthStencilState = &depth;
        pipeline.pDynamicState = &dynamic;
        pipeline.layout = _cloudsLayout;
        for (int i = 0; i < 2; ++i)
        {
            const bool temporal = IsTemporal(_aaMode);
            module.codeSize =
                i ? (temporal ? sizeof(Cwrclouds_composite_temporal_frag) : sizeof(Cwrclouds_composite_frag))
                  : sizeof(Cwrclouds_frag);
            module.pCode =
                i ? (temporal ? Cwrclouds_composite_temporal_frag : Cwrclouds_composite_frag) : Cwrclouds_frag;
            Require(vkCreateShaderModule(_device, &module, nullptr, &fragment), "create fragment shader");
            stages[1].module = fragment;
            output.blendEnable = i ? VK_TRUE : VK_FALSE;
            VkPipelineColorBlendAttachmentState outputs[]{output, {}, {}, {}, {}};
            // Clouds replace sky color only. Invalidate temporal correspondence:
            // wind and volume parallax cannot reuse the stock skydome's motion.
            if (i && temporal)
                outputs[1].colorWriteMask = 15;
            blend.attachmentCount = i ? (_rtaoTargets ? 5 : _rtWaterTargets ? 4 : _rtShadowTargets ? 3 : temporal ? 2 : 1) : 1;
            blend.pAttachments = outputs;
            // Match the actual ambient carrier, including GI without RTAO and
            // a GI fallback before target recreation. Preserve captured light.
            VkPipelineColorBlendAttachmentState ambientOutputs[]{output, {}, {}, {}, {}};
            if(i && _rtaoTargets)
            {
                ambientOutputs[1].colorWriteMask = temporal ? 15 : 0;
                blend.attachmentCount = 5;
                blend.pAttachments = ambientOutputs;
            }
            samples.rasterizationSamples = i ? _worldSamples : VK_SAMPLE_COUNT_1_BIT;
            pipeline.renderPass = i ? _worldResume : _aaPass;
            Require(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &_cloudsPipelines[i]),
                    "create pipeline");
            vkDestroyShaderModule(_device, fragment, nullptr);
            fragment = VK_NULL_HANDLE;
        }
    }
    catch (...)
    {
        if (fragment)
            vkDestroyShaderModule(_device, fragment, nullptr);
        if (vertex)
            vkDestroyShaderModule(_device, vertex, nullptr);
        throw;
    }
    vkDestroyShaderModule(_device, fragment, nullptr);
    vkDestroyShaderModule(_device, vertex, nullptr);
}

bool VulkanContext::DrawVolumetricClouds()
{
    _cloudsDrew = false;
    if (!_worldActive || !_worldPassOpen || _weatherLighting.options[3] <= 0 || _cloudsTargets.empty())
        return false;
    FlushScreenBatch(); // finish skydome, stars, sun and moon before the replacement
    const auto command = _frames[_frame].command;
    const auto weather = PrepareWeather();
    auto& frame = _frames[_frame];
    if (frame.frameQueries)
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 16);
    vkCmdEndRenderPass(command);
    for (unsigned i = 0; i < 2; ++i)
    {
        const auto extent = i ? _worldExtent : _cloudsExtent;
        VkClearValue clear{};
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = i ? _worldResume : _aaPass;
        pass.framebuffer = i ? SceneFramebuffer() : _cloudsTargets[_image].framebuffer;
        pass.renderArea.extent = extent;
        pass.clearValueCount = i ? 0 : 1;
        pass.pClearValues = &clear;
        vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _cloudsPipelines[i]);
        if (i)
            vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _cloudsLayout, 0, 1,
                                    &_cloudsTargets[_image].set, 0, nullptr);
        else
            vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _cloudsLayout, 1, 1, &weather, 0,
                                    nullptr);
        const VkViewport viewport{0, 0, float(extent.width), float(extent.height), 0, 1};
        const VkRect2D scissor{{0, 0}, extent};
        vkCmdSetViewport(command, 0, 1, &viewport);
        vkCmdSetScissor(command, 0, 1, &scissor);
        std::array<float, 24> push{};
        std::copy(_waterCamera.begin(), _waterCamera.end(), push.begin());
        std::copy(_waterProjection.begin(), _waterProjection.end(), push.begin() + 16);
        push[20] = IsTemporal(_aaMode) ? _taaJitter[0] : 0;
        push[21] = IsTemporal(_aaMode) ? _taaJitter[1] : 0;
        vkCmdPushConstants(command, _cloudsLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), push.data());
        vkCmdDraw(command, 3, 1, 0, 0);
        if (!i)
            vkCmdEndRenderPass(command);
    }
    // Leave the compatible world LOAD pass open for normal terrain/objects.
    _commands = {};
    if (frame.frameQueries)
    {
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 17);
        frame.cloudsTimestamped = true;
    }
    _cloudsDrew = true;
    return true;
}
void VulkanContext::DestroyCloudResources() noexcept
{
    for (auto& pipeline : _cloudsPipelines)
    {
        if (pipeline)
            vkDestroyPipeline(_device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (_cloudsLayout)
        vkDestroyPipelineLayout(_device, _cloudsLayout, nullptr);
    if (_cloudsPool)
        vkDestroyDescriptorPool(_device, _cloudsPool, nullptr);
    for (auto& t : _cloudsTargets)
    {
        if (t.framebuffer)
            vkDestroyFramebuffer(_device, t.framebuffer, nullptr);
        if (t.color.view)
            vkDestroyImageView(_device, t.color.view, nullptr);
        if (t.color.image)
            vkDestroyImage(_device, t.color.image, nullptr);
        if (t.color.memory)
            vkFreeMemory(_device, t.color.memory, nullptr);
    }
    _cloudsTargets.clear();
    _cloudsExtent = {};
    _cloudsLayout = VK_NULL_HANDLE;
    _cloudsPool = VK_NULL_HANDLE;
    _cloudsEnabled = _cloudsDrew = false;
}
} // namespace Poseidon::vk
