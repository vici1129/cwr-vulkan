#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/Shaders/gamma.vert.hpp>
#include <PoseidonVK/Shaders/water_capture.frag.hpp>
#include <PoseidonVK/Shaders/water_capture_ms.frag.hpp>
#include <PoseidonVK/Shaders/water_refraction.frag.hpp>
#include <PoseidonVK/Shaders/water_refraction_ms.frag.hpp>
#include <stdexcept>
#include <cmath>

namespace Poseidon::vk
{
namespace
{
void Require(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string("Vulkan water refraction: ") + operation + " failed (" +
                                 std::to_string(result) + ")");
}
struct WaterPush
{
    std::array<float, 16> camera;
    std::array<float, 4> projection, jitter, effects, originTime;
};
static_assert(sizeof(WaterPush) == 128);
} // namespace

std::string VulkanContext::SetRainRipples(std::string_view mode, float rain)
{
    if (mode == "status")
        return std::string("RainRipples requested=") + (_rainRipplesEnabled ? "on" : "off") +
               " active=" + (_rainRipplesEnabled && rain > 0 ? "on" : "off") +
               " rain=" + std::to_string(rain) + "; actual water only, simulation time, no RT requirement";
    if (mode != "on" && mode != "off")
        return "INVALID: triRainRipples on, off, status";
    _rainRipplesEnabled = mode == "on";
    return "OK: rain ripples " + std::string(mode);
}

std::string VulkanContext::SetPBR(std::string_view mode)
{
    if (mode == "status") return std::string("PBR prototype=stock-dielectric-v3 requested=") +
        (_pbrEnabled ? "on" : "off") + "; enhanced=" + (_enhancedMaterials ? "on" : "off") +
        "; native + captured CPU-lit opaque geometry: data/skala_piskovec2.pac (matte rock), "
        "data/sloupek_bily_orezly_.pac (painted signposts), "
        "data/strecha_tasky_vln.pac + data/strecha_tasky_vln90.pac (roof tiles r=0.90), "
        "data/prkna_dark.pac (weathered wood r=0.85); "
        "inspected stone/mortar, concrete, plaster/brick and wood sections; "
        "F0=0.04, metallic=0; unsupported lighting/atlases legacy; no RT requirement; "
        "draws since toggle=" + std::to_string(_pbrDraws) + "; software=" + std::to_string(_pbrSoftwareDraws);
    if (mode != "on" && mode != "off" && mode != "enhanced") return "INVALID: triPBR on, off, enhanced, status";
    if (_pbrEnabled != (mode != "off") || _enhancedMaterials != (mode == "enhanced"))
    { ResetTemporalHistory(); _pbrDraws = {}; _pbrSoftwareDraws = {}; }
    _pbrEnabled = mode != "off";
    _enhancedMaterials = mode == "enhanced";
    return "OK: PBR stock-dielectric-v3 " + std::string(mode);
}

std::string VulkanContext::SetWetSurfaces(std::string_view mode)
{
    if (mode == "status") return std::string("WetSurfaces requested=") + (_wetSurfacesEnabled ? "on" : "off") +
        " rain=" + std::to_string(_wetRain) + "; classified ground and road edges only; no RT requirement";
    if (mode != "on" && mode != "off") return "INVALID: triWetSurfaces on, off, status";
    if (_wetSurfacesEnabled != (mode == "on")) ResetTemporalHistory();
    _wetSurfacesEnabled = mode == "on";
    return "OK: wet surfaces " + std::string(mode);
}

std::string VulkanContext::SetUnderwater(std::string_view mode)
{
    if (mode == "status") return std::string("Underwater requested=") + (_underwaterEnabled ? "on" : "off") +
        " depth=" + std::to_string(_underwater[0]) + "; world composite only; animated sea level; no RT requirement";
    if (mode != "on" && mode != "off") return "INVALID: triUnderwater on, off, status";
    _underwaterEnabled = mode == "on";
    return "OK: underwater " + std::string(mode);
}

void VulkanContext::SetEnvironmentFrame(float rain, float time, const std::array<float, 12>& underwater)
{
    rain = std::isfinite(rain) ? std::clamp(rain, 0.f, 1.f) : 0.f;
    if (_wetTime < 0 || time < _wetTime) _wetRain = rain;
    else _wetRain += (rain - _wetRain) * (1.f - std::exp(-std::max(time - _wetTime, 0.f) / 2.f));
    _wetTime = time;
    _underwater = underwater;
}

std::string VulkanContext::SetWaterRefraction(std::string_view mode)
{
    if (mode == "status")
        return std::string("WaterRefraction requested=") + (_refractionRequested ? "on" : "off") +
               " active=" + (_refractionEnabled ? "on" : "off") + " captured=" + (_waterCaptured ? "yes" : "no") +
               " composited=" + (_waterRefracted ? "yes" : "no") + " captures=" + std::to_string(_waterCaptures) +
               " composites=" + std::to_string(_waterComposites) +
               " lastGPUms(capture/optics)=" + std::to_string(_waterCaptureMs) + "/" + std::to_string(_waterOpticsMs) +
               " fallback=" + (_refractionFailure.empty() ? "none" : _refractionFailure) +
               "; raster-only coastal terrain, missing bottom/underwater retains original water";
    if (mode != "on" && mode != "off")
        return "INVALID: triWaterRefraction on, off, status";
    if (mode == "on")
        if (const auto failure = WaterCaptureSupport(); !failure.empty())
            return "UNSUPPORTED: " + failure;
    if (_refractionRequested != (mode == "on"))
    {
        _refractionRequested = mode == "on";
        _recreate = true;
        ResetTemporalHistory();
    }
    return "OK: water refraction requested for next frame";
}

std::string VulkanContext::WaterCaptureSupport() const
{
    if (!_refractionFailure.empty()) return _refractionFailure;
    if (_physical && _aaColorFormat != SceneFormat) return "water capture requires FP16 HDR";
    VkPhysicalDeviceFeatures features{};
    if (_physical) vkGetPhysicalDeviceFeatures(_physical, &features);
    if (_physical && !features.independentBlend) return "water requires independent MRT blending";
    return {};
}

std::string VulkanContext::SetShoreFoam(std::string_view mode)
{
    if (mode == "status")
        return std::string("ShoreFoam requested=") + (_shoreFoamRequested ? "on" : "off") +
               " active=" + (_shoreFoamEnabled ? "on" : "off") +
               " captured=" + (_waterCaptured ? "yes" : "no") +
               " composited=" + (_waterRefracted ? "yes" : "no") +
               " lastGPUms(shared capture/optics)=" + std::to_string(_waterCaptureMs) + "/" +
               std::to_string(_waterOpticsMs) + " fallback=" +
               (_refractionFailure.empty() ? "none" : _refractionFailure) +
               "; terrain coast only, simulation time, independent refraction, no RT requirement";
    if (mode != "on" && mode != "off") return "INVALID: triShoreFoam on, off, status";
    if (mode == "on")
        if (const auto failure = WaterCaptureSupport(); !failure.empty())
            return "UNSUPPORTED: " + failure;
    if (_shoreFoamRequested != (mode == "on"))
    {
        _shoreFoamRequested = mode == "on";
        _recreate = true;
        ResetTemporalHistory();
    }
    return "OK: shoreline foam requested for next frame";
}

void VulkanContext::FailWaterRefraction(std::string_view reason)
{
    if (!_refractionRequested && !_refractionEnabled && !_shoreFoamRequested && !_shoreFoamEnabled)
        return;
    _refractionFailure = reason;
    _refractionRequested = _refractionEnabled = false;
    _shoreFoamRequested = _shoreFoamEnabled = false;
    _recreate = true;
    ResetTemporalHistory();
}

void VulkanContext::CreateWaterResources()
{
    if (!WaterCaptureEnabled())
        return;
    VkDescriptorSetLayoutBinding bindings[4]{};
    for (unsigned i = 0; i < 4; ++i)
        bindings[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set.bindingCount = 4;
    set.pBindings = bindings;
    Require(vkCreateDescriptorSetLayout(_device, &set, nullptr, &_waterSetLayout), "create descriptor layout");
    const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(_images.size()) * 4};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = uint32_t(_images.size());
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    Require(vkCreateDescriptorPool(_device, &pool, nullptr, &_waterPool), "create pool");
    _waterBackground.resize(_images.size());
    for (unsigned i = 0; i < _waterBackground.size(); ++i)
    {
        auto& target = _waterBackground[i];
        CreateAAImage(target.color, _aaColorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = _aaPass;
        fb.attachmentCount = 1;
        fb.pAttachments = &target.color.view;
        fb.width = _worldExtent.width;
        fb.height = _worldExtent.height;
        fb.layers = 1;
        Require(vkCreateFramebuffer(_device, &fb, nullptr, &target.framebuffer), "create capture framebuffer");
        VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = _waterPool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &_waterSetLayout;
        Require(vkAllocateDescriptorSets(_device, &alloc, &target.set), "allocate descriptor");
        const auto& scene = _aaTargets[i];
        const VkDescriptorImageInfo images[]{
            {_aaSampler, scene.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {_textureSamplers[7], scene.sampledDepth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL},
            {_aaSampler, scene.water.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {_aaSampler, target.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
        VkWriteDescriptorSet writes[4]{};
        for (unsigned j = 0; j < 4; ++j)
        {
            writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[j].dstSet = target.set;
            writes[j].dstBinding = j;
            writes[j].descriptorCount = 1;
            writes[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[j].pImageInfo = &images[j];
        }
        vkUpdateDescriptorSets(_device, 4, writes, 0, nullptr);
    }
    const VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(WaterPush)};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &_waterSetLayout;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    Require(vkCreatePipelineLayout(_device, &layout, nullptr, &_waterLayout), "create layout");
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
        pipeline.layout = _waterLayout;
        const bool ms = _worldSamples != VK_SAMPLE_COUNT_1_BIT;
        for (unsigned i = 0; i < 2; ++i)
        {
            module.codeSize = i ? (ms ? sizeof(Cwrwater_refraction_ms_frag) : sizeof(Cwrwater_refraction_frag))
                                : (ms ? sizeof(Cwrwater_capture_ms_frag) : sizeof(Cwrwater_capture_frag));
            module.pCode = i ? (ms ? Cwrwater_refraction_ms_frag : Cwrwater_refraction_frag)
                             : (ms ? Cwrwater_capture_ms_frag : Cwrwater_capture_frag);
            Require(vkCreateShaderModule(_device, &module, nullptr, &fragment), "create fragment shader");
            stages[1].module = fragment;
            pipeline.renderPass = i ? _ssaoPass : _aaPass;
            if (i)
            {
                output.colorWriteMask = 7;
                output.blendEnable = VK_TRUE;
                output.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                output.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                output.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                output.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            }
            Require(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &_waterPipelines[i]),
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
    vkDestroyShaderModule(_device, vertex, nullptr);
}

void VulkanContext::CaptureWaterBackground()
{
    if (!WaterCaptureEnabled() || _waterCaptured || !_worldActive || !_worldPassOpen || _waterBackground.empty())
        return;
    auto& frame = _frames[_frame];
    const auto command = frame.command;
    // The first actual water draw follows terrain, before water can overwrite depth/color.
    // End/resolve the world attachments before sampling; resume with LOAD afterwards.
    if (frame.frameQueries)
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 12);
    vkCmdEndRenderPass(command);
    VkClearValue clear{};
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = _aaPass;
    pass.framebuffer = _waterBackground[_image].framebuffer;
    pass.renderArea.extent = _worldExtent;
    pass.clearValueCount = 1;
    pass.pClearValues = &clear;
    vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _waterPipelines[0]);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _waterLayout, 0, 1, &_waterBackground[_image].set,
                            0, nullptr);
    const VkViewport viewport{0, 0, float(_worldExtent.width), float(_worldExtent.height), 0, 1};
    const VkRect2D scissor{{0, 0}, _worldExtent};
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);
    const WaterPush push{_waterCamera, _waterProjection, {}, {}, {}};
    vkCmdPushConstants(command, _waterLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
    vkCmdDraw(command, 3, 1, 0, 0);
    vkCmdEndRenderPass(command);
    pass.renderPass = _worldResume;
    pass.framebuffer = SceneFramebuffer();
    pass.clearValueCount = 0;
    vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
    if (frame.frameQueries)
    {
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 13);
        frame.waterCaptureTimestamped = true;
    }
    _waterCaptured = true;
    ++_waterCaptures;
    _commands = {};
}

void VulkanContext::DrawWaterRefraction()
{
    if (!_worldActive || !WaterCaptureEnabled() || !_waterCaptured || _waterRefracted)
        return;
    FlushScreenBatch();
    auto& frame = _frames[_frame];
    const auto command = frame.command;
    if (_worldPassOpen)
        vkCmdEndRenderPass(command);
    _worldPassOpen = false;
    if (frame.frameQueries)
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 14);
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = _ssaoPass;
    pass.framebuffer = _aaTargets[_image].ao;
    pass.renderArea.extent = _worldExtent;
    vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _waterPipelines[1]);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, _waterLayout, 0, 1, &_waterBackground[_image].set,
                            0, nullptr);
    const VkViewport viewport{0, 0, float(_worldExtent.width), float(_worldExtent.height), 0, 1};
    const VkRect2D scissor{{0, 0}, _worldExtent};
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);
    WaterPush push{_waterCamera, _waterProjection, {},
                   {_refractionEnabled ? 1.f : 0.f, _shoreFoamEnabled ? 1.f : 0.f, 0, 0}, _waterOriginTime};
    if (IsTemporal(_aaMode))
    {
        push.jitter[0] = _taaJitter[0];
        push.jitter[1] = _taaJitter[1];
    }
    vkCmdPushConstants(command, _waterLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
    vkCmdDraw(command, 3, 1, 0, 0);
    vkCmdEndRenderPass(command);
    if (frame.frameQueries)
    {
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 15);
        frame.waterOpticsTimestamped = true;
    }
    _waterRefracted = true;
    ++_waterComposites;
    _commands = {};
}

void VulkanContext::DestroyWaterResources() noexcept
{
    for (auto pipeline : _waterPipelines)
        if (pipeline)
            vkDestroyPipeline(_device, pipeline, nullptr);
    if (_waterLayout)
        vkDestroyPipelineLayout(_device, _waterLayout, nullptr);
    if (_waterPool)
        vkDestroyDescriptorPool(_device, _waterPool, nullptr);
    if (_waterSetLayout)
        vkDestroyDescriptorSetLayout(_device, _waterSetLayout, nullptr);
    for (auto& target : _waterBackground)
    {
        if (target.framebuffer)
            vkDestroyFramebuffer(_device, target.framebuffer, nullptr);
        if (target.color.view)
            vkDestroyImageView(_device, target.color.view, nullptr);
        if (target.color.image)
            vkDestroyImage(_device, target.color.image, nullptr);
        if (target.color.memory)
            vkFreeMemory(_device, target.color.memory, nullptr);
    }
    _waterPipelines = {};
    _waterLayout = VK_NULL_HANDLE;
    _waterPool = VK_NULL_HANDLE;
    _waterSetLayout = VK_NULL_HANDLE;
    _waterBackground.clear();
    _refractionEnabled = _waterCaptured = _waterRefracted = false;
    _shoreFoamEnabled = false;
}
} // namespace Poseidon::vk
