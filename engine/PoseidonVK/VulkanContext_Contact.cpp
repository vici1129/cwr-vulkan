#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/Shaders/gamma.vert.hpp>
#include <PoseidonVK/Shaders/contact_shadows.frag.hpp>
#include <PoseidonVK/Shaders/contact_shadows_ms.frag.hpp>
#include <PoseidonVK/Shaders/contact_shadows_rt.frag.hpp>
#include <PoseidonVK/Shaders/contact_shadows_rt_ms.frag.hpp>
#include <stdexcept>

namespace Poseidon::vk
{
namespace
{
void Require(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string("Vulkan contact shadows: ") + operation + " failed (" +
                                 std::to_string(result) + ")");
}
struct ContactPush
{
    std::array<float, 16> camera;
    std::array<float, 4> projection, settings, jitter, sun;
};
static_assert(sizeof(ContactPush) == 128);
}

std::string VulkanContext::SetContactShadows(std::string_view mode)
{
    if (mode == "status")
        return std::string("requested=") + (_contactRequested ? "on" : "off") +
               " active=" + (_contactEnabled ? "on" : "off") +
               " lastDraw=" + (_contactDrew ? "yes" : "no") + " draws=" + std::to_string(_contactDraws) +
               " sunlight=" + std::to_string(_volumeFrame.atmosphereSun[3]) +
               " world=" + std::to_string(_worldExtent.width) + "x" + std::to_string(_worldExtent.height) +
               " reach=1.2m steps=16 fade=25-40m combine=min(CSM,RT,contact)";
    if (mode != "on" && mode != "off")
        return "INVALID: triContactShadows on, off, status";
    if (mode == "on" && _physical)
    {
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(_physical, &features);
        if (!features.independentBlend)
            return "UNSUPPORTED: contact sunlight requires independent MRT blending";
    }
    if (_contactRequested != (mode == "on"))
    {
        _contactRequested = mode == "on";
        _recreate = true;
        ResetTemporalHistory();
    }
    return "OK: contact shadows requested for next frame";
}

void VulkanContext::CreateContactResources()
{
    if (!_contactEnabled)
        return;
    // Reuse world depth and separated sunlight; no contact image or history.
    VkDescriptorSetLayoutBinding bindings[3]{};
    const uint32_t indices[]{0, 4, 6};
    for (unsigned i = 0; i < 3; ++i)
        bindings[i] = {indices[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                       VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set.bindingCount = 3;
    set.pBindings = bindings;
    Require(vkCreateDescriptorSetLayout(_device, &set, nullptr, &_contactSetLayout), "create descriptors");
    const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(_frames.size()) * 3};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = uint32_t(_frames.size());
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    Require(vkCreateDescriptorPool(_device, &pool, nullptr, &_contactPool), "create pool");
    _contactSets.resize(_frames.size());
    for (auto& descriptor : _contactSets)
    {
        VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = _contactPool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &_contactSetLayout;
        Require(vkAllocateDescriptorSets(_device, &alloc, &descriptor), "allocate descriptor");
    }
    const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ContactPush)};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &_contactSetLayout;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    Require(vkCreatePipelineLayout(_device, &layout, nullptr, &_contactLayout), "create layout");
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
        output.colorWriteMask = 7;
        output.blendEnable = VK_TRUE;
        output.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
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
        pipeline.layout = _contactLayout;
        pipeline.renderPass = _ssaoPass;
        const bool ms = _worldSamples != VK_SAMPLE_COUNT_1_BIT;
        for (unsigned i = 0; i < 2; ++i)
        {
            module.codeSize = i ? (ms ? sizeof(Cwrcontact_shadows_rt_ms_frag) : sizeof(Cwrcontact_shadows_rt_frag))
                                : (ms ? sizeof(Cwrcontact_shadows_ms_frag) : sizeof(Cwrcontact_shadows_frag));
            module.pCode = i ? (ms ? Cwrcontact_shadows_rt_ms_frag : Cwrcontact_shadows_rt_frag)
                             : (ms ? Cwrcontact_shadows_ms_frag : Cwrcontact_shadows_frag);
            Require(vkCreateShaderModule(_device, &module, nullptr, &fragment), "create fragment shader");
            stages[1].module = fragment;
            Require(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &_contactPipelines[i]),
                    "create pipeline");
            vkDestroyShaderModule(_device, fragment, nullptr);
            fragment = VK_NULL_HANDLE;
        }
    }
    catch (...)
    {
        if (fragment) vkDestroyShaderModule(_device, fragment, nullptr);
        if (vertex) vkDestroyShaderModule(_device, vertex, nullptr);
        throw;
    }
    vkDestroyShaderModule(_device, vertex, nullptr);
}

void VulkanContext::DrawContactShadows(const std::array<float, 4>& projection, VkImageView rtSun)
{
    if (!_worldActive || !ContactShadowsEligible() || _contactSets.empty())
        return;
    FlushScreenBatch();
    auto& frame = _frames[_frame];
    const auto command = frame.command;
    if (_worldPassOpen)
        vkCmdEndRenderPass(command);
    _worldPassOpen = false;
    if (frame.frameQueries)
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 10);
    const auto set = _contactSets[_frame];
    const VkDescriptorImageInfo images[]{
        {_aaSampler, _aaTargets[_image].sampledDepth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL},
        {_aaSampler, _aaTargets[_image].sunlight.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {_aaSampler, rtSun, VK_IMAGE_LAYOUT_GENERAL}};
    const uint32_t bindings[]{0, 6, 4};
    VkWriteDescriptorSet writes[3]{};
    for (unsigned i = 0; i < (rtSun ? 3u : 2u); ++i)
    {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = bindings[i];
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &images[i];
    }
    vkUpdateDescriptorSets(_device, rtSun ? 3 : 2, writes, 0, nullptr);
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = _ssaoPass;
    pass.framebuffer = _aaTargets[_image].ao;
    pass.renderArea.extent = _worldExtent;
    vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _contactPipelines[rtSun ? 1 : 0]);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _contactLayout, 0, 1, &set, 0, nullptr);
    const VkViewport viewport{0, 0, float(_worldExtent.width), float(_worldExtent.height), 0, 1};
    const VkRect2D scissor{{0, 0}, _worldExtent};
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);
    ContactPush push{};
    push.camera = _volumeFrame.world;
    push.projection = projection;
    push.sun = _volumeFrame.atmosphereSun;
    if (IsTemporal(_aaMode))
    {
        push.jitter[0] = _taaJitter[0];
        push.jitter[1] = _taaJitter[1];
    }
    vkCmdPushConstants(command, _contactLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
    vkCmdDraw(command, 3, 1, 0, 0);
    vkCmdEndRenderPass(command);
    if (frame.frameQueries)
    {
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 11);
        frame.contactTimestamped = true;
    }
    _contactDrew = true;
    ++_contactDraws;
    _commands = {};
}

void VulkanContext::DestroyContactResources() noexcept
{
    for (auto& pipeline : _contactPipelines)
    {
        if (pipeline) vkDestroyPipeline(_device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (_contactLayout) vkDestroyPipelineLayout(_device, _contactLayout, nullptr);
    if (_contactPool) vkDestroyDescriptorPool(_device, _contactPool, nullptr);
    if (_contactSetLayout) vkDestroyDescriptorSetLayout(_device, _contactSetLayout, nullptr);
    _contactLayout = VK_NULL_HANDLE;
    _contactPool = VK_NULL_HANDLE;
    _contactSetLayout = VK_NULL_HANDLE;
    _contactSets.clear();
    _contactEnabled = _contactDrew = false;
}
} // namespace Poseidon::vk
