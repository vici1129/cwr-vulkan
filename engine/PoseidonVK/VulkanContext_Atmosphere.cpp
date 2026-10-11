#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/Shaders/gamma.vert.hpp>
#include <PoseidonVK/Shaders/sun_shafts.frag.hpp>
#include <PoseidonVK/Shaders/sun_shafts_ms.frag.hpp>
#include <cmath>
#include <stdexcept>

namespace Poseidon::vk
{
namespace
{
void Require(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string("Vulkan sun shafts: ") + operation + " failed (" + std::to_string(result) +
                                 ")");
}
} // namespace
std::string VulkanContext::SetAtmosphere(std::string_view mode)
{
    if (mode == "status")
        return std::string("enabled=") + (_atmosphereEnabled ? "on" : "off") +
               " fog=original-transmittance-and-ranges domain=world-linear-HDR";
    if (mode != "on" && mode != "off")
        return "INVALID: on, off, status";
    if (_atmosphereEnabled != (mode == "on"))
    {
        FlushScreenBatch();
        _atmosphereEnabled = mode == "on";
        ResetTemporalHistory();
    }
    return "OK: atmosphere updated";
}
std::string VulkanContext::SetSunShafts(std::string_view mode)
{
    if (mode == "status")
        return std::string("requested=") + (_shaftsRequested ? "on" : "off") +
               " active=" + (_shaftsEnabled && !_shaftTargets.empty() ? "on" : "off") +
               " visibility=" + std::to_string(_sunVisibility) + " weight=" + std::to_string(_screenSun[2]) +
               " sun=" + std::to_string(_screenSun[0]) + "," + std::to_string(_screenSun[1]) +
               " resolution=quarter samples=48 receivers=sky-openings history=none";
    if (mode != "on" && mode != "off")
        return "INVALID: on, off, status";
    if (_shaftsRequested != (mode == "on"))
    {
        _shaftsRequested = mode == "on";
        _recreate = true;
    }
    return "OK: sunlight shafts requested for next frame";
}

void VulkanContext::CreateSunShaftResources()
{
    if (!_shaftsEnabled)
        return;
    _shaftCompositeExtent = IsFSR(_aaMode) ? _extent : _worldExtent;
    _shaftExtent = {std::max(1u, (_shaftCompositeExtent.width + 3) / 4),
                    std::max(1u, (_shaftCompositeExtent.height + 3) / 4)};
    const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(_images.size() * 3)};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = size.descriptorCount;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    Require(vkCreateDescriptorPool(_device, &pool, nullptr, &_shaftPool), "create descriptors");
    _shaftTargets.resize(_images.size());
    auto create = [&](TemporalTarget& target, VkExtent2D extent)
    {
        CreateAAImage(target.color, SceneFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, extent);
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = _aaPass;
        fb.attachmentCount = 1;
        fb.pAttachments = &target.color.view;
        fb.width = extent.width;
        fb.height = extent.height;
        fb.layers = 1;
        Require(vkCreateFramebuffer(_device, &fb, nullptr, &target.framebuffer), "create framebuffer");
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = _shaftPool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &_textureLayout;
        Require(vkAllocateDescriptorSets(_device, &allocation, &target.set), "allocate descriptor");
        const VkDescriptorImageInfo image{_aaSampler, target.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = target.set;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image;
        vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
    };
    for (auto& target : _shaftTargets)
    {
        create(target.mask, _shaftExtent);
        create(target.radial, _shaftExtent);
        create(target.composite, _shaftCompositeExtent);
    }
    const VkDescriptorSetLayout sets[]{_textureLayout, _textureLayout, _textureLayout};
    const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16 * sizeof(float)};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 3;
    layout.pSetLayouts = sets;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    Require(vkCreatePipelineLayout(_device, &layout, nullptr, &_shaftLayout), "create layout");
    VkShaderModule vertex{}, fragment{};
    try
    {
        VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        module.codeSize = sizeof(Cwrgamma_vert);
        module.pCode = Cwrgamma_vert;
        Require(vkCreateShaderModule(_device, &module, nullptr, &vertex), "create vertex shader");
        module.codeSize =
            _worldSamples == VK_SAMPLE_COUNT_1_BIT ? sizeof(Cwrsun_shafts_frag) : sizeof(Cwrsun_shafts_ms_frag);
        module.pCode = _worldSamples == VK_SAMPLE_COUNT_1_BIT ? Cwrsun_shafts_frag : Cwrsun_shafts_ms_frag;
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
        pipeline.layout = _shaftLayout;
        pipeline.renderPass = _aaPass;
        Require(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &_shaftPipeline),
                "create pipeline");
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

VkDescriptorSet VulkanContext::DrawSunShafts(VkDescriptorSet source)
{
    const double now = ProfileClock();
    const float dt = float(std::clamp((now - _shaftLastTime) / 1000.0, 0.0, .1));
    _shaftLastTime = now;
    _shaftVisibility += (_sunVisibility - _shaftVisibility) * (1.f - std::exp(-dt / .12f));
    const float edge = std::min({_screenSun[0], _screenSun[1], 1.f - _screenSun[0], 1.f - _screenSun[1]});
    float fade = std::clamp(edge / .08f, 0.f, 1.f);
    fade = fade * fade * (3 - 2 * fade);
    const float strength = .18f * _screenSun[2] * _shaftVisibility * fade;
    if (strength < .0001f)
        return source;
    auto& frame = _frames[_frame];
    const auto command = frame.command;
    if (frame.frameQueries)
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 6);
    auto& target = _shaftTargets[_image];
    auto draw =
        [&](TemporalTarget& output, VkExtent2D extent, VkDescriptorSet input, VkDescriptorSet radial, float mode)
    {
        VkClearValue clear{};
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = _aaPass;
        pass.framebuffer = output.framebuffer;
        pass.renderArea.extent = extent;
        pass.clearValueCount = 1;
        pass.pClearValues = &clear;
        vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaftPipeline);
        const VkDescriptorSet sets[]{input, radial, _aaTargets[_image].depthSet};
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaftLayout, 0, 3, sets, 0, nullptr);
        const VkViewport viewport{0, 0, float(extent.width), float(extent.height), 0, 1};
        const VkRect2D scissor{{0, 0}, extent};
        vkCmdSetViewport(command, 0, 1, &viewport);
        vkCmdSetScissor(command, 0, 1, &scissor);
        const float push[]{_screenSun[0],
                           _screenSun[1],
                           strength,
                           mode,
                           _atmosphereColor[0],
                           _atmosphereColor[1],
                           _atmosphereColor[2],
                           _aaMode == AAMode::TAA ? 1.f : 0.f,
                           1.f / _shaftExtent.width,
                           1.f / _shaftExtent.height,
                           float(extent.width) / extent.height,
                           0,
                           IsTemporal(_aaMode) ? _taaJitter[0] * .5f : 0,
                           IsTemporal(_aaMode) ? _taaJitter[1] * .5f : 0,
                           float(_worldExtent.width),
                           float(_worldExtent.height)};
        vkCmdPushConstants(command, _shaftLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), push);
        vkCmdDraw(command, 3, 1, 0, 0);
        vkCmdEndRenderPass(command);
    };
    draw(target.mask, _shaftExtent, source, source, 0);
    draw(target.radial, _shaftExtent, target.mask.set, target.mask.set, 1);
    draw(target.composite, _shaftCompositeExtent, source, target.radial.set, 2);
    if (frame.frameQueries)
    {
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 7);
        frame.shaftsTimestamped = true;
    }
    _commands = {};
    return target.composite.set;
}

void VulkanContext::DestroySunShaftResources() noexcept
{
    if (_shaftPipeline)
        vkDestroyPipeline(_device, _shaftPipeline, nullptr);
    if (_shaftLayout)
        vkDestroyPipelineLayout(_device, _shaftLayout, nullptr);
    if (_shaftPool)
        vkDestroyDescriptorPool(_device, _shaftPool, nullptr);
    for (auto& target : _shaftTargets)
        for (auto* t : {&target.mask, &target.radial, &target.composite})
        {
            if (t->framebuffer)
                vkDestroyFramebuffer(_device, t->framebuffer, nullptr);
            if (t->color.view)
                vkDestroyImageView(_device, t->color.view, nullptr);
            if (t->color.image)
                vkDestroyImage(_device, t->color.image, nullptr);
            if (t->color.memory)
                vkFreeMemory(_device, t->color.memory, nullptr);
        }
    _shaftTargets.clear();
    _shaftExtent = _shaftCompositeExtent = {};
    _shaftPipeline = VK_NULL_HANDLE;
    _shaftLayout = VK_NULL_HANDLE;
    _shaftPool = VK_NULL_HANDLE;
    _shaftsEnabled = false;
    _shaftVisibility = 0;
    _shaftLastTime = 0;
}
} // namespace Poseidon::vk
