#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/Shaders/gamma.vert.hpp>
#include <PoseidonVK/Shaders/bloom.frag.hpp>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace Poseidon::vk
{
namespace
{
void Require(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string("Vulkan bloom: ") + operation + " failed (" + std::to_string(result) + ")");
}
}

std::string VulkanContext::SetBloom(std::string_view mode)
{
    if (mode == "status")
        return std::string("requested=") + (_bloomRequested ? "on" : "off") +
            " active=" + (_bloomEnabled && !_bloomTargets.empty() ? "on" : "off") +
            " threshold=" + std::to_string(_bloomThreshold) + " knee=25% strength=" + std::to_string(_bloomStrength) +
            " levels=" + std::to_string(_bloomTargets.empty() ? 0 : BloomLevels) +
            " base=" + std::to_string(_bloomExtents[0].width) + "x" + std::to_string(_bloomExtents[0].height) +
            " domain=world-linear-HDR-after-AA-before-tonemap overlays=excluded";
    if (mode == "on" || mode == "off")
    {
        const bool enabled = mode == "on";
        if (enabled != _bloomRequested) { _bloomRequested = enabled; _recreate = true; }
        return "OK: bloom requested for next frame; use status for active state";
    }
    const bool threshold = mode.starts_with("threshold ");
    if (threshold || mode.starts_with("strength "))
    {
        const std::string number(mode.substr(threshold ? 10 : 9));
        char* end = nullptr;
        const float value = std::strtof(number.c_str(), &end);
        if (end == number.c_str() || *end || !std::isfinite(value) ||
            (threshold ? value < 1 || value > 32 : value < 0 || value > .3f))
            return "INVALID: threshold 1..32 linear units; strength 0..0.3";
        if (threshold) _bloomThreshold = value;
        else _bloomStrength = value;
        return "OK: bloom tuning updated";
    }
    return "INVALID: on, off, status, threshold <1..32>, strength <0..0.3>";
}

void VulkanContext::CreateBloomResources()
{
    if (!_bloomEnabled) return;
    // Per acquired image, owned by the same idle recreation as AA. No history
    // and no read/write aliasing between downsample and upsample attachments.
    VkExtent2D extent = _extent;
    for (auto& level : _bloomExtents)
    {
        extent = {std::max(1u, (extent.width + 1) / 2), std::max(1u, (extent.height + 1) / 2)};
        level = extent;
    }
    const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                    uint32_t(_images.size() * (2 * BloomLevels - 1))};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = size.descriptorCount;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    Require(vkCreateDescriptorPool(_device, &pool, nullptr, &_bloomPool), "create descriptors");
    _bloomTargets.resize(_images.size());
    auto create = [&](TemporalTarget& target, VkExtent2D dimensions)
    {
        CreateAAImage(target.color, SceneFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, dimensions);
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = _aaPass;
        fb.attachmentCount = 1;
        fb.pAttachments = &target.color.view;
        fb.width = dimensions.width; fb.height = dimensions.height; fb.layers = 1;
        Require(vkCreateFramebuffer(_device, &fb, nullptr, &target.framebuffer), "create framebuffer");
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = _bloomPool;
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
    for (auto& target : _bloomTargets)
        for (unsigned i = 0; i < BloomLevels; ++i)
        {
            create(target.down[i], _bloomExtents[i]);
            if (i + 1 < BloomLevels) create(target.up[i], _bloomExtents[i]);
        }
    const VkDescriptorSetLayout sets[]{_textureLayout, _textureLayout};
    const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float)};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 2; layout.pSetLayouts = sets;
    layout.pushConstantRangeCount = 1; layout.pPushConstantRanges = &push;
    Require(vkCreatePipelineLayout(_device, &layout, nullptr, &_bloomLayout), "create layout");
    VkShaderModule vertex{}, fragment{};
    try
    {
        VkShaderModuleCreateInfo module{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        module.codeSize = sizeof(Cwrgamma_vert); module.pCode = Cwrgamma_vert;
        Require(vkCreateShaderModule(_device, &module, nullptr, &vertex), "create vertex shader");
        module.codeSize = sizeof(Cwrbloom_frag); module.pCode = Cwrbloom_frag;
        Require(vkCreateShaderModule(_device, &module, nullptr, &fragment), "create fragment shader");
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT; stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[0].module = vertex; stages[1].module = fragment;
        stages[0].pName = stages[1].pName = "main";
        VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL; raster.cullMode = VK_CULL_MODE_NONE; raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState output{}; output.colorWriteMask = 15;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1; blend.pAttachments = &output;
        const VkDynamicState states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2; dynamic.pDynamicStates = states;
        VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline.stageCount = 2; pipeline.pStages = stages;
        pipeline.pVertexInputState = &input; pipeline.pInputAssemblyState = &assembly;
        pipeline.pViewportState = &viewport; pipeline.pRasterizationState = &raster;
        pipeline.pMultisampleState = &samples; pipeline.pColorBlendState = &blend;
        pipeline.pDynamicState = &dynamic; pipeline.layout = _bloomLayout; pipeline.renderPass = _aaPass;
        Require(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &_bloomPipeline), "create pipeline");
    }
    catch (...)
    {
        if (fragment) vkDestroyShaderModule(_device, fragment, nullptr);
        if (vertex) vkDestroyShaderModule(_device, vertex, nullptr);
        throw; // Swapchain teardown releases all partially created targets.
    }
    vkDestroyShaderModule(_device, fragment, nullptr);
    vkDestroyShaderModule(_device, vertex, nullptr);
}

void VulkanContext::DrawBloom(VkDescriptorSet source)
{
    auto& frame = _frames[_frame];
    auto command = frame.command;
    if (frame.frameQueries) vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 4);
    auto& target = _bloomTargets[_image];
    auto draw = [&](TemporalTarget& output, unsigned level, VkDescriptorSet input, VkDescriptorSet detail,
                    VkExtent2D inputSize, float mode)
    {
        VkClearValue clear{};
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = _aaPass; pass.framebuffer = output.framebuffer;
        pass.renderArea.extent = _bloomExtents[level]; pass.clearValueCount = 1; pass.pClearValues = &clear;
        vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _bloomPipeline);
        const VkDescriptorSet sets[]{input, detail};
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _bloomLayout, 0, 2, sets, 0, nullptr);
        const VkViewport viewport{0, 0, float(_bloomExtents[level].width), float(_bloomExtents[level].height), 0, 1};
        const VkRect2D scissor{{0, 0}, _bloomExtents[level]};
        vkCmdSetViewport(command, 0, 1, &viewport); vkCmdSetScissor(command, 0, 1, &scissor);
        const float push[]{1.f / inputSize.width, 1.f / inputSize.height, _bloomThreshold, mode};
        vkCmdPushConstants(command, _bloomLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), push);
        vkCmdDraw(command, 3, 1, 0, 0);
        vkCmdEndRenderPass(command);
    };
    auto size = IsFSR(_aaMode) ? _extent : _worldExtent;
    for (unsigned i = 0; i < BloomLevels; ++i)
    {
        draw(target.down[i], i, source, source, size, i == 0 ? 0.f : 1.f);
        source = target.down[i].set; size = _bloomExtents[i];
    }
    for (int i = BloomLevels - 2; i >= 0; --i)
    {
        draw(target.up[i], unsigned(i), source, target.down[i].set, size, 2);
        source = target.up[i].set; size = _bloomExtents[i];
    }
    if (frame.frameQueries)
    {
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 5);
        frame.bloomTimestamped = true;
    }
    _commands = {};
}

void VulkanContext::DestroyBloomResources() noexcept
{
    if (_bloomPipeline) vkDestroyPipeline(_device, _bloomPipeline, nullptr);
    if (_bloomLayout) vkDestroyPipelineLayout(_device, _bloomLayout, nullptr);
    if (_bloomPool) vkDestroyDescriptorPool(_device, _bloomPool, nullptr);
    auto destroy = [&](TemporalTarget& target)
    {
        if (target.framebuffer) vkDestroyFramebuffer(_device, target.framebuffer, nullptr);
        if (target.color.view) vkDestroyImageView(_device, target.color.view, nullptr);
        if (target.color.image) vkDestroyImage(_device, target.color.image, nullptr);
        if (target.color.memory) vkFreeMemory(_device, target.color.memory, nullptr);
    };
    for (auto& target : _bloomTargets)
    {
        for (auto& level : target.down) destroy(level);
        for (auto& level : target.up) destroy(level);
    }
    _bloomTargets.clear(); _bloomExtents = {};
    _bloomPipeline = VK_NULL_HANDLE; _bloomLayout = VK_NULL_HANDLE; _bloomPool = VK_NULL_HANDLE;
    _bloomEnabled = false;
}
} // namespace Poseidon::vk
