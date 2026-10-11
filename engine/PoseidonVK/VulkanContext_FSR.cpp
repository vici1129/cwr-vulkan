#include <PoseidonVK/VulkanContext.hpp>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <bit>
#ifdef CWR_FSR3
#include <FidelityFX/host/ffx_fsr3upscaler.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#endif

namespace Poseidon::vk
{
#ifdef CWR_FSR3
namespace
{
void Require(FfxErrorCode result, const char* operation)
{
    if (result != FFX_OK) throw std::runtime_error(std::string("FSR 3.1.4: ") + operation + " code=" + std::to_string(result));
}
void RequireVK(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS) throw std::runtime_error(std::string("FSR Vulkan: ") + operation + " code=" + std::to_string(result));
}
FfxFsr3UpscalerQualityMode Quality(AAMode mode)
{
    switch (mode)
    {
        case AAMode::FSRQuality: return FFX_FSR3UPSCALER_QUALITY_MODE_QUALITY;
        case AAMode::FSRBalanced: return FFX_FSR3UPSCALER_QUALITY_MODE_BALANCED;
        case AAMode::FSRPerformance: return FFX_FSR3UPSCALER_QUALITY_MODE_PERFORMANCE;
        default: return FFX_FSR3UPSCALER_QUALITY_MODE_NATIVEAA;
    }
}
FfxResource Resource(VkImage image, VkFormat format, VkExtent2D extent, bool storage, const wchar_t* name)
{
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D; info.format = format;
    info.extent = {extent.width, extent.height, 1}; info.arrayLayers = info.mipLevels = 1;
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | (storage ? VK_IMAGE_USAGE_STORAGE_BIT : 0);
    return ffxGetResourceVK(reinterpret_cast<void*>(image), ffxGetImageResourceDescriptionVK(image, info), name,
        storage ? FFX_RESOURCE_STATE_UNORDERED_ACCESS : FFX_RESOURCE_STATE_COMPUTE_READ);
}
void Barrier(VkCommandBuffer command, VkImage image, VkImageLayout from, VkImageLayout to,
             VkAccessFlags source, VkAccessFlags destination)
{
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.srcAccessMask = source; b.dstAccessMask = destination;
    b.oldLayout = from; b.newLayout = to;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image; b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &b);
}
}
struct VulkanContext::FSRState
{
    FfxFsr3UpscalerContext context{};
    std::vector<char> scratch;
    bool created = false, initialized = false;
    // Converted velocity, copied nonlinear depth, reactive and composition masks.
    std::array<DepthAttachment, 4> inputs{};
    std::array<DepthAttachment, 3> shared{};
    DepthAttachment output;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkDescriptorSet outputSet = VK_NULL_HANDLE;
    std::array<float, 2> jitter{};
    std::chrono::steady_clock::time_point lastDispatch{};
};
VkExtent2D VulkanContext::FSRRenderExtent() const
{
    uint32_t width = 0, height = 0;
    Require(ffxFsr3UpscalerGetRenderResolutionFromQualityMode(&width, &height, _extent.width, _extent.height, Quality(_aaMode)), "render resolution");
    return {uint32_t(width), uint32_t(height)};
}
void VulkanContext::CreateFSR()
{
    _fsr = std::make_shared<FSRState>();
    auto& s = *_fsr;
    s.scratch.resize(ffxGetScratchMemorySizeVK(_physical, FFX_FSR3UPSCALER_CONTEXT_COUNT));
    VkDeviceContext device{_device, _physical, vkGetDeviceProcAddr};
    FfxFsr3UpscalerContextDescription description{};
    description.maxRenderSize = {_worldExtent.width, _worldExtent.height};
    description.maxUpscaleSize = {_extent.width, _extent.height};
    // Scene-linear FP16, fixed unit exposure/preExposure, no auto exposure.
    // SDK null exposure defaults to 1; output stays HDR through reconstruction.
    description.flags = FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE |
        (_validationEnabled ? FFX_FSR3UPSCALER_ENABLE_DEBUG_CHECKING : 0);
    description.fpMessage = [](FfxMsgType type, const wchar_t* message) {
        std::fprintf(stderr, "FSR SDK message %d: %ls\n", int(type), message);
    };
    Require(ffxGetInterfaceVK(&description.backendInterface, ffxGetDeviceVK(&device), s.scratch.data(),
                              s.scratch.size(), FFX_FSR3UPSCALER_CONTEXT_COUNT), "Vulkan interface");
    Require(ffxFsr3UpscalerContextCreate(&s.context, &description), "context creation");
    s.created = true;
    constexpr VkFormat inputFormats[]{VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R8_UNORM, VK_FORMAT_R8_UNORM};
    VkAttachmentDescription attachments[4]{};
    VkAttachmentReference references[4]{};
    VkImageView views[4]{};
    for (unsigned i = 0; i < 4; ++i)
    {
        CreateAAImage(s.inputs[i], inputFormats[i], VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                       VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        attachments[i].format = inputFormats[i]; attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        references[i] = {i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; views[i] = s.inputs[i].view;
    }
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 4; subpass.pColorAttachments = references;
    VkSubpassDependency dependencies[2]{};
    dependencies[0] = {VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0};
    dependencies[1] = {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 0};
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass.attachmentCount = 4; pass.pAttachments = attachments;
    pass.subpassCount = 1; pass.pSubpasses = &subpass;
    pass.dependencyCount = 2; pass.pDependencies = dependencies;
    RequireVK(vkCreateRenderPass(_device, &pass, nullptr, &_fsrMaskPass), "mask pass");
    VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fb.renderPass = _fsrMaskPass; fb.attachmentCount = 4; fb.pAttachments = views;
    fb.width = _worldExtent.width; fb.height = _worldExtent.height; fb.layers = 1;
    RequireVK(vkCreateFramebuffer(_device, &fb, nullptr, &s.framebuffer), "input framebuffer");
    FfxFsr3UpscalerSharedResourceDescriptions shared{};
    Require(ffxFsr3UpscalerGetSharedResourceDescriptions(&s.context, &shared), "shared resource descriptions");
    const FfxCreateResourceDescription* resources[]{&shared.dilatedDepth, &shared.dilatedMotionVectors, &shared.reconstructedPrevNearestDepth};
    constexpr VkFormat sharedFormats[]{VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32_UINT};
    for (unsigned i = 0; i < 3; ++i)
        CreateAAImage(s.shared[i], sharedFormats[i], VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            {resources[i]->resourceDescription.width, resources[i]->resourceDescription.height});
    CreateAAImage(s.output, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                  VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, _extent);
    VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocation.descriptorPool = _aaPool; allocation.descriptorSetCount = 1; allocation.pSetLayouts = &_textureLayout;
    RequireVK(vkAllocateDescriptorSets(_device, &allocation, &s.outputSet), "output descriptor");
    VkDescriptorImageInfo image{_aaSampler, s.output.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = s.outputSet; write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; write.pImageInfo = &image;
    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
    std::fprintf(stderr, "FSR 3.1.4: context created, input %ux%u, output %ux%u\n", _worldExtent.width, _worldExtent.height, _extent.width, _extent.height);
}
void VulkanContext::DestroyFSR() noexcept
{
    if (!_fsr) return;
    auto& s = *_fsr;
    if (s.created) ffxFsr3UpscalerContextDestroy(&s.context);
    if (s.framebuffer) vkDestroyFramebuffer(_device, s.framebuffer, nullptr);
    if (_fsrMaskPass) vkDestroyRenderPass(_device, _fsrMaskPass, nullptr);
    _fsrMaskPass = VK_NULL_HANDLE;
    auto destroy = [&](DepthAttachment& image) {
        if (image.view) vkDestroyImageView(_device, image.view, nullptr);
        if (image.image) vkDestroyImage(_device, image.image, nullptr);
        if (image.memory) vkFreeMemory(_device, image.memory, nullptr);
    };
    for (auto& image : s.inputs) destroy(image);
    for (auto& image : s.shared) destroy(image);
    destroy(s.output);
    _fsr.reset();
}
void VulkanContext::JitterFSR()
{
    auto& s = *_fsr;
    const auto previous = _taaJitter;
    const int phases = ffxFsr3UpscalerGetJitterPhaseCount(_worldExtent.width, _extent.width);
    Require(ffxFsr3UpscalerGetJitterOffset(&s.jitter[0], &s.jitter[1], ++_taaFrame, phases), "jitter");
    // Positive-height Vulkan viewport: projection +X/+Y displaces right/down.
    _taaJitter = {2 * s.jitter[0] / _worldExtent.width, 2 * s.jitter[1] / _worldExtent.height};
    _fsrJitterDelta = {(previous[0] - _taaJitter[0]) * .5f, (previous[1] - _taaJitter[1]) * .5f};
}
VkDescriptorSet VulkanContext::DispatchFSR(const std::array<float, 4>& projection)
{
    auto& s = *_fsr;
    auto& frame = _frames[_frame];
    auto command = frame.command;
    const auto& world = _aaTargets[_image];
    // SSAO and world color writes were graphics work. Make them visible to the
    // SDK's compute stages (the original AA dependencies target fragment reads).
    Barrier(command, world.color.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
    DrawAAPass(6, s.framebuffer, world.motionSet, _worldExtent);
    if (!s.initialized)
        for (auto& image : s.shared)
            Barrier(command, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0,
                    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
    Barrier(command, s.output.image, s.initialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL, s.initialized ? VK_ACCESS_SHADER_READ_BIT : 0, VK_ACCESS_SHADER_WRITE_BIT);
    FfxFsr3UpscalerDispatchDescription dispatch{};
    dispatch.commandList = ffxGetCommandListVK(command);
    dispatch.color = Resource(world.color.image, _aaColorFormat, _worldExtent, false, L"CWR world color");
    dispatch.motionVectors = Resource(s.inputs[0].image, VK_FORMAT_R16G16_SFLOAT, _worldExtent, false, L"CWR motion");
    dispatch.depth = Resource(s.inputs[1].image, VK_FORMAT_R32_SFLOAT, _worldExtent, false, L"CWR nonlinear depth");
    dispatch.reactive = Resource(s.inputs[2].image, VK_FORMAT_R8_UNORM, _worldExtent, false, L"CWR reactive");
    dispatch.transparencyAndComposition = Resource(s.inputs[3].image, VK_FORMAT_R8_UNORM, _worldExtent, false, L"CWR composition");
    dispatch.dilatedDepth = Resource(s.shared[0].image, VK_FORMAT_R32_SFLOAT, _worldExtent, true, L"FSR dilated depth");
    dispatch.dilatedMotionVectors = Resource(s.shared[1].image, VK_FORMAT_R16G16_SFLOAT, _worldExtent, true, L"FSR dilated motion");
    dispatch.reconstructedPrevNearestDepth = Resource(s.shared[2].image, VK_FORMAT_R32_UINT, _worldExtent, true, L"FSR previous depth");
    dispatch.output = Resource(s.output.image, VK_FORMAT_R16G16B16A16_SFLOAT, _extent, true, L"FSR native output");
    dispatch.jitterOffset = {s.jitter[0], s.jitter[1]};
    dispatch.motionVectorScale = {float(_worldExtent.width), float(_worldExtent.height)};
    dispatch.renderSize = {_worldExtent.width, _worldExtent.height};
    dispatch.upscaleSize = {_extent.width, _extent.height};
    dispatch.preExposure = 1;
    dispatch.reset = !_taaValid;
    dispatch.cameraNear = -projection[3] / projection[2];
    dispatch.cameraFar = -projection[3] / (projection[2] - 1.f);
    dispatch.cameraFovAngleVertical = 2.f * std::atan(1.f / std::abs(projection[1]));
    dispatch.viewSpaceToMetersFactor = 1;
    const auto now = std::chrono::steady_clock::now();
    dispatch.frameTimeDelta = dispatch.reset || s.lastDispatch.time_since_epoch().count() == 0 ? 16.667f :
        std::chrono::duration<float, std::milli>(now - s.lastDispatch).count();
    s.lastDispatch = now;
    if (frame.frameQueries) vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 2);
    const auto result = ffxFsr3UpscalerContextDispatch(&s.context, &dispatch);
    if (frame.frameQueries)
    {
        vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.frameQueries, 3);
        frame.taaTimestamped = true;
    }
    if (result != FFX_OK)
    {
        _fsrAvailable = false;
        _fsrReason = "dispatch failed, code=" + std::to_string(result);
        std::fprintf(stderr, "FSR 3.1.4: %s; returning to Off at the next idle boundary\n", _fsrReason.c_str());
        _requestedAA = AAMode::Off;
        _recreate = true;
        // Do not present uninitialized reconstruction or repeatedly dispatch a
        // failed context. One clear frame precedes ordinary rendering recovery.
        Barrier(command, s.output.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
        VkClearColorValue clear{};
        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(command, s.output.image, VK_IMAGE_LAYOUT_GENERAL, &clear, 1, &range);
    }
    else
    {
        ++_fsrDispatches;
        if (dispatch.reset) ++_fsrResets;
    }
    Barrier(command, s.output.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
    s.initialized = true;
    _taaValid = result == FFX_OK;
    _commands = {};
    return s.outputSet;
}
#else
VkExtent2D VulkanContext::FSRRenderExtent() const { return _extent; }
void VulkanContext::CreateFSR() { throw std::logic_error("FSR was not compiled"); }
void VulkanContext::DestroyFSR() noexcept {}
void VulkanContext::JitterFSR() {}
VkDescriptorSet VulkanContext::DispatchFSR(const std::array<float, 4>&) { return VK_NULL_HANDLE; }
#endif
}
