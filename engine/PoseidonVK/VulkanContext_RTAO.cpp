#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/Shaders/gamma.vert.hpp>
#include <PoseidonVK/Shaders/rtao.comp.hpp>
#include <PoseidonVK/Shaders/rtao_ms.comp.hpp>
#include <PoseidonVK/Shaders/rtao_filter.frag.hpp>
#include <PoseidonVK/Shaders/rtao_filter_ms.frag.hpp>
#include <PoseidonVK/Shaders/rtshadow.comp.hpp>
#include <PoseidonVK/Shaders/rtshadow_ms.comp.hpp>
#include <PoseidonVK/Shaders/rtshadow_filter.frag.hpp>
#include <PoseidonVK/Shaders/rtshadow_filter_ms.frag.hpp>
#include <PoseidonVK/Shaders/rtwater.comp.hpp>
#include <PoseidonVK/Shaders/rtwater_ms.comp.hpp>
#include <PoseidonVK/Shaders/rtwater_filter.frag.hpp>
#include <PoseidonVK/Shaders/rtwater_filter_ms.frag.hpp>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <set>
#include <sstream>
#include <stdexcept>

namespace Poseidon::vk
{
namespace
{
void Require(VkResult result, const char* operation)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string("Vulkan RTAO: ") + operation + " (" + std::to_string(result) + ")");
}
VkDeviceAddress Address(VkDevice device, const BufferVK& buffer)
{
    VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    info.buffer = buffer.buffer;
    auto get = reinterpret_cast<PFN_vkGetBufferDeviceAddress>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddress"));
    if (!get)
        throw std::runtime_error("RTAO: buffer device address entry point unavailable");
    return get(device, &info);
}
struct RTBuffer
{
    VkDevice device = VK_NULL_HANDLE;
    BufferVK buffer;
    ~RTBuffer() { DestroyBuffer(device, buffer); }
    void Create(VkPhysicalDevice physical, VkDevice dev, VkDeviceSize size, VkBufferUsageFlags usage, bool host = false)
    {
        device = dev;
        usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if (host)
        {
            Require(CreateHostVisibleBuffer(physical, device, size, usage, buffer), "allocate mapped RT buffer");
            return;
        }
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = size;
        info.usage = usage;
        Require(vkCreateBuffer(device, &info, nullptr, &buffer.buffer), "create RT buffer");
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device, buffer.buffer, &requirements);
        VkPhysicalDeviceMemoryProperties memory{};
        vkGetPhysicalDeviceMemoryProperties(physical, &memory);
        VkMemoryAllocateFlagsInfo flags{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
        flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.pNext = &flags;
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex =
            FindMemoryType(memory, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (allocation.memoryTypeIndex == UINT32_MAX)
            throw std::runtime_error("RTAO: no device-local memory");
        Require(vkAllocateMemory(device, &allocation, nullptr, &buffer.memory), "allocate RT memory");
        Require(vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0), "bind RT buffer");
        buffer.size = size;
    }
};
struct Acceleration
{
    VkDevice device = VK_NULL_HANDLE;
    PFN_vkDestroyAccelerationStructureKHR destroy = nullptr;
    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    RTBuffer storage;
    ~Acceleration()
    {
        if (handle)
            destroy(device, handle, nullptr);
    }
};
struct BLAS : Acceleration
{
    std::shared_ptr<MeshBuffers> mesh;
    std::vector<RTAORange> ranges;
    RTBuffer materials;
    bool index16 = true;
    uint64_t used = 0;
    uint32_t triangles = 0;
    VkDeviceAddress address = 0;
};
struct RayMaterial
{
    uint64_t indices, vertices, alpha;
    uint32_t first, index16, width, height, sampler;
    float cutoff;
    uint64_t color;
    uint32_t colorWidth, colorHeight;
};
static_assert(sizeof(RayMaterial) == 64);
struct Push
{
    std::array<float, 16> camera;
    std::array<float, 4> projection;
    std::array<float, 4> settings{2.f, .035f, .85f, 60.f};
    std::array<float, 4> jitter{};
    std::array<float, 4> sun{};
};
static_assert(sizeof(Push) == 128);
void BuildBarrier(VkCommandBuffer command, VkPipelineStageFlags destination)
{
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, destination, 0, 1, &barrier,
                         0, nullptr, 0, nullptr);
}
} // namespace

struct VulkanContext::RTAOState
{
    PFN_vkCreateAccelerationStructureKHR create = nullptr;
    PFN_vkDestroyAccelerationStructureKHR destroy = nullptr;
    PFN_vkGetAccelerationStructureBuildSizesKHR sizes = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR build = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR address = nullptr;
    uint32_t alignment = 256;
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    std::array<VkPipeline, 6> compute{}, filter{};
    struct Slot
    {
        std::shared_ptr<Acceleration> tlas;
        std::shared_ptr<RTBuffer> instances, scratch, counters, materials;
        std::vector<std::shared_ptr<RTBuffer>> buildScratch;
        std::vector<std::shared_ptr<BLAS>> retained;
        std::vector<VkAccelerationStructureInstanceKHR> previous;
        DepthAttachment ao;
        DepthAttachment sunlight;
        DepthAttachment reflection;
        std::shared_ptr<RTBuffer> reflectionLight;
        VkDescriptorSet waterSet = VK_NULL_HANDLE;
        VkExtent2D extent{};
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSet set = VK_NULL_HANDLE;
        VkDescriptorSet sunSet = VK_NULL_HANDLE;
        VkQueryPool queries = VK_NULL_HANDLE;
        bool pending = false;
        uint32_t built = 0, tlasBuilt = 0;
    };
    std::array<Slot, FramesInFlight> slots;
    std::vector<std::shared_ptr<BLAS>> cache;
    std::vector<RTAOInstance> scene;
    std::array<float, 16> camera{};
    uint64_t serial = 0, completed = 0, totalRays = 0, totalHits = 0, blasBuilds = 0, tlasBuilds = 0;
    uint64_t timingSamples = 0;
    uint32_t lastRays = 0, lastHits = 0, instanceCount = 0, triangleCount = 0;
    double blasMs = 0, tlasMs = 0, rayMs = 0, filterMs = 0, frameMs = 0;
    bool worldDispatched = false;
    uint64_t totalSunRays = 0, totalSunHits = 0;
    uint32_t lastSunRays = 0, lastSunHits = 0;
    uint32_t lastTerrainHits = 0, lastAlphaHits = 0;
    double sunRayMs = 0, sunFilterMs = 0;
    bool sunDispatched = false;
    uint64_t waterRays = 0, shadedHits = 0;
    uint32_t lastWaterRays = 0, lastWaterHits = 0, lastShaded = 0, lastShadedTerrain = 0, lastShadedAlpha = 0;
    bool waterDispatched = false;
    double waterRayMs = 0, waterFilterMs = 0;
};

void VulkanContext::FailRTReflections(std::string_view reason)
{
    _rtReflectionFailure = reason;
    _rtReflectionsEnabled = false;
    _recreate = true;
    ResetTemporalHistory();
    if (_rtao)
    {
        _rtao->scene.clear(); // never query the preceding camera's stale coverage
        _rtao->waterDispatched = false;
        for (auto& slot : _rtao->slots)
            slot.previous.clear(); // retry any interrupted TLAS build
    }
    std::fprintf(stderr, "RT water unavailable: %s; preserving ordinary water\n", _rtReflectionFailure.c_str());
}

std::string VulkanContext::SetRTReflections(std::string_view mode)
{
    if (mode == "status")
    {
        std::ostringstream out;
        out << "RTReflections=" << (RTReflectionsEnabled() ? "on" : "off") << " capability=" << _rtaoReason;
        if (!_rtReflectionFailure.empty())
            out << " fallback=" << _rtReflectionFailure;
        if (_rtao)
            out << " waterRays=" << _rtao->lastWaterRays << " hits=" << _rtao->lastWaterHits
                << " shadedHits=" << _rtao->lastShaded << " shadedTerrain=" << _rtao->lastShadedTerrain
                << " shadedAlpha=" << _rtao->lastShadedAlpha << " totalWaterRays=" << _rtao->waterRays
                << " totalShadedHits=" << _rtao->shadedHits
                << " rayQueriesExecuted=" << (_rtao->waterRays ? "yes" : "no")
                << " hitShadingExecuted=" << (_rtao->shadedHits ? "yes" : "no")
                << " lastWorldDispatch=" << (_rtao->waterDispatched ? "yes" : "no");
        else
            out << " waterRays=0 shadedHits=0 rayQueriesExecuted=no hitShadingExecuted=no";
        if (_rtao && _rtao->timingSamples)
            out << " lastGPUms(traceShade/filter)=" << _rtao->waterRayMs << '/' << _rtao->waterFilterMs;
        else
            out << " GPUtimings=unavailable";
        out << "; shared " << SetRTAO("status")
            << "; actual water only, 80m receivers/70m rays, static textured hits and weather sky fallback";
        return out.str();
    }
    if (mode != "on" && mode != "off")
        return "INVALID: triRTReflections on, off, status";
    if (mode == "on" && !_rtaoAvailable)
        return "UNSUPPORTED: " + _rtaoReason;
    if (mode == "on")
    {
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(_physical, &features);
        if (!features.independentBlend)
            return "UNSUPPORTED: RT water requires independent MRT blending";
        if (!_rtReflectionFailure.empty())
            return "UNSUPPORTED: " + _rtReflectionFailure;
    }
    if (_rtReflectionsEnabled != (mode == "on"))
    {
        _rtReflectionsEnabled = mode == "on";
        _recreate = true;
        ResetTemporalHistory();
    }
    if (!_rtReflectionsEnabled && _rtao)
    {
        _rtao->waterDispatched = false;
        if (!RaySceneEnabled())
        {
            _rtao->scene.clear();
            _rtao->instanceCount = _rtao->triangleCount = 0;
        }
    }
    return _rtReflectionsEnabled ? "OK: hardware water reflections enabled (next frame)"
                                 : "OK: original water restored";
}

std::string VulkanContext::SetRTShadows(std::string_view mode)
{
    if (mode == "status")
    {
        std::ostringstream out;
        out << "RTShadows=" << (RTShadowsEnabled() ? "on" : "off") << " capability=" << _rtaoReason
            << " terrainPatches=" << _rtTerrain << " foliageInstances=" << _rtFoliage;
        if (_rtao)
            out << " lastCompletedRays=" << _rtao->lastSunRays << " lastCompletedHits=" << _rtao->lastSunHits
                << " totalRays=" << _rtao->totalSunRays
                << " rayQueriesExecuted=" << (_rtao->totalSunRays ? "yes" : "no")
                << " lastWorldDispatch=" << (_rtao->sunDispatched ? "yes" : "no")
                << " lastTerrainHits=" << _rtao->lastTerrainHits << " lastAlphaHits=" << _rtao->lastAlphaHits;
        if (_rtao && _rtao->timingSamples)
            out << " lastGPUms(rays/compose)=" << _rtao->sunRayMs << '/' << _rtao->sunFilterMs;
        else
            out << " GPUtimings=unavailable";
        if (!_rtao)
            out << " totalRays=0 rayQueriesExecuted=no lastWorldDispatch=no";
        out << "; shared " << SetRTAO("status")
            << "; sunlight-only native opaque/cutout and terrain receivers, CSM fallback retained";
        return out.str();
    }
    if (mode != "on" && mode != "off")
        return "INVALID: triRTShadows on, off, status";
    if (mode == "on" && !_rtaoAvailable)
        return "UNSUPPORTED: " + _rtaoReason;
    if (mode == "on")
    {
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(_physical, &features);
        if (!features.independentBlend)
            return "UNSUPPORTED: RT sunlight requires independent MRT blending";
    }
    if (_rtShadowsEnabled != (mode == "on"))
    {
        _rtShadowsEnabled = mode == "on";
        _recreate = true;
        ResetTemporalHistory();
    }
    if (!_rtShadowsEnabled)
    {
        _rtTerrain = _rtFoliage = 0;
        if (_rtao)
            _rtao->sunDispatched = false;
        if (_rtao && !RaySceneEnabled())
        {
            _rtao->scene.clear();
            _rtao->instanceCount = _rtao->triangleCount = 0;
        }
    }
    return _rtShadowsEnabled ? "OK: RT sunlight shadows enabled (next frame), CSM fallback retained"
                             : "OK: RT sunlight shadows off";
}

void VulkanContext::InitRTAO()
{
    _rtao = std::make_shared<RTAOState>();
    auto& rt = *_rtao;
#define LOAD(member, name)                                                         \
    rt.member = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(_device, #name)); \
    if (!rt.member)                                                                \
    {                                                                              \
        _rtaoAvailable = false;                                                    \
        _rtaoReason = "missing " #name;                                            \
        return;                                                                    \
    }
    LOAD(create, vkCreateAccelerationStructureKHR);
    LOAD(destroy, vkDestroyAccelerationStructureKHR);
    LOAD(sizes, vkGetAccelerationStructureBuildSizesKHR);
    LOAD(build, vkCmdBuildAccelerationStructuresKHR);
    LOAD(address, vkGetAccelerationStructureDeviceAddressKHR);
#undef LOAD
    VkPhysicalDeviceAccelerationStructurePropertiesKHR as{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties.pNext = &as;
    auto query = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
        vkGetInstanceProcAddr(_instance, "vkGetPhysicalDeviceProperties2"));
    if (!query || !vkGetDeviceProcAddr(_device, "vkGetBufferDeviceAddress"))
    {
        _rtaoAvailable = false;
        _rtaoReason = "Vulkan 1.2 query/address entry points unavailable";
        return;
    }
    query(_physical, &properties);
    rt.alignment = as.minAccelerationStructureScratchOffsetAlignment;
    if (as.maxInstanceCount < 1024 || as.maxGeometryCount < 1024 || as.maxPrimitiveCount < 1000000)
    {
        _rtaoAvailable = false;
        _rtaoReason = "acceleration structure limits below the bounded scene budget";
        return;
    }
    std::fprintf(stderr, "Vulkan RTAO: hardware ray queries available, default off; scratch alignment=%u\n",
                 rt.alignment);
}

std::string VulkanContext::SetRTAO(std::string_view mode)
{
    if (mode == "status")
    {
        std::ostringstream out;
        out << "RTAO=" << (_rtaoEnabled ? "on" : "off") << " capability=" << _rtaoReason;
        if (_rtao)
        {
            const auto& rt = *_rtao;
            std::set<const BLAS*> live;
            for (const auto& blas : rt.cache)
                if (blas)
                    live.insert(blas.get());
            for (const auto& slot : rt.slots)
                for (const auto& blas : slot.retained)
                    live.insert(blas.get());
            out << " BLAS=" << live.size() << " cachedBLAS=" << rt.cache.size() << " TLAS="
                << std::count_if(rt.slots.begin(), rt.slots.end(), [](const auto& slot) { return bool(slot.tlas); })
                << " instances=" << rt.instanceCount << " instancedTriangles=" << rt.triangleCount
                << " completedPasses=" << rt.completed << " lastCompletedRays=" << rt.lastRays
                << " lastCompletedHits=" << rt.lastHits << " totalRays=" << rt.totalRays
                << " rayQueriesExecuted=" << (rt.totalRays ? "yes" : "no")
                << " lastWorldDispatch=" << (rt.worldDispatched ? "yes" : "no") << " BLASbuilds=" << rt.blasBuilds
                << " TLASbuilds=" << rt.tlasBuilds;
            if (rt.timingSamples)
                out << " lastGPUms(BLAS/TLAS/rays/filter/frame)=" << rt.blasMs << '/' << rt.tlasMs << '/' << rt.rayMs
                    << '/' << rt.filterMs << '/' << rt.frameMs;
            else
                out << " GPUtimings=unavailable (requires CWR_VK_PROFILE=1 and completed samples)";
        }
        else
            out << " BLAS=0 TLAS=0 instances=0 completedPasses=0 rayQueriesExecuted=no";
        out << "; shared static LOD0 scene radius=" << (RTShadowsEnabled() || RTReflectionsEnabled() ? 160 : 80)
            << "m; AO mask excludes terrain/foliage; all rays exclude animation, damage, alpha blending and proxies";
        return out.str();
    }
    if (mode != "on" && mode != "off")
        return "INVALID: triRTAO on, off, status";
    if (mode == "on" && !_rtaoAvailable)
        return "UNSUPPORTED: " + _rtaoReason;
    _rtaoEnabled = mode == "on";
    ResetTemporalHistory();
    if (!_rtaoEnabled && _rtao)
    {
        if (!RaySceneEnabled())
        {
            _rtao->scene.clear();
            _rtao->instanceCount = _rtao->triangleCount = 0;
        }
        _rtao->worldDispatched = false;
    }
    return _rtaoEnabled ? "OK: RTAO enabled; SSAO suspended; status reports completed GPU rays"
                        : "OK: RTAO off; previous SSAO setting restored";
}

void VulkanContext::SetRTAOScene(std::vector<RTAOInstance> instances, const std::array<float, 16>& camera)
{
    if (!RaySceneEnabled() || !_rtao)
        return;
    if (instances.size() > 1024)
        instances.resize(1024);
    _rtao->scene = std::move(instances);
    _rtao->camera = camera;
}

void VulkanContext::PrepareRTAOAlpha(const std::shared_ptr<TextureImage>& image, uint32_t width, uint32_t height,
                                     const void* rgba)
{
    if (!RaySceneEnabled() || !image || image->rayAlpha.buffer)
        return;
    Require(CreateHostVisibleBuffer(_physical, _device, VkDeviceSize(width) * height * 4,
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                    image->rayAlpha),
            "allocate building alpha mask");
    UploadMappedBuffer(image->rayAlpha, rgba, size_t(width) * height * 4);
    image->rayWidth = width;
    image->rayHeight = height;
}

void VulkanContext::RetireRTAO()
{
    if (!_rtao)
        return;
    auto& rt = *_rtao;
    auto& slot = rt.slots[_frame]; // caller has waited for this slot's fence
    if (slot.pending)
    {
        const auto* counts = static_cast<const uint32_t*>(slot.counters->buffer.mapped);
        rt.lastRays = counts[0];
        rt.lastHits = counts[1];
        rt.totalRays += counts[0];
        rt.totalHits += counts[1];
        rt.lastSunRays = counts[2];
        rt.lastSunHits = counts[3];
        rt.totalSunRays += counts[2];
        rt.totalSunHits += counts[3];
        rt.lastTerrainHits = counts[4];
        rt.lastAlphaHits = counts[5];
        rt.lastWaterRays = counts[6];
        rt.lastWaterHits = counts[7];
        rt.lastShaded = counts[8];
        rt.lastShadedTerrain = counts[9];
        rt.lastShadedAlpha = counts[10];
        rt.waterRays += counts[6];
        rt.shadedHits += counts[8];
        ++rt.completed;
        rt.blasBuilds += slot.built;
        rt.tlasBuilds += slot.tlasBuilt;
        if (slot.queries)
        {
            uint64_t stamps[16]{};
            if (vkGetQueryPoolResults(_device, slot.queries, 0, 16, sizeof(stamps), stamps, sizeof(uint64_t),
                                      VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
            {
                const uint64_t mask = _timestampBits == 64 ? UINT64_MAX : (uint64_t(1) << _timestampBits) - 1;
                auto ms = [&](int i) { return double((stamps[i + 1] - stamps[i]) & mask) * _timestampPeriod / 1e6; };
                rt.blasMs = ms(0);
                rt.tlasMs = ms(2);
                rt.rayMs = ms(4);
                rt.filterMs = ms(6);
                rt.sunRayMs = ms(8);
                rt.sunFilterMs = ms(10);
                rt.waterRayMs = ms(12);
                rt.waterFilterMs = ms(14);
                ++rt.timingSamples;
                uint64_t frame[2]{};
                if (_frames[_frame].frameQueries &&
                    vkGetQueryPoolResults(_device, _frames[_frame].frameQueries, 0, 2, sizeof(frame), frame,
                                          sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
                    rt.frameMs = double((frame[1] - frame[0]) & mask) * _timestampPeriod / 1e6;
                if (slot.built || slot.tlasBuilt || rt.completed % 120 == 0)
                    std::fprintf(stderr,
                                 "Vulkan RTAO GPU: BLAS=%.4f TLAS=%.4f rays=%.4f filter=%.4f frame=%.4f ms; "
                                 "built=%u/%u instances=%u triangles=%u rays=%u hits=%u\n",
                                 rt.blasMs, rt.tlasMs, rt.rayMs, rt.filterMs, rt.frameMs, slot.built, slot.tlasBuilt,
                                 rt.instanceCount, rt.triangleCount, rt.lastRays, rt.lastHits);
                if (rt.completed % 120 == 0)
                    std::fprintf(stderr,
                                 "Vulkan RT sunlight GPU: rays=%.4f compose=%.4f frame=%.4f ms; rays=%u hits=%u "
                                 "terrain=%u foliage=%u\n",
                                 rt.sunRayMs, rt.sunFilterMs, rt.frameMs, rt.lastSunRays, rt.lastSunHits, _rtTerrain,
                                 _rtFoliage);
                if (rt.completed % 120 == 0)
                    std::fprintf(stderr,
                                 "Vulkan RT water GPU: traceShade=%.4f filter=%.4f frame=%.4f ms; rays=%u hits=%u "
                                 "shaded=%u terrain=%u alpha=%u\n",
                                 rt.waterRayMs, rt.waterFilterMs, rt.frameMs, rt.lastWaterRays, rt.lastWaterHits,
                                 rt.lastShaded, rt.lastShadedTerrain, rt.lastShadedAlpha);
            }
        }
        slot.pending = false;
    }
    slot.buildScratch.clear();
}

void VulkanContext::DrawRTAO(const std::array<float, 4>& projection)
{
    if (!_frameOpen || !_worldActive || !RaySceneEnabled() || !_rtao)
        return;
    auto& rt = *_rtao;
    auto& slot = rt.slots[_frame];
    auto command = _frames[_frame].command;
    ++rt.serial;
    // No stale scene: empty coverage leaves lighting untouched and executes no rays.
    rt.instanceCount = rt.triangleCount = 0;
    rt.worldDispatched = false;
    rt.sunDispatched = false;
    rt.waterDispatched = false;
    if (rt.scene.empty())
        return;
    const VkExtent2D half{(_worldExtent.width + 1) / 2, (_worldExtent.height + 1) / 2};
    if (!rt.layout)
    {
        if (rt.setLayout)
            vkDestroyDescriptorSetLayout(_device, rt.setLayout, nullptr);
        rt.setLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayoutBinding bindings[9]{};
        const VkDescriptorType types[]{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
        for (uint32_t i = 0; i < 9; ++i)
            bindings[i] = {i, types[i], 1, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        set.bindingCount = 9;
        set.pBindings = bindings;
        Require(vkCreateDescriptorSetLayout(_device, &set, nullptr, &rt.setLayout), "create descriptors");
        const VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Push)};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        CreateWeatherLayout();
        const VkDescriptorSetLayout layouts[]{rt.setLayout, _weatherLayout};
        layout.setLayoutCount = 2;
        layout.pSetLayouts = layouts;
        layout.pushConstantRangeCount = 1;
        layout.pPushConstantRanges = &push;
        Require(vkCreatePipelineLayout(_device, &layout, nullptr, &rt.layout), "create layout");
    }
    if (std::any_of(rt.compute.begin(), rt.compute.begin() + (_rtWaterTargets ? 6 : 4), [](auto p) { return !p; }))
    {
        auto shader = [&](const uint32_t* code, size_t size)
        {
            VkShaderModule module = VK_NULL_HANDLE;
            VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            info.codeSize = size;
            info.pCode = code;
            Require(vkCreateShaderModule(_device, &info, nullptr, &module), "create shader");
            return module;
        };
        for (int i = 0; i < 6; ++i)
        {
            if (rt.compute[i] || (i >= 4 && !_rtWaterTargets))
                continue;
            const uint32_t* codes[]{Cwrrtao_comp,        Cwrrtao_ms_comp, Cwrrtshadow_comp,
                                    Cwrrtshadow_ms_comp, Cwrrtwater_comp, Cwrrtwater_ms_comp};
            const size_t sizes[]{sizeof(Cwrrtao_comp),        sizeof(Cwrrtao_ms_comp), sizeof(Cwrrtshadow_comp),
                                 sizeof(Cwrrtshadow_ms_comp), sizeof(Cwrrtwater_comp), sizeof(Cwrrtwater_ms_comp)};
            auto module = shader(codes[i], sizes[i]);
            VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
            info.layout = rt.layout;
            info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            info.stage.module = module;
            info.stage.pName = "main";
            auto result = vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &info, nullptr, &rt.compute[i]);
            vkDestroyShaderModule(_device, module, nullptr);
            Require(result, "create ray query pipeline");
        }
    }
    if (std::any_of(rt.filter.begin(), rt.filter.begin() + (_rtWaterTargets ? 6 : 4), [](auto p) { return !p; }))
    {
        VkShaderModuleCreateInfo shader{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shader.codeSize = sizeof(Cwrgamma_vert);
        shader.pCode = Cwrgamma_vert;
        VkShaderModule vertex = VK_NULL_HANDLE;
        Require(vkCreateShaderModule(_device, &shader, nullptr, &vertex), "create fullscreen vertex");
        for (int i = 0; i < 6; ++i)
        {
            if (rt.filter[i] || (i >= 4 && !_rtWaterTargets))
                continue;
            const uint32_t* codes[]{Cwrrtao_filter_frag,        Cwrrtao_filter_ms_frag, Cwrrtshadow_filter_frag,
                                    Cwrrtshadow_filter_ms_frag, Cwrrtwater_filter_frag, Cwrrtwater_filter_ms_frag};
            const size_t sizes[]{sizeof(Cwrrtao_filter_frag),     sizeof(Cwrrtao_filter_ms_frag),
                                 sizeof(Cwrrtshadow_filter_frag), sizeof(Cwrrtshadow_filter_ms_frag),
                                 sizeof(Cwrrtwater_filter_frag),  sizeof(Cwrrtwater_filter_ms_frag)};
            shader.codeSize = sizes[i];
            shader.pCode = codes[i];
            VkShaderModule fragment = VK_NULL_HANDLE;
            const auto fragmentResult = vkCreateShaderModule(_device, &shader, nullptr, &fragment);
            if (fragmentResult != VK_SUCCESS)
            {
                vkDestroyShaderModule(_device, vertex, nullptr);
                Require(fragmentResult, "create filter shader");
            }
            VkPipelineShaderStageCreateInfo stages[2]{};
            for (auto& stage : stages)
            {
                stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stage.pName = "main";
            }
            stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            stages[0].module = vertex;
            stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[1].module = fragment;
            VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            VkPipelineInputAssemblyStateCreateInfo assembly{
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewport.viewportCount = viewport.scissorCount = 1;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            raster.polygonMode = VK_POLYGON_MODE_FILL;
            raster.lineWidth = 1;
            VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            VkPipelineColorBlendAttachmentState output{};
            output.blendEnable = VK_TRUE;
            output.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            output.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
            output.colorBlendOp = output.alphaBlendOp = VK_BLEND_OP_ADD;
            if (i >= 2)
            {
                output.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                output.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                output.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
            }
            output.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            if (i >= 4)
            {
                output.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                output.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                output.colorBlendOp = VK_BLEND_OP_ADD;
            }
            output.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            output.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
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
            pipeline.layout = rt.layout;
            pipeline.renderPass = _ssaoPass;
            auto result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &rt.filter[i]);
            vkDestroyShaderModule(_device, fragment, nullptr);
            if (result != VK_SUCCESS)
            {
                vkDestroyShaderModule(_device, vertex, nullptr);
                Require(result, "create filter pipeline");
            }
        }
        vkDestroyShaderModule(_device, vertex, nullptr);
    }
    if (!slot.pool)
    {
        VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12},
                                     {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 3},
                                     {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 9}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = 3;
        pool.poolSizeCount = 4;
        pool.pPoolSizes = sizes;
        Require(vkCreateDescriptorPool(_device, &pool, nullptr, &slot.pool), "create descriptor pool");
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool = slot.pool;
        allocate.descriptorSetCount = 1;
        allocate.pSetLayouts = &rt.setLayout;
        Require(vkAllocateDescriptorSets(_device, &allocate, &slot.set), "allocate descriptors");
        Require(vkAllocateDescriptorSets(_device, &allocate, &slot.sunSet), "allocate sun descriptors");
        Require(vkAllocateDescriptorSets(_device, &allocate, &slot.waterSet), "allocate water descriptors");
        CreateAAImage(slot.ao, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, half);
        slot.extent = half;
        if (RTShadowsEnabled())
            CreateAAImage(slot.sunlight, VK_FORMAT_R32G32B32A32_SFLOAT,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, _worldExtent);
        if (_rtWaterTargets)
            CreateAAImage(slot.reflection, VK_FORMAT_R32G32B32A32_SFLOAT,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, _worldExtent);
    }
    auto buffer = [&](VkDeviceSize size, VkBufferUsageFlags usage, bool host = false)
    {
        auto result = std::make_shared<RTBuffer>();
        result->Create(_physical, _device, size, usage, host);
        return result;
    };
    if (!slot.counters)
        slot.counters = buffer(48, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    if (!slot.materials)
        slot.materials = buffer(1024 * sizeof(uint64_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    if (!slot.reflectionLight)
        slot.reflectionLight = buffer(sizeof(_reflectionLighting), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    UploadMappedBuffer(slot.reflectionLight->buffer, _reflectionLighting.data(), sizeof(_reflectionLighting));
    std::memset(slot.counters->buffer.mapped, 0, 48);
    if (!slot.queries && _timestampBits && _profile.enabled)
    {
        VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = 16;
        Require(vkCreateQueryPool(_device, &info, nullptr, &slot.queries), "create GPU timestamps");
    }
    FlushScreenBatch();
    if (_worldPassOpen)
        vkCmdEndRenderPass(command);
    _worldPassOpen = false;
    _commands = {};
    if (slot.queries)
        vkCmdResetQueryPool(command, slot.queries, 0, 16);
    auto stamp = [&](uint32_t i)
    {
        if (slot.queries)
            vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, slot.queries, i);
    };
    // Depth render-pass dependencies were originally fragment-only. Explicitly
    // include the compute reader; host uploads are made available at submission.
    VkMemoryBarrier depthBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    depthBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    depthBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &depthBarrier, 0, nullptr, 0, nullptr);
    auto allocateAS = [&](Acceleration& as, VkAccelerationStructureTypeKHR type, VkDeviceSize size)
    {
        as.device = _device;
        as.destroy = rt.destroy;
        as.storage.Create(_physical, _device, size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);
        VkAccelerationStructureCreateInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        info.type = type;
        info.buffer = as.storage.buffer.buffer;
        info.size = size;
        Require(rt.create(_device, &info, nullptr, &as.handle), "create acceleration structure");
    };
    auto scratchAddress = [&](const RTBuffer& scratch)
    {
        const auto address = Address(_device, scratch.buffer);
        return (address + rt.alignment - 1) & ~VkDeviceAddress(rt.alignment - 1);
    };
    slot.built = slot.tlasBuilt = 0;
    slot.retained.clear();
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    stamp(0);
    for (const auto& entry : rt.scene)
    {
        // A shared mesh can have more than one conservative material subset.
        // Keep each variant, rather than rebuilding when instances alternate subsets.
        auto found = std::find_if(rt.cache.begin(), rt.cache.end(),
                                  [&](const auto& blas)
                                  {
                                      return blas && blas->mesh == entry.mesh && blas->ranges == entry.ranges &&
                                             blas->index16 == entry.index16;
                                  });
        if (found == rt.cache.end())
        {
            rt.cache.emplace_back();
            found = rt.cache.end() - 1;
        }
        auto& cached = *found;
        if (!cached)
        {
            auto blas = std::make_shared<BLAS>();
            blas->mesh = entry.mesh;
            blas->ranges = entry.ranges;
            blas->index16 = entry.index16;
            std::vector<VkAccelerationStructureGeometryKHR> geometries;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
            std::vector<uint32_t> counts;
            std::vector<RayMaterial> materials;
            for (const auto& range : entry.ranges)
            {
                if (range.count % 3 ||
                    (uint64_t(range.first) + range.count) * (entry.index16 ? 2 : 4) > entry.mesh->indices.size)
                    throw std::runtime_error("RTAO: invalid immutable triangle range");
                VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
                geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                geometry.flags = range.alpha ? 0 : VK_GEOMETRY_OPAQUE_BIT_KHR;
                auto& triangles = geometry.geometry.triangles;
                triangles = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
                triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
                triangles.vertexData.deviceAddress = Address(_device, entry.mesh->vertices);
                triangles.vertexStride = 32; // ShapeVertex: position, normal, UV
                triangles.maxVertex = uint32_t(entry.mesh->vertices.size / 32) - 1;
                triangles.indexType = entry.index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
                triangles.indexData.deviceAddress =
                    Address(_device, entry.mesh->indices) + VkDeviceSize(range.first) * (entry.index16 ? 2 : 4);
                geometries.push_back(geometry);
                counts.push_back(range.count / 3);
                ranges.push_back({range.count / 3, 0, 0, 0});
                blas->triangles += range.count / 3;
                materials.push_back(
                    {Address(_device, entry.mesh->indices), Address(_device, entry.mesh->vertices),
                     range.alpha ? Address(_device, range.alpha->rayAlpha) : 0, range.first, entry.index16 ? 1u : 0u,
                     range.alpha ? range.alpha->rayWidth : 0, range.alpha ? range.alpha->rayHeight : 0, range.sampler,
                     range.cutoff, range.color ? Address(_device, range.color->rayAlpha) : 0,
                     range.color ? range.color->rayWidth : 0, range.color ? range.color->rayHeight : 0});
            }
            if (geometries.empty() || geometries.size() > 1024 || blas->triangles > 1000000)
                continue;
            blas->materials.Create(_physical, _device, materials.size() * sizeof(RayMaterial),
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
            UploadMappedBuffer(blas->materials.buffer, materials.data(), materials.size() * sizeof(RayMaterial));
            VkAccelerationStructureBuildGeometryInfoKHR info{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
            info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            info.geometryCount = uint32_t(geometries.size());
            info.pGeometries = geometries.data();
            VkAccelerationStructureBuildSizesInfoKHR sizes{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
            rt.sizes(_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info, counts.data(), &sizes);
            allocateAS(*blas, info.type, sizes.accelerationStructureSize);
            auto scratch = buffer(sizes.buildScratchSize + rt.alignment, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            info.dstAccelerationStructure = blas->handle;
            info.scratchData.deviceAddress = scratchAddress(*scratch);
            const auto* rangePointer = ranges.data();
            rt.build(command, 1, &info, &rangePointer);
            slot.buildScratch.push_back(scratch);
            VkAccelerationStructureDeviceAddressInfoKHR address{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
            address.accelerationStructure = blas->handle;
            blas->address = rt.address(_device, &address);
            cached = blas;
            ++slot.built;
        }
        cached->used = rt.serial;
        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = entry.transform;
        instance.mask = entry.mask;
        instance.instanceCustomIndex = uint32_t(instances.size()) | (entry.kind << 20);
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = cached->address;
        static_cast<uint64_t*>(slot.materials->buffer.mapped)[instances.size()] =
            Address(_device, cached->materials.buffer);
        instances.push_back(instance);
        slot.retained.push_back(cached);
        rt.triangleCount += cached->triangles;
    }
    BuildBarrier(command, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR);
    stamp(1);
    stamp(2);
    rt.instanceCount = uint32_t(instances.size());
    const bool changed = instances.size() != slot.previous.size() ||
                         (!instances.empty() && std::memcmp(instances.data(), slot.previous.data(),
                                                            instances.size() * sizeof(instances[0])) != 0);
    if (!slot.tlas || changed)
    {
        // Per-frame-slot ownership: replacing TLAS/scratch never races an in-flight query.
        slot.tlas.reset();
        slot.scratch.reset();
        slot.instances.reset();
        slot.instances = buffer(std::max<size_t>(1, instances.size()) * sizeof(VkAccelerationStructureInstanceKHR),
                                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true);
        if (!instances.empty())
            UploadMappedBuffer(slot.instances->buffer, instances.data(), instances.size() * sizeof(instances[0]));
        VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
        geometry.geometry.instances.data.deviceAddress = Address(_device, slot.instances->buffer);
        VkAccelerationStructureBuildGeometryInfoKHR info{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        info.geometryCount = 1;
        info.pGeometries = &geometry;
        const uint32_t count = uint32_t(instances.size());
        VkAccelerationStructureBuildSizesInfoKHR sizes{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        rt.sizes(_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info, &count, &sizes);
        slot.tlas = std::make_shared<Acceleration>();
        allocateAS(*slot.tlas, info.type, sizes.accelerationStructureSize);
        slot.scratch = buffer(sizes.buildScratchSize + rt.alignment, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.dstAccelerationStructure = slot.tlas->handle;
        info.scratchData.deviceAddress = scratchAddress(*slot.scratch);
        VkAccelerationStructureBuildRangeInfoKHR range{count, 0, 0, 0};
        const auto* ptr = &range;
        rt.build(command, 1, &info, &ptr);
        slot.previous = instances;
        slot.tlasBuilt = 1;
    }
    BuildBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    stamp(3);
    // Cache only local working geometry. Frame slots keep evicted entries alive
    // until every TLAS that references them has completed and been replaced.
    std::erase_if(rt.cache, [&](const auto& item) { return !item || item->used + 120 < rt.serial; });
    // All three effects consume this frame slot's single shared TLAS: sunlight,
    // water, then AO. Skipped timestamp intervals remain explicitly empty.
    for (int effect : {1, 2, 0})
    {
        // Contact-only sunlight must also precede reflections and AO when the
        // shared RT scene is used without hardware sunlight shadows.
        if (effect != 1 && !_contactDrew)
            DrawContactShadows(projection);
        const uint32_t rayStamp = 4 + effect * 4, filterStamp = rayStamp + 2;
        if (effect == 2 ? !(RTReflectionsEnabled() && _rtWaterTargets)
            : effect    ? !(RTShadowsEnabled() && _rtShadowTargets)
                        : !RTAOEnabled())
        {
            stamp(rayStamp);
            stamp(rayStamp + 1);
            stamp(filterStamp);
            stamp(filterStamp + 1);
            continue;
        }
        const auto set = effect == 2 ? slot.waterSet : effect ? slot.sunSet : slot.set;
        auto& target = effect == 2 ? slot.reflection : effect ? slot.sunlight : slot.ao;
        const auto dispatchExtent = effect ? _worldExtent : half;
        VkDescriptorImageInfo images[3]{
            {_aaSampler, _aaTargets[_image].sampledDepth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL},
            {VK_NULL_HANDLE, target.view, VK_IMAGE_LAYOUT_GENERAL},
            {_aaSampler, target.view, VK_IMAGE_LAYOUT_GENERAL}};
        VkDescriptorImageInfo direct{
            _aaSampler, _rtShadowTargets ? _aaTargets[_image].sunlight.view : _aaTargets[_image].color.view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo water{_aaSampler,
                                    _rtWaterTargets ? _aaTargets[_image].water.view : _aaTargets[_image].color.view,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorBufferInfo reflectionLight{slot.reflectionLight->buffer.buffer, 0, VK_WHOLE_SIZE};
        VkDescriptorBufferInfo counter{slot.counters->buffer.buffer, 0, 48};
        VkDescriptorBufferInfo materialBuffer{slot.materials->buffer.buffer, 0, VK_WHOLE_SIZE};
        VkWriteDescriptorSetAccelerationStructureKHR as{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
        as.accelerationStructureCount = 1;
        as.pAccelerationStructures = &slot.tlas->handle;
        VkWriteDescriptorSet writes[9]{};
        for (uint32_t i = 0; i < 9; ++i)
        {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = set;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
        }
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &images[0];
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        writes[1].pNext = &as;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[2].pImageInfo = &images[1];
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[3].pBufferInfo = &counter;
        writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[4].pImageInfo = &images[2];
        writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[5].pBufferInfo = &materialBuffer;
        writes[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[6].pImageInfo = &direct;
        writes[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[7].pImageInfo = &water;
        writes[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[8].pBufferInfo = &reflectionLight;
        vkUpdateDescriptorSets(_device, 9, writes, 0, nullptr);
        VkImageMemoryBarrier image{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        image.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        image.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        image.srcQueueFamilyIndex = image.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image.image = target.image;
        image.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &image);
        const int ms = (_worldSamples != VK_SAMPLE_COUNT_1_BIT) + effect * 2;
        Push push;
        push.camera = rt.camera;
        push.projection = projection;
        push.sun = _rtSun;
        if (effect)
            push.settings[3] = 50.f;
        if (IsTemporal(_aaMode))
        {
            push.jitter[0] = _taaJitter[0];
            push.jitter[1] = _taaJitter[1];
        }
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, rt.compute[ms]);
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, rt.layout, 0, 1, &set, 0, nullptr);
        const auto weather = PrepareWeather();
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, rt.layout, 1, 1, &weather, 0, nullptr);
        vkCmdPushConstants(command, rt.layout, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(push), &push);
        stamp(rayStamp);
        vkCmdDispatch(command, (dispatchExtent.width + 7) / 8, (dispatchExtent.height + 7) / 8, 1);
        stamp(rayStamp + 1);
        VkMemoryBarrier ready{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        ready.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        ready.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &ready, 0,
                             nullptr, 0, nullptr);
        stamp(filterStamp);
        if (effect == 1 && ContactShadowsEligible())
        {
            DrawContactShadows(projection, target.view);
            stamp(filterStamp + 1);
            rt.sunDispatched = true;
            continue;
        }
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = _ssaoPass;
        pass.framebuffer = _aaTargets[_image].ao;
        pass.renderArea.extent = _worldExtent;
        vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, rt.filter[ms]);
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, rt.layout, 0, 1, &set, 0, nullptr);
        const VkViewport viewport{0, 0, float(_worldExtent.width), float(_worldExtent.height), 0, 1};
        const VkRect2D scissor{{0, 0}, _worldExtent};
        vkCmdSetViewport(command, 0, 1, &viewport);
        vkCmdSetScissor(command, 0, 1, &scissor);
        vkCmdDraw(command, 3, 1, 0, 0);
        vkCmdEndRenderPass(command);
        stamp(filterStamp + 1);
        if (effect == 2)
            rt.waterDispatched = true;
        else if (effect)
            rt.sunDispatched = true;
        else
            rt.worldDispatched = true;
    }
    slot.pending = true;
}

void VulkanContext::DestroyRTAOTargets() noexcept
{
    if (!_rtao)
        return;
    for (auto& slot : _rtao->slots)
    {
        if (slot.pool)
            vkDestroyDescriptorPool(_device, slot.pool, nullptr);
        slot.pool = VK_NULL_HANDLE;
        slot.set = VK_NULL_HANDLE;
        slot.sunSet = VK_NULL_HANDLE;
        slot.waterSet = VK_NULL_HANDLE;
        if (slot.reflection.view)
            vkDestroyImageView(_device, slot.reflection.view, nullptr);
        if (slot.reflection.image)
            vkDestroyImage(_device, slot.reflection.image, nullptr);
        if (slot.reflection.memory)
            vkFreeMemory(_device, slot.reflection.memory, nullptr);
        slot.reflection = {};
        if (slot.sunlight.view)
            vkDestroyImageView(_device, slot.sunlight.view, nullptr);
        if (slot.sunlight.image)
            vkDestroyImage(_device, slot.sunlight.image, nullptr);
        if (slot.sunlight.memory)
            vkFreeMemory(_device, slot.sunlight.memory, nullptr);
        slot.sunlight = {};
        if (slot.ao.view)
            vkDestroyImageView(_device, slot.ao.view, nullptr);
        if (slot.ao.image)
            vkDestroyImage(_device, slot.ao.image, nullptr);
        if (slot.ao.memory)
            vkFreeMemory(_device, slot.ao.memory, nullptr);
        slot.ao = {};
        slot.extent = {};
    }
    for (auto& pipeline : _rtao->filter)
    {
        if (pipeline)
            vkDestroyPipeline(_device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
}

void VulkanContext::DestroyRTAO() noexcept
{
    if (!_rtao)
        return;
    DestroyRTAOTargets();
    for (auto& slot : _rtao->slots)
    {
        if (slot.queries)
            vkDestroyQueryPool(_device, slot.queries, nullptr);
        slot.tlas.reset();
        slot.retained.clear();
    }
    for (auto pipeline : _rtao->compute)
        if (pipeline)
            vkDestroyPipeline(_device, pipeline, nullptr);
    if (_rtao->layout)
        vkDestroyPipelineLayout(_device, _rtao->layout, nullptr);
    if (_rtao->setLayout)
        vkDestroyDescriptorSetLayout(_device, _rtao->setLayout, nullptr);
    _rtao.reset();
    _rtaoAvailable = _rtaoEnabled = false;
}
} // namespace Poseidon::vk
