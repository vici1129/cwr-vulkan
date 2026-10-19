#include <PoseidonVK/VulkanContext.hpp>
#include <stdexcept>

namespace Poseidon::vk
{
namespace
{
void Require(VkResult result)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error("Vulkan weather resource allocation failed: " + std::to_string(result));
}
} // namespace
std::string VulkanContext::SetCloudShadows(std::string_view mode)
{
    if (mode == "status")
        return std::string("requested=") + (_cloudShadows ? "on" : "off") +
               " active=" + (_weatherLighting.sun[3] > 0 ? "on" : "off") +
               " coverage=" + std::to_string(_weatherLighting.cloudColor[3]) +
               " position=" + std::to_string(_weatherLighting.options[2]) + " fallback=" + _weatherFailure;
    if (mode != "on" && mode != "off")
        return "INVALID: triCloudShadows on, off, status";
    if (_cloudShadows != (mode == "on"))
    {
        _cloudShadows = mode == "on";
        ResetTemporalHistory();
    }
    return "OK: cloud shadows " + std::string(mode);
}
std::string VulkanContext::SetSkyReflections(std::string_view mode)
{
    if (mode == "status")
        return std::string("requested=") + (_skyReflections ? "on" : "off") +
               " active=" + (_weatherLighting.options[0] > 0 && RTReflectionsEnabled() ? "on" : "off") +
               " prerequisite=" +
               (RTReflectionsEnabled() ? "available" : "requires triRTReflections on and initialized ray-query support") +
               " position=" + std::to_string(_weatherLighting.options[2]) + " fallback=" + _weatherFailure;
    if (mode != "on" && mode != "off")
        return "INVALID: triSkyReflections on, off, status";
    if (_skyReflections != (mode == "on"))
    {
        _skyReflections = mode == "on";
        ResetTemporalHistory();
    }
    return "OK: sky reflections " + std::string(mode) +
           (_skyReflections && !RTReflectionsEnabled()
                ? "; inactive: requires triRTReflections on and initialized ray-query support"
                : "");
}
void VulkanContext::FailWeather(std::string_view reason)
{
    _weatherLighting = {};
    _weatherFailure = reason;
}
void VulkanContext::SetWeatherFrame(const WeatherLighting& lighting,
                                    const std::array<std::shared_ptr<TextureImage>, 4>& images)
{
    _weatherLighting = lighting;
    _weatherLighting.options[3] = _cloudsEnabled && !_cloudsTargets.empty() && images[0] && images[1] && images[2] ? 1.f : 0.f;
    _weatherLighting.sun[3] *= _cloudShadows ? 1.f : 0.f;
    _weatherLighting.options[0] *= _skyReflections && RTReflectionsEnabled() ? 1.f : 0.f;
    _weatherImages = images;
    _weatherFailure.clear();
}
void VulkanContext::CreateWeatherLayout()
{
    if (_weatherLayout)
        return;
    VkDescriptorSetLayoutBinding bindings[8]{};
    for (unsigned i = 0; i < 8; ++i)
        bindings[i] = {i, i ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                       VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout.bindingCount = 8;
    layout.pBindings = bindings;
    Require(vkCreateDescriptorSetLayout(_device, &layout, nullptr, &_weatherLayout));
}
VkDescriptorSet VulkanContext::PrepareWeather()
{
    CreateWeatherLayout();
    auto& slot = _weatherSlots[_frame * 8 + (_csmActive ? 1 : 0) + _localShadowMask * 2];
    if (slot.prepared)
        return slot.set;
    if (!slot.pool)
    {
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
                                           {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = 1;
        pool.poolSizeCount = 2;
        pool.pPoolSizes = sizes;
        Require(vkCreateDescriptorPool(_device, &pool, nullptr, &slot.pool));
        VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = slot.pool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &_weatherLayout;
        Require(vkAllocateDescriptorSets(_device, &alloc, &slot.set));
        Require(CreateHostVisibleBuffer(_physical, _device, sizeof(WeatherLighting), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        slot.uniform));
    }
    if (!_whiteTexture)
    {
        const uint32_t white = 0xffffffff;
        _whiteTexture = UploadTexture(1, 1, &white);
    }
    CreateShadowFallback();
    UploadMappedBuffer(slot.uniform, &_weatherLighting, sizeof(_weatherLighting));
    const VkDescriptorBufferInfo buffer{slot.uniform.buffer, 0, sizeof(WeatherLighting)};
    VkDescriptorImageInfo images[7]{};
    VkWriteDescriptorSet writes[8]{};
    for (unsigned i = 0; i < 8; ++i)
    {
        writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writes[i].dstSet = slot.set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = i ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        if (!i)
            writes[i].pBufferInfo = &buffer;
        else if (i < 5)
        {
            slot.images[i - 1] = _weatherImages[i - 1] ? _weatherImages[i - 1] : _whiteTexture;
            images[i - 1] = {_textureSamplers[i == 4 ? 2 : 3], slot.images[i - 1]->colorView,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes[i].pImageInfo = &images[i - 1];
        }
        else if (i == 5)
        {
            // Share set 3 with CSM: retain Vulkan's four-set minimum even when disabled.
            images[4] = {_csmActive ? _csmSampler : _textureSamplers[0],
                         _csmActive ? _frames[_frame].shadow.view : _csmFallbackView,
                         _csmActive ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                    : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes[i].pImageInfo = &images[4];
        }
        else
        {
            const bool active = (_localShadowMask & (1u << (i - 6))) != 0;
            images[i - 1] = {active ? _csmSampler : _textureSamplers[0],
                             active ? _frames[_frame].localShadow[i - 6].view : _csmFallbackView,
                             active ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                    : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes[i].pImageInfo = &images[i - 1];
        }
    }
    vkUpdateDescriptorSets(_device, 8, writes, 0, nullptr);
    slot.prepared = true;
    return slot.set;
}
void VulkanContext::DestroyWeather() noexcept
{
    for (auto& slot : _weatherSlots)
    {
        if (slot.pool)
            vkDestroyDescriptorPool(_device, slot.pool, nullptr);
        DestroyBuffer(_device, slot.uniform);
        slot = {};
    }
    _weatherImages = {};
    if (_weatherLayout)
        vkDestroyDescriptorSetLayout(_device, _weatherLayout, nullptr);
    _weatherLayout = VK_NULL_HANDLE;
}
} // namespace Poseidon::vk
