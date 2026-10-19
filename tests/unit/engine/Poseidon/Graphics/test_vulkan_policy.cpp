// Deliberately standalone: fake Vulkan handles, no SDL initialization or driver.
#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/TriangleVK.hpp>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <limits>
#include <cstdlib>
#include <optional>

namespace
{
int checks = 0;
std::vector<const char*> availableInstanceExtensions;
std::vector<std::string> requestedInstanceExtensions;
std::vector<std::string> requestedInstanceLayers;
bool validationLayerAvailable = false;
unsigned instanceCreationCalls = 0, layerEnumerationCalls = 0;
uint32_t requestedApiVersion = 0;
VkInstanceCreateFlags requestedInstanceFlags = 0;
PFN_vkDebugUtilsMessengerCallbackEXT instanceCallback = nullptr;
void* instanceCallbackUser = nullptr;
bool injectTeardownError = false;
uint32_t loaderVersion = VK_API_VERSION_1_0;
bool startupRT = true, startupFSR = true;
unsigned rejectedDeviceFeatures = 0;
std::vector<unsigned> deviceAttempts;
struct EnvironmentOverride
{
    const char* name;
    std::optional<std::string> previous;
    EnvironmentOverride(const char* key, const char* value) : name(key)
    {
        if (const char* old = std::getenv(key)) previous = old;
        Set(value);
    }
    void Set(const char* value)
    {
#ifdef _WIN32
        _putenv_s(name, value ? value : "");
#else
        if (value) setenv(name, value, 1); else unsetenv(name);
#endif
    }
    ~EnvironmentOverride() { Set(previous ? previous->c_str() : nullptr); }
};

VKAPI_ATTR VkResult VKAPI_CALL EnumerateVersion(uint32_t* version)
{
    *version = loaderVersion;
    return VK_SUCCESS;
}

template <class Handle>
Handle DebugMessengerHandle()
{
    if constexpr (std::is_pointer_v<Handle>)
        return reinterpret_cast<Handle>(uintptr_t{2});
    else
        return static_cast<Handle>(2);
}
VKAPI_ATTR VkResult VKAPI_CALL CreateDebugMessenger(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*,
                                                    const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT* out)
{
    *out = DebugMessengerHandle<VkDebugUtilsMessengerEXT>();
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL DestroyDebugMessenger(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*) {}
void Check(bool condition, const char* message)
{
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}

template <class Exception, class Action>
void CheckThrows(Action action, const char* message)
{
    ++checks;
    try
    {
        action();
    }
    catch (const Exception&)
    {
        return;
    }
    throw std::runtime_error(message);
}
} // namespace

// Replace only instance entry points: exercise real extension selection without
// creating a driver instance or requiring any particular machine's extensions.
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t* count, VkLayerProperties* properties)
{
    ++layerEnumerationCalls;
    if (properties && validationLayerAvailable)
    {
        if (!*count)
            return VK_INCOMPLETE;
        properties[0] = {};
        std::memcpy(properties[0].layerName, "VK_LAYER_KHRONOS_validation", sizeof("VK_LAYER_KHRONOS_validation"));
    }
    *count = validationLayerAvailable ? 1 : 0;
    return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char*, uint32_t* count,
                                                                      VkExtensionProperties* properties)
{
    if (!properties)
    {
        *count = static_cast<uint32_t>(availableInstanceExtensions.size());
        return VK_SUCCESS;
    }
    const auto copied = std::min(*count, static_cast<uint32_t>(availableInstanceExtensions.size()));
    for (uint32_t i = 0; i < copied; ++i)
    {
        properties[i] = {};
        std::memcpy(properties[i].extensionName, availableInstanceExtensions[i],
                    std::strlen(availableInstanceExtensions[i]) + 1);
    }
    *count = copied;
    return copied == availableInstanceExtensions.size() ? VK_SUCCESS : VK_INCOMPLETE;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* info, const VkAllocationCallbacks*,
                                                VkInstance* instance)
{
    ++instanceCreationCalls;
    requestedInstanceLayers.clear();
    for (uint32_t i = 0; i < info->enabledLayerCount; ++i)
        requestedInstanceLayers.emplace_back(info->ppEnabledLayerNames[i]);
    requestedInstanceExtensions.assign(info->ppEnabledExtensionNames,
                                       info->ppEnabledExtensionNames + info->enabledExtensionCount);
    requestedApiVersion = info->pApplicationInfo->apiVersion;
    requestedInstanceFlags = info->flags;
    instanceCallback = nullptr;
    instanceCallbackUser = nullptr;
    for (auto* next = static_cast<const VkBaseInStructure*>(info->pNext); next; next = next->pNext)
        if (next->sType == VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT)
        {
            const auto* debug = reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(next);
            instanceCallback = debug->pfnUserCallback;
            instanceCallbackUser = debug->pUserData;
        }
    *instance = reinterpret_cast<VkInstance>(uintptr_t{1});
    return VK_SUCCESS;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance, const char* name)
{
    if (std::strcmp(name, "vkEnumerateInstanceVersion") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(&EnumerateVersion);
    if (std::strcmp(name, "vkGetPhysicalDeviceFeatures2") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(&vkGetPhysicalDeviceFeatures2);
    if (std::strcmp(name, "vkCreateDebugUtilsMessengerEXT") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(&CreateDebugMessenger);
    if (std::strcmp(name, "vkDestroyDebugUtilsMessengerEXT") == 0)
        return reinterpret_cast<PFN_vkVoidFunction>(&DestroyDebugMessenger);
    return nullptr;
}
// A fake presentation device exercises the real startup negotiation. Stop at
// the first frame-resource allocation so these tests never call a real driver.
VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance, uint32_t* count, VkPhysicalDevice* out)
{
    if (out) *out = reinterpret_cast<VkPhysicalDevice>(uintptr_t{1});
    *count = 1;
    return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice, const char*, uint32_t* count, VkExtensionProperties* out)
{
    std::vector<const char*> names{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (startupRT)
        for (auto name : {VK_KHR_RAY_QUERY_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                          VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME}) names.push_back(name);
    if (out)
        for (size_t i = 0; i < names.size(); ++i)
        {
            out[i] = {};
            std::strcpy(out[i].extensionName, names[i]);
        }
    *count = uint32_t(names.size());
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice, uint32_t* count, VkQueueFamilyProperties* out)
{
    if (out) { *out = {}; out->queueCount = 1; out->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT; }
    *count = 1;
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32* out)
{ *out = VK_TRUE; return VK_SUCCESS; }
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR* out)
{ *out = {}; out->supportedUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; return VK_SUCCESS; }
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice, VkSurfaceKHR, uint32_t* count, VkSurfaceFormatKHR* out)
{ if (out) *out = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}; *count = 1; return VK_SUCCESS; }
VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice, VkSurfaceKHR, uint32_t* count, VkPresentModeKHR* out)
{ if (out) *out = VK_PRESENT_MODE_FIFO_KHR; *count = 1; return VK_SUCCESS; }
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice, VkPhysicalDeviceProperties* out)
{
    *out = {};
    out->apiVersion = loaderVersion;
    out->limits.maxColorAttachments = 4;
    std::strcpy(out->deviceName, "Startup test device");
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(VkPhysicalDevice, VkPhysicalDeviceFeatures* out)
{
    *out = {};
    out->independentBlend = VK_TRUE;
    out->shaderInt64 = startupRT;
    out->shaderStorageImageExtendedFormats = out->shaderStorageImageWriteWithoutFormat = startupFSR;
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physical, VkPhysicalDeviceFeatures2* out)
{
    vkGetPhysicalDeviceFeatures(physical, &out->features);
    for (auto* next = static_cast<VkBaseOutStructure*>(out->pNext); next; next = next->pNext)
    {
        switch (next->sType)
        {
        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR:
            reinterpret_cast<VkPhysicalDeviceRayQueryFeaturesKHR*>(next)->rayQuery = startupRT; break;
        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR:
            reinterpret_cast<VkPhysicalDeviceAccelerationStructureFeaturesKHR*>(next)->accelerationStructure = startupRT; break;
        case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES:
            reinterpret_cast<VkPhysicalDeviceBufferDeviceAddressFeatures*>(next)->bufferDeviceAddress = startupRT; break;
        default: break;
        }
    }
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(VkPhysicalDevice physical, VkPhysicalDeviceProperties2* out)
{
    vkGetPhysicalDeviceProperties(physical, &out->properties);
    if (out->pNext)
    {
        auto* subgroup = static_cast<VkPhysicalDeviceSubgroupProperties*>(out->pNext);
        subgroup->supportedStages = VK_SHADER_STAGE_COMPUTE_BIT;
        subgroup->supportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_ARITHMETIC_BIT | VK_SUBGROUP_FEATURE_QUAD_BIT;
    }
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice, VkFormat, VkFormatProperties* out)
{
    *out = {};
    out->bufferFeatures = VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR;
    out->optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice, const VkDeviceCreateInfo* info, const VkAllocationCallbacks*, VkDevice* out)
{
    unsigned mask = info->pEnabledFeatures->shaderStorageImageExtendedFormats ? 2 : 0;
    bool address = false, address12 = false, hasVulkan12 = false;
    for (auto* next = static_cast<const VkBaseInStructure*>(info->pNext); next; next = next->pNext)
    {
        if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR) mask |= 1;
        if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES) address = true;
        if (next->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES)
        {
            hasVulkan12 = true;
            address12 = reinterpret_cast<const VkPhysicalDeviceVulkan12Features*>(next)->bufferDeviceAddress;
        }
    }
    Check(!(address && hasVulkan12), "combined RT/FSR device must not duplicate buffer-device-address feature structures");
    Check((mask & 1) == 0 || (address || address12), "RT retry retains device address support");
    Check(bool(info->pEnabledFeatures->shaderInt64) == bool(mask & 1), "raster retries remove RT features");
    deviceAttempts.push_back(mask);
    *out = VK_NULL_HANDLE;
    if (mask & rejectedDeviceFeatures) return VK_ERROR_FEATURE_NOT_PRESENT;
    *out = reinterpret_cast<VkDevice>(uintptr_t{1});
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice, uint32_t, uint32_t, VkQueue* out) { *out = VK_NULL_HANDLE; }
VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice, const VkCommandPoolCreateInfo*, const VkAllocationCallbacks*, VkCommandPool*)
{ return VK_ERROR_OUT_OF_HOST_MEMORY; }
VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice) { return VK_SUCCESS; }
VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice, const VkAllocationCallbacks*) {}
VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks*) {}
VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance, const VkAllocationCallbacks*)
{
    // The instance pNext callback remains valid during vkDestroyInstance, after
    // the persistent debug messenger has already been destroyed.
    if (injectTeardownError && instanceCallback)
    {
        VkDebugUtilsMessengerCallbackDataEXT data{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT};
        data.pMessageIdName = "TEST-teardown-only";
        data.pMessage = "Intentional driver-free destruction error";
        instanceCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                         &data, instanceCallbackUser);
    }
}
int TestVulkanBuffers();
int TestVulkanCommands();

int main()
{
    using namespace Poseidon::vk;
    try
    {
        EnvironmentOverride rtEnvironment("CWR_VK_RTAO", nullptr), fsrEnvironment("CWR_VK_FSR", nullptr);
        Check(Poseidon::vk::TextureAnisotropy(false, 16) == 1, "unsupported anisotropy falls back");
        Check(Poseidon::vk::TextureAnisotropy(true, 8) == 8, "anisotropy respects device limit");
        Check(Poseidon::vk::TextureAnisotropy(true, 32) == 16, "anisotropy matches GL33 cap");
        for (float anisotropy : {1.f, 8.f, 16.f})
            for (unsigned i = 0; i < 8; ++i)
            {
                const auto sampler = Poseidon::vk::TextureSamplerInfo(i, anisotropy);
                const bool point = (i & 4) != 0;
                Check(sampler.minFilter == (point ? VK_FILTER_NEAREST : VK_FILTER_LINEAR) &&
                          sampler.magFilter == sampler.minFilter, "min/mag filtering");
                Check(sampler.mipmapMode == (point ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR),
                      "mip filtering");
                Check(bool(sampler.anisotropyEnable) == (!point && anisotropy > 1), "point/fallback anisotropy disabled");
                Check(sampler.maxAnisotropy == (sampler.anisotropyEnable ? anisotropy : 1), "sampler anisotropy limit");
                Check(sampler.addressModeU == ((i & 1) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT) &&
                          sampler.addressModeV == ((i & 2) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT) &&
                          sampler.addressModeW == VK_SAMPLER_ADDRESS_MODE_REPEAT, "clamp/repeat preserved");
                Check(sampler.mipLodBias == 0 && sampler.minLod == 0 && sampler.maxLod == VK_LOD_CLAMP_NONE,
                      "no artificial LOD bias or mip clamp");
            }
        checks += TestVulkanBuffers();
        const std::array<TextureMip, 7> square{{{128,128,nullptr}, {64,64,nullptr}, {32,32,nullptr},
                                               {16,16,nullptr}, {8,8,nullptr}, {4,4,nullptr}, {2,2,nullptr}}};
        Check(TextureSampledMipCount(square) == 5, "stock detail chain matches GL33 8x8 tail");
        auto rectangle = square;
        for (auto& mip : rectangle) mip.height = std::max(1u, mip.height / 4);
        Check(TextureSampledMipCount(rectangle) == 3, "rectangular chain stops on either dimension");
        Check(TextureSampledMipCount(std::span(square).first(3)) == 3, "short stored chains remain short");
        Check(TextureSampledMipCount(std::span(square).last(1)) == 1, "tiny UI base remains accessible");
        Check(TextureSampledMipCount({}) == 0, "empty chain does not invent levels");
        checks += TestVulkanCommands();
        Check(sizeof(TriangleVertex) == 20 && sizeof(TriangleIndices[0]) == 2,
              "diagnostic vertex/index formats must match the pipeline");
        Check(TriangleIndices == std::array<uint16_t, 3>{2, 0, 1}, "diagnostic must exercise non-sequential indexing");
        for (auto index : TriangleIndices)
            Check(index < TriangleVertices.size(), "every index must address a real vertex");
        VkSurfaceCapabilitiesKHR caps{};
        caps.currentExtent = {1920, 1080};
        auto extent = ChooseExtent(caps, 640, 480);
        Check(extent.width == 1920 && extent.height == 1080, "fixed surface extent must win over requested size");
        caps.currentExtent = {UINT32_MAX, UINT32_MAX};
        caps.minImageExtent = {64, 64};
        caps.maxImageExtent = {4096, 2160};
        extent = ChooseExtent(caps, 1, 5000);
        Check(extent.width == 64 && extent.height == 2160, "variable extent must clamp both dimensions");
        extent = ChooseExtent(caps, 800, 600);
        Check(extent.width == 800 && extent.height == 600, "valid variable extent must survive");
        caps.currentExtent = {0, 0};
        extent = ChooseExtent(caps, 800, 600);
        Check(!extent.width && !extent.height, "zero surface extent must suspend creation");
        caps.minImageCount = 2;
        caps.maxImageCount = 0;
        Check(ChooseImageCount(caps) == 3, "zero maximum means unbounded image count");
        caps.maxImageCount = 2;
        Check(ChooseImageCount(caps) == 2, "bounded image count must honor surface maximum");
        caps.minImageCount = UINT32_MAX;
        caps.maxImageCount = 0;
        Check(ChooseImageCount(caps) == UINT32_MAX, "image count must not overflow");

        const VkSurfaceFormatKHR fallback{VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        const VkSurfaceFormatKHR preferred{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        Check(ChooseFormat({fallback, preferred}).format == preferred.format, "prefer BGRA UNORM surface");
        Check(ChooseFormat({fallback}).format == fallback.format, "respect a supported non-preferred format");
        Check(ChooseFormat({{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}}).format == preferred.format,
              "undefined singleton permits choosing a concrete format");
        CheckThrows<std::runtime_error>([] { ChooseFormat({}); }, "empty format list must fail");
        const VkSurfaceFormatKHR hdr{VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT};
        const VkSurfaceFormatKHR wrongHDR{VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_HDR10_ST2084_EXT};
        Check(IsHDR10(ChooseFormat({preferred, hdr}, true)), "HDR explicitly selects 10-bit PQ");
        Check(!IsHDR10(ChooseFormat({hdr, preferred})), "HDR is opt-in even when listed first");
        Check(!IsHDR10(ChooseFormat({preferred}, true)), "HDR request falls back to SDR");
        Check(!IsHDR10(ChooseFormat({wrongHDR, preferred}, true)), "FP16 alone is not HDR10");
        CheckThrows<std::runtime_error>([&] { ChooseFormat({hdr}); }, "never send SDR values to a PQ-only surface");
        {
            VulkanContext hdrContext;
            Check(hdrContext.SetVolumetricFog("status").find("requested=off active=off") != std::string::npos,
                  "volumetric fog defaults off");
            Check(hdrContext.SetVolumetricFog("on").starts_with("OK") && !hdrContext.VolumetricFogEnabled(),
                  "volumetrics wait for the idle resource boundary");
            Check(hdrContext.SetVolumetricFog("invalid").starts_with("INVALID") &&
                  hdrContext.SetVolumetricFog("status").find("requested=on") != std::string::npos,
                  "invalid volumetric request preserves selection");
            Check(hdrContext.SetVolumetricFog("off").starts_with("OK"), "disable volumetric fog");
            Check(hdrContext.SetContactShadows("status").find("requested=off active=off") != std::string::npos,
                  "contact shadows default off without an RT capability request");
            Check(hdrContext.SetContactShadows("on").starts_with("OK") && !hdrContext.ContactShadowsEnabled(),
                  "contact enable waits for idle recreation");
            Check(hdrContext.SetContactShadows("invalid").starts_with("INVALID") &&
                  hdrContext.SetContactShadows("status").find("requested=on") != std::string::npos,
                  "invalid contact command preserves selection");
            Check(!hdrContext.RaySceneEnabled(), "contact does not request a ray tracing scene");
            Check(hdrContext.SetContactShadows("off").starts_with("OK"), "disable contact shadows");
            Check(!hdrContext.SoftShadowsEnabled(), "soft CSM defaults off");
            Check(!hdrContext.LocalShadowsEnabled(), "local shadows default off");
            Check(hdrContext.SetLocalShadows("on").starts_with("OK") && hdrContext.LocalShadowsEnabled(),
                  "local shadows enable without RT or sunlight");
            Check(hdrContext.SetLocalShadows("invalid").starts_with("INVALID") && hdrContext.LocalShadowsEnabled(),
                  "invalid local-shadow control preserves state");
            Check(!hdrContext.RaySceneEnabled() && !hdrContext.SoftShadowsEnabled(), "local shadows are independent");
            Check(hdrContext.SetLocalShadows("status").find("active=0") != std::string::npos,
                  "no local depth pass is fabricated by enabling shadows");
            Check(hdrContext.SetLocalShadows("off").starts_with("OK") && !hdrContext.LocalShadowsEnabled(),
                  "local shadows can be disabled");
            Check(hdrContext.SetSoftShadows("on").starts_with("OK") && hdrContext.SoftShadowsEnabled(),
                  "soft CSM does not require hardware RT");
            Check(!hdrContext.SoftShadowsEligible(), "soft CSM skips absent sunlight");
            ShapeLighting softSource{};
            softSource.atmosphereSun[3] = .5f;
            softSource.atmosphereColor = {1,1,1,0};
            hdrContext.SetVolumetricFrame(softSource);
            Check(hdrContext.SoftShadowsEligible() && hdrContext.SeparateSunlight(),
                  "soft sunlight separation works without a contact or RT target");
            softSource.atmosphereColor = {};
            hdrContext.SetVolumetricFrame(softSource);
            Check(!hdrContext.SoftShadowsEligible(), "soft CSM skips zero sun radiance");
            hdrContext.SetVolumetricFrame({});
            Check(hdrContext.SetSoftShadows("invalid").starts_with("INVALID") && hdrContext.SoftShadowsEnabled(),
                  "invalid soft CSM selection preserves state");
            Check(!hdrContext.RaySceneEnabled() && !hdrContext.ContactShadowsEnabled(),
                  "soft CSM does not enable ray tracing or contact shadows");
            Check(hdrContext.SetSoftShadows("off").starts_with("OK") && !hdrContext.SoftShadowsEnabled(),
                  "soft CSM can return to original filtering");
            ShapeLighting volume{};
            volume.fogParams = {100, 1000, 0, 0};
            Check(!VulkanContext::VolumetricSourceEligible(volume), "zero source needs no volume CSM");
            volume.atmosphereSun[3] = .0009f;
            Check(!VulkanContext::VolumetricSourceEligible(volume), "negligible source needs no volume CSM");
            volume.atmosphereSun[3] = .001f;
            Check(VulkanContext::VolumetricSourceEligible(volume), "draw and CSM share the source threshold");
            hdrContext.SetVolumetricFrame(volume);
            Check(!hdrContext.VolumetricScatteringEligible(), "source cannot enable an off effect");
            volume.fogParams[1] = 100;
            Check(!VulkanContext::VolumetricSourceEligible(volume), "empty fog range needs no volume CSM");
            volume.fogParams[1] = 99;
            Check(!VulkanContext::VolumetricSourceEligible(volume), "reversed fog range needs no volume CSM");
            const auto ultraTerrain = ShadowTerrainKey(440, 405, 8.f, 50, 6.25f);
            const auto extremeTerrain = ShadowTerrainKey(440, 405, 8.f, 50, 3.125f);
            Check(ultraTerrain != extremeTerrain,
                  "stationary Ultra/Extreme terrain changes invalidate the volume shadow mesh");
            Check(ultraTerrain == ShadowTerrainKey(440, 405, 8.f, 50, 6.25f),
                  "unchanged terrain subdivision reuses the volume shadow mesh");
            Check(ultraTerrain != ShadowTerrainKey(441, 405, 8.f, 50, 6.25f) &&
                      ultraTerrain != ShadowTerrainKey(440, 406, 8.f, 50, 6.25f) &&
                      ultraTerrain != ShadowTerrainKey(440, 405, 8.f, 51, 6.25f) &&
                      ultraTerrain != ShadowTerrainKey(440, 405, 12.5f, 50, 6.25f),
                  "volume shadow cache still tracks cell, extent and sampling grid");
            // Matrix translation must reproduce per-vertex camera subtraction,
            // including nonidentity rotations and the homogeneous row.
            const std::array<float, 16> matrix{1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
            for (const std::array<float, 3> offset : {std::array<float, 3>{0,0,0}, {7,-90,-5}, {-8,-120,8}})
            {
                const auto translated = TranslateShadowMatrix(matrix.data(), offset);
                for (int row = 0; row < 4; ++row)
                {
                    const float x = 112, y = 35, z = -48;
                    const float original = matrix[row]*(x+offset[0]) + matrix[4+row]*(y+offset[1]) +
                                           matrix[8+row]*(z+offset[2]) + matrix[12+row];
                    const float cached = translated[row]*x + translated[4+row]*y + translated[8+row]*z + translated[12+row];
                    Check(original == cached, "cached terrain transform matches camera-relative vertices");
                }
            }
            Check(!hdrContext.AtmosphereEnabled(), "atmosphere defaults off");
            Check(hdrContext.SetAtmosphere("on").starts_with("OK") && hdrContext.AtmosphereEnabled(),
                  "enable atmospheric perspective independently");
            Check(hdrContext.SetAtmosphere("invalid").starts_with("INVALID") && hdrContext.AtmosphereEnabled(),
                  "invalid atmosphere request preserves state");
            Check(hdrContext.SetAtmosphere("status").find("enabled=on") != std::string::npos,
                  "atmosphere status reports enablement");
            Check(hdrContext.SetAtmosphere("off").starts_with("OK") && !hdrContext.AtmosphereEnabled(),
                  "disable atmospheric perspective");
            Check(hdrContext.SetSunShafts("status").find("requested=off active=off") != std::string::npos,
                  "shafts default off without targets");
            Check(hdrContext.SetSunShafts("on").starts_with("OK"), "request shafts");
            Check(hdrContext.SetSunShafts("status").find("requested=on active=off") != std::string::npos,
                  "shaft request does not claim allocated resources");
            Check(hdrContext.SetSunShafts("strength 1").starts_with("INVALID"), "shafts have no tuning interface");
            Check(!hdrContext.AtmosphereEnabled(), "shafts do not force atmosphere on");
            Check(hdrContext.SetSunShafts("off").starts_with("OK"), "disable shafts");
            Check(hdrContext.SetBloom("status").find("requested=off active=off") != std::string::npos,
                  "bloom defaults off without allocated targets");
            for (const auto value : {"strength nan", "strength inf", "strength -1", "strength .31", "threshold 0", "threshold 33", "threshold 4junk", "unknown"})
                Check(hdrContext.SetBloom(value).starts_with("INVALID"), "reject invalid bloom tuning");
            Check(hdrContext.SetBloom("threshold 4").starts_with("OK"), "accept linear bloom threshold");
            Check(hdrContext.SetBloom("strength 0").starts_with("OK"), "accept zero contribution");
            Check(hdrContext.SetBloom("on").starts_with("OK"), "request bloom enable");
            Check(hdrContext.SetBloom("status").find("requested=on active=off") != std::string::npos,
                  "request alone must not claim active bloom resources");
            Check(hdrContext.SetBloom("off").starts_with("OK"), "request bloom disable");
            Check(hdrContext.SetHDR("peak nan").starts_with("INVALID"), "reject nonfinite peak");
            Check(hdrContext.SetHDR("white 600").starts_with("INVALID"), "bound reference white");
            Check(hdrContext.SetHDR("peak 100").starts_with("INVALID"), "peak must cover white");
            Check(hdrContext.SetHDR("peak 2000").starts_with("OK"), "accept TV peak");
            Check(hdrContext.SetHDR("white 200").starts_with("OK"), "accept reference white");
            Check(hdrContext.SetHDR("on").starts_with("OK"), "accept HDR request");
            Check(hdrContext.SetHDR("status").find("active=SDR-sRGB") != std::string::npos,
                  "requested HDR cannot claim negotiated HDR");
            Check(hdrContext.SetHDR("status").find("physicalDisplay=unverified") != std::string::npos,
                  "status never equates swapchain with verified physical output");
        }
        Check(ChoosePresentMode({VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR}, 1) == VK_PRESENT_MODE_FIFO_KHR,
              "vsync default must be FIFO");
        Check(ChoosePresentMode({VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR}, 0) ==
                  VK_PRESENT_MODE_MAILBOX_KHR,
              "off prefers mailbox when available");
        Check(ChoosePresentMode({VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR}, 0) ==
                  VK_PRESENT_MODE_IMMEDIATE_KHR,
              "off can use immediate mode");
        Check(ChoosePresentMode({VK_PRESENT_MODE_FIFO_KHR}, 0) == VK_PRESENT_MODE_FIFO_KHR,
              "off must fall back on FIFO-only surfaces");
        Check(ChoosePresentMode({VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR}, -1) ==
                  VK_PRESENT_MODE_FIFO_RELAXED_KHR,
              "adaptive prefers relaxed FIFO");
        Check(ChoosePresentMode({VK_PRESENT_MODE_FIFO_KHR}, -1) == VK_PRESENT_MODE_FIFO_KHR,
              "unsupported adaptive must fall back to FIFO");
        Check(ChooseCompositeAlpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ==
                  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
              "prefer opaque composition");
        Check(ChooseCompositeAlpha(VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) == VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
              "composition must respect the surface mask");
        CheckThrows<std::runtime_error>([] { ChooseCompositeAlpha(0); }, "empty alpha mask must fail");

        std::vector<VkQueueFamilyProperties> families(3);
        families[0].queueCount = families[1].queueCount = families[2].queueCount = 1;
        families[0].queueFlags = families[2].queueFlags = VK_QUEUE_GRAPHICS_BIT;
        auto queues = ChooseQueues(families, {VK_FALSE, VK_TRUE, VK_TRUE});
        Check(queues.Complete() && queues.graphics == 2 && queues.present == 2,
              "combined family must beat earlier split families");
        queues = ChooseQueues(families, {VK_FALSE, VK_TRUE, VK_FALSE});
        Check(queues.Complete() && queues.graphics == 0 && queues.present == 1,
              "separate graphics/present queues must be supported");
        families[2].queueCount = 0;
        queues = ChooseQueues(families, {VK_FALSE, VK_FALSE, VK_TRUE});
        Check(!queues.Complete(), "zero-queue family cannot present");
        Check(!ChooseQueues({}, {}).Complete(), "empty queue inventory must fail");
        Check(!ChooseQueues(families, {}).Complete(), "missing surface-support inventory must fail");
        Check(NeedsRecreation(VK_ERROR_OUT_OF_DATE_KHR), "out-of-date requires recreation");
        Check(NeedsRecreation(VK_SUBOPTIMAL_KHR), "suboptimal requires recreation");
        Check(!NeedsRecreation(VK_ERROR_DEVICE_LOST), "device loss is terminal, not a resize");
        Check(!NeedsRecreation(VK_SUCCESS), "successful present must not force recreation");

        const char* surfaceExtensions[] = {VK_KHR_SURFACE_EXTENSION_NAME};
        VulkanContext instanceContext;
        availableInstanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME};
        instanceContext.CreateInstance(surfaceExtensions, 1);
        Check(requestedInstanceLayers.empty() && !layerEnumerationCalls,
              "without a validation request, a missing layer must not affect instance creation");
        Check(requestedInstanceExtensions == std::vector<std::string>{VK_KHR_SURFACE_EXTENSION_NAME},
              "native Vulkan 1.0 must work without optional properties2 support");
        Check(requestedApiVersion == VK_API_VERSION_1_0 && requestedInstanceFlags == 0,
              "native instance must retain the Vulkan 1.0 baseline and flags");
        instanceContext.Shutdown();

        loaderVersion = VK_API_VERSION_1_2;
        auto startup = [&](std::vector<unsigned> expected)
        {
            VulkanContext context;
            context.CreateInstance(surfaceExtensions, 1);
            deviceAttempts.clear();
            bool reachedFrames = false;
            try { context.CreateDevice(reinterpret_cast<VkSurfaceKHR>(uintptr_t{1})); }
            catch (const std::runtime_error& error)
            { reachedFrames = std::string(error.what()).find("create command pool failed") != std::string::npos; }
            Check(reachedFrames, "optional negotiation must reach ordinary frame initialization");
            Check(deviceAttempts == expected, "startup must select supported capabilities and retry rejected optional devices");
            Check(!context.RTAOEnabled() && !context.RTGIEnabled() && !context.RTShadowsEnabled() && !context.RTReflectionsEnabled(),
                  "capability detection must not enable RT effects");
            Check(context.GetImageQuality().aa == "off", "capability detection must not select FSR reconstruction");
        };
#ifdef CWR_FSR3
        startup({3}); // Both startup variables absent on supported hardware.
        Check(requestedApiVersion == VK_API_VERSION_1_2, "normal startup automatically negotiates Vulkan 1.2");
        rejectedDeviceFeatures = 1; startup({3, 2}); // RT failure preserves FSR.
        rejectedDeviceFeatures = 2; startup({3, 2, 1}); // FSR failure preserves RT.
        rejectedDeviceFeatures = 3; startup({3, 2, 1, 0});
#else
        startup({1});
        rejectedDeviceFeatures = 1; startup({1, 0});
#endif
        rejectedDeviceFeatures = 0;
        startupRT = startupFSR = false; startup({0});
#ifdef CWR_FSR3
        startupFSR = true; startup({2}); // FSR remains usable on non-RT hardware.
        startupRT = true; startupFSR = false; startup({1});
#endif
        startupRT = startupFSR = true;
        rtEnvironment.Set("0"); fsrEnvironment.Set("0"); startup({0});
        Check(requestedApiVersion == VK_API_VERSION_1_0, "diagnostic overrides retain the original API baseline");
        rtEnvironment.Set("1"); fsrEnvironment.Set("1");
#ifdef CWR_FSR3
        startup({3});
        rtEnvironment.Set("0"); startup({2});
        rtEnvironment.Set("1"); fsrEnvironment.Set("0"); startup({1});
#else
        startup({1});
#endif
        rtEnvironment.Set(nullptr); fsrEnvironment.Set(nullptr);
        loaderVersion = VK_API_VERSION_1_0; startup({0});

        const unsigned createdBeforeRequest = instanceCreationCalls;
        bool rejectedMissingLayer = false;
        try
        {
            instanceContext.CreateInstance(surfaceExtensions, 1, true);
        }
        catch (const std::runtime_error& error)
        {
            rejectedMissingLayer = true;
            const std::string message = error.what();
            Check(message.find("--vk-validation") != std::string::npos &&
                      message.find("VK_LAYER_KHRONOS_validation") != std::string::npos,
                  "missing requested validation layer must identify the flag and dependency");
        }
        Check(rejectedMissingLayer, "explicit validation must fail when the Khronos layer is unavailable");
        Check(!instanceContext.Instance() && instanceCreationCalls == createdBeforeRequest,
              "missing requested validation layer must fail before creating an instance");
        validationLayerAvailable = true;
        VulkanContext validatedContext;
        validatedContext.CreateInstance(surfaceExtensions, 1, true);
        Check(requestedInstanceLayers == std::vector<std::string>{"VK_LAYER_KHRONOS_validation"},
              "available requested Khronos layer must be enabled, not just detected");
        validatedContext.Shutdown();
        validationLayerAvailable = false;

#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        availableInstanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
                                       VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
        instanceContext.CreateInstance(surfaceExtensions, 1);
        Check(std::count(requestedInstanceExtensions.begin(), requestedInstanceExtensions.end(),
                         VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 1,
              "Vulkan 1.0 portability requires properties2 enabled at instance creation");
        Check(requestedApiVersion == VK_API_VERSION_1_0 &&
                  (requestedInstanceFlags & VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR),
              "portability enumeration must retain its flag without raising the API baseline");
        instanceContext.Shutdown();
#endif

        availableInstanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME,
                                       VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
        const char* suppliedExtensions[] = {VK_KHR_SURFACE_EXTENSION_NAME,
                                            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
        instanceContext.CreateInstance(suppliedExtensions, 2);
        Check(std::count(requestedInstanceExtensions.begin(), requestedInstanceExtensions.end(),
                         VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 1,
              "properties2 supplied by the caller must not be added twice");
        instanceContext.Shutdown();

        availableInstanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
        VulkanContext teardownContext;
        teardownContext.CreateInstance(surfaceExtensions, 1);
        Check(teardownContext.ValidationErrors() == 0 && instanceCallback,
              "teardown regression must start with no validation errors and a registered callback");
        injectTeardownError = true;
        Check(teardownContext.Shutdown() == 1,
              "shutdown must return errors raised only during resource destruction, not the pre-teardown count");
        injectTeardownError = false;
        Check(teardownContext.Shutdown() == 1 && !teardownContext.Instance(),
              "repeated shutdown must retain the final error count without recreating resources");

        const auto raster = ShapeRasterization(false);
        Check(raster.cullMode == VK_CULL_MODE_BACK_BIT && raster.frontFace == VK_FRONT_FACE_CLOCKWISE,
              "native and software/screen geometry must reject the reverse face as GL33 does");
        Check(raster.polygonMode == VK_POLYGON_MODE_FILL && raster.lineWidth == 1 && !raster.depthBiasEnable,
              "culling correction must leave fill width and depth bias unchanged");
        Check(ShapeRasterization(true).cullMode == VK_CULL_MODE_NONE,
              "projected shadows retain their existing uncullled rasterization");
        VulkanContext context;
        {
            VulkanContext quality;
            using Poseidon::GraphicsQuality;
            Check(quality.SetGraphicsQuality(GraphicsQuality::Enhanced()).empty(), "enhanced raster quality accepted");
            Check(quality.GetGraphicsQuality() == GraphicsQuality::Enhanced(), "enhanced enables exactly its four groups");
            Check(!quality.RTShadowsEnabled() && !quality.RTGIEnabled() && !quality.RTAOEnabled() &&
                  !quality.RTReflectionsEnabled() && !quality.VolumetricFogEnabled(), "preset excludes deferred and RT features");
            const auto enhanced = quality.GetGraphicsQuality();
            Check(!quality.SetGraphicsQuality({2, 0, 0, false}).empty(), "uninitialized RT rejected");
            Check(quality.GetGraphicsQuality() == enhanced, "unsupported bundle changes nothing");
            Check(!quality.SetGraphicsQuality({0, 1, 2, true}).empty(), "uninitialized RT water rejected");
            Check(quality.GetGraphicsQuality() == enhanced, "unsupported water bundle changes nothing");
            Check(!quality.SetGraphicsQuality({0, 0, 3, false}).empty(), "invalid water rejected");
            Check(!quality.SetGraphicsQuality({3, 0, 0, false}).empty(), "invalid group rejected");
            Check(quality.GetGraphicsQuality() == enhanced, "invalid bundle changes nothing");
            Check(quality.SetGraphicsQuality({0, 1, 0, true}).empty(), "independent raster groups accepted");
            Check(quality.GetGraphicsQuality() == GraphicsQuality{0, 1, 0, true}, "independent groups retain values");
            Check(quality.GetGraphicsQuality().Preset() == 2, "mixed groups derive Custom");
            quality.SetSoftShadows("on");
            Check(quality.GetGraphicsQuality().shadows == -1, "developer partial groups are reported honestly");
            Check(quality.SetGraphicsQuality({}).empty(), "return to Standard accepted");
            Check(quality.GetGraphicsQuality() == GraphicsQuality{}, "Standard disables optional groups and AO");
        }
        Check(!context.SSAOEnabled(), "SSAO must be disabled by default");
        Check(context.SetSSAO(true, 0.7f, 1.2f, 0.03f, 80), "valid SSAO controls are accepted");
        Check(!context.RTAOEnabled(), "RTAO defaults off");
        Check(!context.RTGIEnabled(), "RTGI defaults off");
        Check(context.SetRTGI("on").starts_with("UNSUPPORTED:"), "GI requires initialized ray-query capabilities");
        Check(context.SetRTGI("invalid").starts_with("INVALID:"), "GI rejects invalid controls");
        Check(context.SetRTGI("status").find("lastWorldDispatch=no") != std::string::npos,
              "GI never claims execution from initialization");
        Check(context.SetRTGI("off").starts_with("OK:") && !context.RaySceneEnabled() && context.SSAOEnabled(),
              "GI off preserves independent AO selection");
        context.FailRTGI("test GI resources unavailable");
        Check(!context.RTGIEnabled() && !context.RaySceneEnabled() && context.SSAOEnabled(),
              "GI resource failure preserves ordinary lighting and independent SSAO");
        Check(context.SetRTGI("status").find("fallback=test GI resources unavailable") != std::string::npos,
              "GI resource failure exposes the actual fallback reason");
        Check(!context.RTDynamicEnabled() && context.SetRTDynamic("status").starts_with("RTDynamic=off active=no"),
              "dynamic geometry defaults off independently of the ray effects");
        Check(context.SetRTDynamic("on").starts_with("UNSUPPORTED:"), "dynamic geometry requires opted-in RT hardware");
        Check(context.SetRTDynamic("invalid").starts_with("INVALID:"), "dynamic geometry validates controls");
        Check(context.SetRTDynamic("off").starts_with("OK:") && context.SSAOEnabled(),
              "dynamic geometry off preserves the AO selection");
        Check(!context.RTShadowsEnabled(), "RT sunlight defaults off independently of AO");
        Check(!context.SoftRTShadowsEnabled() &&
                  context.SetSoftRTShadows("status").starts_with("SoftRTShadows=off eligible=no"),
              "soft RT defaults off and reports eligibility separately");
        Check(context.SetSoftRTShadows("on").starts_with("UNSUPPORTED:"), "soft RT requires opted-in ray capability");
        Check(!context.RaySceneEnabled() && !context.SoftShadowsEnabled(),
              "soft RT rejection never enables prerequisites or PCSS");
        const auto softRTStatus = context.SetSoftRTShadows("status");
        Check(context.SetSoftRTShadows("invalid").starts_with("INVALID:") &&
                  context.SetSoftRTShadows("status") == softRTStatus,
              "invalid soft RT command preserves selection");
        Check(context.SetSoftRTShadows("off").starts_with("OK:") && !context.SoftRTShadowsEnabled(),
              "soft RT off is safe without a device");
        Check(!context.RTReflectionsEnabled(), "RT water defaults off independently of AO and sunlight");
        Check(context.SetRTReflections("status").find("hitSurface=unavailable (diagnostics disabled)") != std::string::npos,
              "disabled hit diagnostics do not report misleading zero counts");
        Check(!context.PBREnabled() && context.SetPBR("status").find("requested=off") != std::string::npos,
              "PBR prototype defaults off");
        Check(context.SetPBR("on").starts_with("OK:") && context.PBREnabled() && !context.RaySceneEnabled(),
              "PBR needs no ray capable device");
        Check(context.SetPBR("status").find("stock-dielectric-v2") != std::string::npos &&
              context.SetPBR("status").find("software/other materials legacy") != std::string::npos,
              "PBR status identifies the prototype and coverage");
        Check(context.SetPBR("status").find("data/strecha_tasky_vln.pac") != std::string::npos &&
              context.SetPBR("status").find("data/strecha_tasky_vln90.pac") != std::string::npos &&
              context.SetPBR("status").find("data/prkna_dark.pac") != std::string::npos &&
              context.SetPBR("status").find("data/skala_piskovec2.pac") != std::string::npos &&
              context.SetPBR("status").find("data/sloupek_bily_orezly_.pac") != std::string::npos,
              "PBR status covers both new and original stock materials");
        context.RecordPBRDraw();
        context.SetPBR("on");
        Check(context.SetPBR("status").find("native draws since toggle=1") != std::string::npos,
              "idempotent PBR control retains native draw telemetry");
        Check(context.SetPBR("invalid").starts_with("INVALID:") && context.PBREnabled(),
              "invalid PBR control preserves mode");
        Check(context.SetPBR("off").starts_with("OK:") && !context.PBREnabled(), "PBR off restores legacy selection");
        Check(context.SetPBR("status").find("native draws since toggle=0") != std::string::npos,
              "PBR mode change resets telemetry");
        Check(context.SetWetSurfaces("status").find("requested=off") != std::string::npos &&
              context.SetUnderwater("status").find("requested=off") != std::string::npos,
              "environment effects default off");
        context.SetEnvironmentFrame(0, 10, {});
        context.SetEnvironmentFrame(1, 10, {});
        context.SetWetSurfaces("on");
        Check(context.WetRain() == 0, "wet weather smoothing freezes with simulation clock");
        context.SetEnvironmentFrame(1, 12, {});
        Check(context.WetRain() > .6f && context.WetRain() < .7f, "rain changes smoothly in simulation time");
        const float wet = context.WetRain();
        context.SetUnderwater("on");
        std::array<float, 12> submerged{};
        submerged[0] = .125f;
        context.SetEnvironmentFrame(1, 12, submerged);
        Check(context.UnderwaterRainVisibility() == .5f, "rain film fades continuously within the submerged crossing");
        submerged[0] = .25f;
        context.SetEnvironmentFrame(1, 12, submerged);
        Check(context.UnderwaterRainVisibility() == 0, "submerged camera excludes stock screen rain");
        Check(context.SetWetSurfaces("invalid").starts_with("INVALID:") && context.WetRain() == wet &&
              context.SetUnderwater("invalid").starts_with("INVALID:"), "invalid environment commands preserve state");
        context.SetWetSurfaces("off");
        Check(context.WetRain() == 0 && context.SetUnderwater("status").find("requested=on") != std::string::npos,
              "wet off restores immediately without changing underwater");
        context.SetUnderwater("off");
        Check(context.UnderwaterRainVisibility() == 1, "underwater off restores original rain film");
        context.SetEnvironmentFrame(0, 0, {});
        context.SetWetSurfaces("on");
        Check(context.WetRain() == 0 && !context.RaySceneEnabled() && !context.WaterSurfaceEnabled(),
              "simulation rewind resets weather and environmental effects need no ray or water resources");
        context.SetWetSurfaces("off");
        Check(!context.RainRipplesEnabled() &&
                  context.SetRainRipples("status", 1).find("requested=off active=off rain=1.000000") != std::string::npos,
              "rain ripples default off even in heavy rain");
        Check(context.SetRainRipples("on").starts_with("OK:") && context.RainRipplesEnabled(),
              "rain ripples require no ray device or refraction resources");
        Check(context.SetRainRipples("status", 0).find("requested=on active=off rain=0.000000") != std::string::npos,
              "dry weather leaves ripple request dormant");
        Check(context.SetRainRipples("status", .2f).find("requested=on active=on rain=0.200000") != std::string::npos,
              "status reports actual light rainfall");
        Check(context.SetRainRipples("invalid").starts_with("INVALID:") && context.RainRipplesEnabled(),
              "invalid ripple commands preserve selection");
        Check(!context.WaterSurfaceEnabled() && !context.RaySceneEnabled(),
              "ripples never allocate water targets or enable other water effects");
        Check(context.SetRainRipples("off").starts_with("OK:") && !context.RainRipplesEnabled(),
              "ripples toggle off independently");
        Check(context.SetWaterRefraction("status").find("requested=off active=off captured=no") != std::string::npos,
              "refraction defaults off and reports capture separately from selection");
        Check(context.SetWaterRefraction("on").starts_with("OK:"), "raster refraction needs no ray capability");
        const auto refractionRequested = context.SetWaterRefraction("status");
        Check(context.SetWaterRefraction("invalid").starts_with("INVALID:") &&
                  context.SetWaterRefraction("status") == refractionRequested,
              "invalid refraction commands preserve the current request");
        Check(!context.RTReflectionsEnabled() && !context.RTShadowsEnabled() && context.SSAOEnabled(),
              "refraction selection does not enable rays or change AO");
        Check(context.SetWaterRefraction("off").starts_with("OK:"), "refraction toggles off independently");
        Check(context.SetShoreFoam("status").find("requested=off active=off captured=no") != std::string::npos &&
                  !context.WaterCaptureEnabled(), "foam defaults off without capture resources");
        Check(context.SetShoreFoam("on").starts_with("OK:") && !context.RaySceneEnabled() &&
                  context.SetWaterRefraction("status").find("requested=off") != std::string::npos,
              "foam can request shared capture without refraction or ray support");
        const auto foamRequested = context.SetShoreFoam("status");
        Check(context.SetShoreFoam("invalid").starts_with("INVALID:") &&
                  context.SetShoreFoam("status") == foamRequested, "invalid foam command preserves selection");
        context.SetWaterRefraction("on");
        context.SetWaterRefraction("off");
        Check(context.SetShoreFoam("status") == foamRequested && !context.RainRipplesEnabled(),
              "refraction round trip preserves independent foam and ripple selections");
        Check(context.SetShoreFoam("off").starts_with("OK:") &&
                  context.SetShoreFoam("status").find("requested=off active=off") != std::string::npos,
              "foam round trip restores disabled request");
        {
            VulkanContext foamFailure;
            foamFailure.SetShoreFoam("on");
            foamFailure.FailWaterRefraction("test shared capture unavailable");
            Check(foamFailure.SetShoreFoam("status").find("requested=off active=off") != std::string::npos &&
                      foamFailure.SetShoreFoam("on").starts_with("UNSUPPORTED:") && !foamFailure.WaterCaptureEnabled(),
                  "foam-only capture failure disables shared resources and reports fallback");
        }
        context.SetWaterRefraction("on");
        context.FailWaterRefraction("test capture unavailable");
        Check(context.SetWaterRefraction("status").find("requested=off active=off") != std::string::npos &&
                  context.SetWaterRefraction("on").starts_with("UNSUPPORTED:"),
              "failed refraction resources retain original water and a concrete fallback");
        Check(context.SetRTReflections("on").starts_with("UNSUPPORTED:"), "RT water rejects missing ray capabilities");
        Check(context.SSAOEnabled() && !context.RTAOEnabled() && !context.RTShadowsEnabled(),
              "water rejection leaves other lighting selections intact");
        Check(context.SetRTReflections("bad").starts_with("INVALID:"), "water control validates input");
        Check(context.SetRTReflections("off").starts_with("OK:"), "ordinary water remains available");
        Check(context.SetRTReflections("status").find("rayQueriesExecuted=no hitShadingExecuted=no") !=
                  std::string::npos,
              "water status distinguishes completed shading from capability initialization");
        context.FailRTReflections("test resource unavailable");
        Check(!context.RTReflectionsEnabled() && context.SSAOEnabled(),
              "failed water resources preserve ordinary lighting");
        Check(context.SetRTReflections("status").find("fallback=test resource unavailable") != std::string::npos,
              "water resource failures expose a concrete fallback reason");
        Check(context.SetRTShadows("on").starts_with("UNSUPPORTED:"), "RT sunlight rejects missing capabilities");
        Check(context.SSAOEnabled() && !context.RTAOEnabled(), "RT shadow rejection preserves AO selection");
        Check(context.SetRTShadows("invalid").starts_with("INVALID:"), "RT sunlight validates controls");
        Check(context.SetRTShadows("off").starts_with("OK:"), "RT sunlight off is always safe");
        Check(context.SetRTShadows("status").find("totalRays=0 rayQueriesExecuted=no") != std::string::npos,
              "RT sunlight status never equates initialization with ray execution");
        Check(context.SetRTAO("on").starts_with("UNSUPPORTED:"), "RTAO rejects missing device capabilities");
        Check(context.SSAOEnabled(), "unsupported RTAO leaves SSAO intact");
        Check(context.SetRTAO("status").find("rayQueriesExecuted=no") != std::string::npos,
              "RTAO never claims tracing from initialization alone");
        Check(context.SetRTAO("invalid").starts_with("INVALID:"), "RTAO rejects unknown commands");
        Check(context.SetRTAO("off").starts_with("OK:") && context.SSAOEnabled(), "RTAO off preserves SSAO preference");
        Check(context.SSAOEnabled(), "SSAO runtime enable must take effect");
        Check(context.SetCloudShadows("status").find("requested=off active=off") == 0, "cloud shadows default off");
        Check(context.SetSkyReflections("status").find("requested=off active=off") == 0, "sky reflections default off");
        Check(context.SetCloudShadows("on").starts_with("OK:") && context.CloudShadowsEnabled(),
              "cloud sunlight works without ray-query hardware");
        Check(context.SeparateSunlight(), "clouds request direct sunlight separation");
        Check(context.SetSkyReflections("on").find("inactive: requires") != std::string::npos,
              "sky reflections report the missing RT prerequisite");
        Check(context.CloudShadowsEnabled() && !context.RTReflectionsEnabled(), "sky control does not enable RT or change clouds");
        Check(context.SetCloudShadows("invalid").starts_with("INVALID:") && context.CloudShadowsEnabled(),
              "invalid cloud selector preserves the request");
        Check(context.SetSkyReflections("invalid").starts_with("INVALID:"), "invalid sky selector is rejected");
        context.SetCloudShadows("off");
        Check(!context.CloudShadowsEnabled() && context.SetSkyReflections("status").find("requested=on") == 0,
              "cloud and sky requests are independent");
        context.SetSkyReflections("off");
        Check(!context.WeatherNeeded(), "disabled weather requires no coverage or sky update");
        Check(context.SetVolumetricClouds("status").starts_with("requested=off active=off"),
              "volume clouds default off");
        Check(!context.DrawVolumetricClouds(), "unavailable volume draw preserves legacy clouds");
        Check(context.SetVolumetricClouds("on").starts_with("OK:") && context.WeatherNeeded(),
              "volume clouds request stock weather without hardware ray queries");
        Check(!context.CloudShadowsEnabled() && !context.RTReflectionsEnabled() && !context.SeparateSunlight(),
              "volume cloud control is independent of surface shadows and RT");
        Check(context.SetVolumetricClouds("invalid").starts_with("INVALID:") && context.WeatherNeeded(),
              "invalid cloud command preserves request");
        Check(!context.DrawVolumetricClouds(), "requested clouds without an acquired world must not suppress stock draw");
        context.SetVolumetricClouds("off");
        Check(!context.WeatherNeeded(), "volume clouds off releases weather demand");
        Check(!context.SetSSAO(false, std::numeric_limits<float>::quiet_NaN(), 1, 0, 80), "reject NaN SSAO strength");
        Check(!context.SetSSAO(false, 1, 0, 0, 80), "reject zero SSAO radius");
        Check(!context.SetSSAO(false, 1, 1, 1, 80), "reject SSAO bias covering its radius");
        Check(!context.SetSSAO(false, 1, 1, 0, std::numeric_limits<float>::infinity()), "reject infinite SSAO distance");
        Check(context.SSAOEnabled(), "invalid controls must not partially change SSAO state");
        context.DrawSSAO({1, 1, 1.001f, -0.1f}); // No acquired frame: no driver commands/resources.
        Check(context.SetSSAO(false, 0.7f, 1.2f, 0.03f, 80) && !context.SSAOEnabled(), "SSAO toggles off without recreation");
        CheckThrows<std::logic_error>([&] { context.DrawDiagnosticTriangle(); },
                                      "indexed draw must require an acquired recording frame");
        Check(!context.Instance() && !context.FrameOpen(), "context must start empty");
        Check(context.SetAntiAliasing("status").find("active=off requested=off scale=100") == 0, "AA is opt-in");
        Check(context.SetAntiAliasing("fxaa").find("OK:") == 0, "queue FXAA selection");
        const auto requested = context.SetAntiAliasing("status");
        Check(context.SetAntiAliasing("msaa8").find("UNSUPPORTED:") == 0, "unsupported samples fail explicitly");
        Check(context.SetAntiAliasing("status") == requested, "failed AA selections retain previous request");
        Check(context.SetAntiAliasing("taa").find("OK:") == 0, "queue motion-based temporal AA");
        Check(context.SetAntiAliasing("motion").find("OK:") == 0, "motion visualization is explicit");
        Check(context.SetAntiAliasing("motion-off").find("OK:") == 0, "disable motion visualization");
        Check(context.SetAntiAliasing("smaa+taa").find("INVALID:") == 0, "no implicit stacking");
        const auto beforeFSR = context.SetAntiAliasing("status");
        for (const char* name : {"fsr-native", "fsr-quality", "fsr-balanced", "fsr-performance"})
        {
            AAMode mode = AAMode::Off;
            Check(ParseAA(name, mode) && IsFSR(mode) && IsTemporal(mode), "FSR selectors use the temporal world path");
            Check(context.SetAntiAliasing(name).find("UNSUPPORTED:") == 0, "FSR needs explicit runtime capability");
            Check(context.SetAntiAliasing("status") == beforeFSR, "unavailable FSR preserves configuration");
            Check(AASamples(mode) == VK_SAMPLE_COUNT_1_BIT, "FSR cannot stack MSAA");
        }
        Check(!context.SetRenderScale(150), "scale needs a device before limits query");
        for (int scale : {100,125,150,200}) Check(ValidRenderScale(scale), "supported render scale");
        Check(!ValidRenderScale(99) && !ValidRenderScale(201), "reject unbounded render scales");
        const auto scaled = ScaledExtent({1921,1081},125);
        Check(scaled.width==2402 && scaled.height==1352, "odd drawable fractional scale rounds up");
        Check(AASamples(AAMode::MSAA2)==VK_SAMPLE_COUNT_2_BIT && AASamples(AAMode::MSAA4)==VK_SAMPLE_COUNT_4_BIT &&
              AASamples(AAMode::MSAA8)==VK_SAMPLE_COUNT_8_BIT, "real MSAA sample counts");
        Check(context.SwapInterval() == 1, "vsync starts enabled");
        context.SetSwapInterval(-1);
        Check(context.SwapInterval() == -1, "adaptive interval can be requested before device setup");
        CheckThrows<std::invalid_argument>([&] { context.SetSwapInterval(2); }, "invalid swap interval must fail");
        CheckThrows<std::runtime_error>([&] { context.CreateInstance(nullptr, 0); },
                                        "missing SDL extensions must fail before driver access");
        CheckThrows<std::logic_error>([&] { context.CreateDevice(VK_NULL_HANDLE); },
                                      "missing instance/surface must fail");
        CheckThrows<std::logic_error>([&] { context.BeginFrame(800, 600); }, "uninitialized frame must fail");
        context.Clear(0, 0, 0, 1); // No frame: no Vulkan call.
        context.ClearDepth();
        CheckThrows<std::logic_error>([&] { context.EndShadowPass(); }, "unmatched shadow pass end must fail");
        context.BeginShadowPass(); // Minimized/no-frame pass is balanced without Vulkan calls.
        CheckThrows<std::logic_error>([&] { context.BeginShadowPass(); }, "nested shadow passes must fail");
        CheckThrows<std::logic_error>([&] { context.EndFrame(); }, "unfinished shadow pass cannot cross frames");
        context.EndShadowPass();
        const std::array<float, 16> matrix{};
        const std::array<float, 4> color{1, 1, 1, 1};
        CheckThrows<std::invalid_argument>([&] { context.UploadMesh(matrix.data(), sizeof(matrix), matrix.data(), 6); },
                                           "mesh upload without a device must fail before Vulkan calls");
        CheckThrows<std::logic_error>([&] { context.DrawMesh({}, 0, 3, true, matrix, color); },
                                      "mesh draw without a frame/buffers must fail before Vulkan calls");
        context.EndFrame();
        context.SetNightEye(1);
        Check(context.EyeCoef() == std::array<float, 4>{0.2f, 0.9f, 0.4f, 0}, "night eye coefficients match GL33");
        context.SetNightEye(0);
        Check(context.EyeCoef() == std::array<float, 4>{0, 0, 0, 1}, "day and HUD disable night eye");
        context.WaitIdle();
        context.Shutdown();
        context.Shutdown();
        Check(!context.Instance() && !context.FrameOpen(), "empty shutdown must be idempotent");
        std::printf("Vulkan driver-free policy/lifetime guards: %d checks passed\n", checks);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Vulkan test failed: %s\n", error.what());
        return 1;
    }
}
