#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/Shaders/gamma.vert.hpp>
#include <PoseidonVK/Shaders/volumetric_fog.frag.hpp>
#include <PoseidonVK/Shaders/volumetric_fog_ms.frag.hpp>
#include <stdexcept>

namespace Poseidon::vk
{
namespace
{
void Require(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string("Vulkan volumetric fog: ") + operation + " failed (" +
                                 std::to_string(result) + ")");
}
} // namespace
std::string VulkanContext::SetVolumetricFog(std::string_view mode)
{
    if (mode == "status")
        return std::string("requested=") + (_volumeRequested ? "on" : "off") +
               " active=" + (_volumeEnabled && !_volumeTargets.empty() ? "on" : "off") +
               " scattering=" + (_volumeDrew ? "on" : "off") + " draws=" + std::to_string(_volumeDraws) +
               " csm=" + (_csmActive ? "ready" : "unavailable") + " csmReach=" +
               std::to_string(_csmActive ? _csmState.splits[std::clamp(int(_csmState.control[0]), 1, 4) - 1] : 0) +
               " sunlight=" + std::to_string(_volumeFrame.atmosphereSun[3]) +
               " fog=" + std::to_string(_volumeFrame.fogParams[0]) + "," + std::to_string(_volumeFrame.fogParams[1]) +
               " half=" + std::to_string(_volumeExtent.width) + "x" + std::to_string(_volumeExtent.height) +
               " steps=48 reach=400m history=world-AA shafts=exclusive";
    if (mode != "on" && mode != "off")
        return "INVALID: on, off, status";
    if (_volumeRequested != (mode == "on"))
    {
        _volumeRequested = mode == "on";
        _recreate = true;
        ResetTemporalHistory();
    }
    return "OK: volumetric fog requested for next frame";
}

void VulkanContext::CreateVolumetricResources()
{
    if (!_volumeEnabled)
        return;
    _volumeExtent = {std::max(1u, (_worldExtent.width + 1) / 2), std::max(1u, (_worldExtent.height + 1) / 2)};
    const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(_images.size())};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = size.descriptorCount;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    Require(vkCreateDescriptorPool(_device, &pool, nullptr, &_volumePool), "create pool");
    _volumeTargets.resize(_images.size());
    for (auto& target : _volumeTargets)
    {
        CreateAAImage(target.color, SceneFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, _volumeExtent);
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = _aaPass;
        fb.attachmentCount = 1;
        fb.pAttachments = &target.color.view;
        fb.width = _volumeExtent.width;
        fb.height = _volumeExtent.height;
        fb.layers = 1;
        Require(vkCreateFramebuffer(_device, &fb, nullptr, &target.framebuffer), "create framebuffer");
        VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = _volumePool;
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
    const VkDescriptorSetLayout sets[]{_textureLayout, _textureLayout, _lightingLayout, _weatherLayout};
    const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 32};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 4;
    layout.pSetLayouts = sets;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    Require(vkCreatePipelineLayout(_device, &layout, nullptr, &_volumeLayout), "create layout");
    VkShaderModule vertex{}, fragment{};
    try
    {
        VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        module.codeSize = sizeof(Cwrgamma_vert);
        module.pCode = Cwrgamma_vert;
        Require(vkCreateShaderModule(_device, &module, nullptr, &vertex), "create vertex shader");
        module.codeSize =
            _worldSamples == VK_SAMPLE_COUNT_1_BIT ? sizeof(Cwrvolumetric_fog_frag) : sizeof(Cwrvolumetric_fog_ms_frag);
        module.pCode = _worldSamples == VK_SAMPLE_COUNT_1_BIT ? Cwrvolumetric_fog_frag : Cwrvolumetric_fog_ms_frag;
        Require(vkCreateShaderModule(_device, &module, nullptr, &fragment), "create fragment shader");
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
        output.srcColorBlendFactor = output.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        output.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        output.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &output;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = states;
        VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline.stageCount = 2;
        pipeline.pStages = stages;
        pipeline.pVertexInputState = &input;
        pipeline.pInputAssemblyState = &assembly;
        pipeline.pViewportState = &viewport;
        pipeline.pRasterizationState = &raster;
        pipeline.pMultisampleState = &samples;
        pipeline.pColorBlendState = &blend;
        pipeline.pDynamicState = &dynamic;
        pipeline.layout = _volumeLayout;
        for (int i = 0; i < 2; ++i)
        {
            output.blendEnable = i ? VK_TRUE : VK_FALSE;
            pipeline.renderPass = i ? _ssaoPass : _aaPass;
            Require(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &_volumePipelines[i]),
                    "create pipeline");
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

void VulkanContext::DrawVolumetricFog(const std::array<float, 4>& projection)
{
    _volumeDrew = false;
    if (!VolumetricScatteringEligible() || !_csmActive || _volumeTargets.empty())
        return;
    auto& frame = _frames[_frame];
    const auto command = frame.command;
    if (frame.frameQueries)
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 8);
    auto lighting = _volumeFrame;
    lighting.shadow = _csmState;
    // Reuse the existing frame-fenced uniform allocator, then bind its result
    // explicitly to this layout (the shape push-constant range is different).
    BindLighting(lighting, false);
    const auto& uniform = frame.nativeUniform;
    for (unsigned i = 0; i < 2; ++i)
    {
        const auto extent = i ? _worldExtent : _volumeExtent;
        VkClearValue clear{};
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = i ? _ssaoPass : _aaPass;
        pass.framebuffer = i ? _aaTargets[_image].ao : _volumeTargets[_image].framebuffer;
        pass.renderArea.extent = extent;
        pass.clearValueCount = 1;
        pass.pClearValues = &clear;
        vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _volumePipelines[i]);
        const VkDescriptorSet sets[]{i ? _volumeTargets[_image].set : _aaTargets[_image].colorSet,
                                     _aaTargets[_image].depthSet, uniform.set, PrepareWeather()};
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _volumeLayout, 0, 4, sets, 1,
                                &uniform.offset);
        const VkViewport viewport{0, 0, float(extent.width), float(extent.height), 0, 1};
        const VkRect2D scissor{{0, 0}, extent};
        vkCmdSetViewport(command, 0, 1, &viewport);
        vkCmdSetScissor(command, 0, 1, &scissor);
        const float push[]{projection[0],
                           projection[1],
                           projection[2],
                           projection[3],
                           float(i),
                           IsTemporal(_aaMode) ? _taaJitter[0] : 0,
                           IsTemporal(_aaMode) ? _taaJitter[1] : 0,
                           400};
        vkCmdPushConstants(command, _volumeLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), push);
        vkCmdDraw(command, 3, 1, 0, 0);
        vkCmdEndRenderPass(command);
    }
    if (frame.frameQueries)
    {
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 9);
        frame.volumeTimestamped = true;
    }
    _volumeDrew = true;
    ++_volumeDraws;
    _commands = {};
}
void VulkanContext::DestroyVolumetricResources() noexcept
{
    for (auto& pipeline : _volumePipelines)
    {
        if (pipeline)
            vkDestroyPipeline(_device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (_volumeLayout)
        vkDestroyPipelineLayout(_device, _volumeLayout, nullptr);
    if (_volumePool)
        vkDestroyDescriptorPool(_device, _volumePool, nullptr);
    for (auto& t : _volumeTargets)
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
    _volumeTargets.clear();
    _volumeExtent = {};
    _volumeLayout = VK_NULL_HANDLE;
    _volumePool = VK_NULL_HANDLE;
    _volumeEnabled = _volumeDrew = false;
}
} // namespace Poseidon::vk
