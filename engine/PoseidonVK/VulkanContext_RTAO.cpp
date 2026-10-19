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
#include <PoseidonVK/Shaders/rtgi.comp.hpp>
#include <PoseidonVK/Shaders/rtgi_ms.comp.hpp>
#include <PoseidonVK/Shaders/rtgi_filter.frag.hpp>
#include <PoseidonVK/Shaders/rtgi_filter_ms.frag.hpp>
#include <PoseidonVK/Shaders/rtgi_resolve.comp.hpp>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <sstream>
#include <stdexcept>
#include <map>

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
    uint64_t dynamicId = 0, revision = 0;
    std::shared_ptr<RTBuffer> scratch;
};
struct RayMaterial
{
    uint64_t indices, vertices, alpha;
    uint32_t first, index16, width, height, sampler;
    float cutoff;
    uint64_t color;
    uint32_t colorWidth, colorHeight;
    uint64_t normalRoughness;
    uint32_t normalWidth, normalHeight;
    std::array<float, 4> enhanced;
};
static_assert(sizeof(RayMaterial) == 96);
static_assert(offsetof(RayMaterial,normalRoughness)==64 && offsetof(RayMaterial,enhanced)==80);
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
    std::array<VkPipeline, 8> compute{}, filter{};
    VkPipeline giResolve = VK_NULL_HANDLE;
    std::array<DepthAttachment, 2> giHistory{};
    unsigned giHistoryIndex = 0, giPreviousSlot = FramesInFlight, giPhase = 0, giLastFrame = 0;
    bool giHistoryInitialized = false, giHistoryValid = false;
    uint64_t giHistoryBytes = 0, giHistoryAllocations = 0, giHistoryResets = 0;
    std::array<float, 25> giLighting{};
    const TextureImage* giSkyImage = nullptr;
    struct Slot
    {
        struct DynamicMesh
        {
            std::shared_ptr<MeshBuffers> mesh;
            std::vector<unsigned char> vertices, indices;
            uint64_t revision = 0;
            bool seen = false;
            bool deformed = false;
        };
        std::map<uint64_t, DynamicMesh> dynamicMeshes;
        RTDynamicStats dynamic;
        std::shared_ptr<Acceleration> tlas;
        std::shared_ptr<RTBuffer> instances, scratch, counters, materials;
        std::vector<std::shared_ptr<RTBuffer>> buildScratch;
        std::vector<std::shared_ptr<BLAS>> retained;
        std::vector<VkAccelerationStructureInstanceKHR> previous;
        DepthAttachment ao;
        DepthAttachment sunlight;
        DepthAttachment reflection;
        DepthAttachment gi;
        VkDescriptorSet giSet = VK_NULL_HANDLE;
        bool giDispatched = false;
        bool giTemporal = false;
        double giCpuMs = 0;
        std::shared_ptr<RTBuffer> reflectionLight;
        VkDescriptorSet waterSet = VK_NULL_HANDLE;
        VkExtent2D extent{};
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSet set = VK_NULL_HANDLE;
        VkDescriptorSet sunSet = VK_NULL_HANDLE;
        VkQueryPool queries = VK_NULL_HANDLE;
        bool pending = false;
        bool surfaceMeasured = false;
        uint32_t built = 0, tlasBuilt = 0;
        uint32_t sunSamples = 0;
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
    uint32_t lastSunSamples = 0;
    uint32_t lastTerrainHits = 0, lastAlphaHits = 0;
    double sunRayMs = 0, sunFilterMs = 0;
    bool sunDispatched = false;
    uint64_t waterRays = 0, shadedHits = 0;
    uint32_t lastWaterRays = 0, lastWaterHits = 0, lastShaded = 0, lastShadedTerrain = 0, lastShadedAlpha = 0;
    std::array<uint32_t, 5> surface{}; // eligible, vertex normal, face fallback, unavailable, invalid data
    bool hitDiagnostics = false, surfaceMeasured = false;
    bool waterDispatched = false;
    double waterRayMs = 0, waterFilterMs = 0;
    std::array<uint32_t, 8> giCounts{};
    std::array<uint32_t, 8> giSources{};
    std::array<uint32_t, 8> giSkyCounts{};
    std::array<uint32_t, 4> giHistoryCounts{};
    uint64_t giFrames = 0;
    bool giDispatched = false, giCompleted = false, giTemporal = false;
    double giRayMs = 0, giFilterMs = 0, giTemporalMs = 0, giCpuMs = 0;
    RTDynamicStats dynamic;
    std::map<uint64_t, VkTransformMatrixKHR> dynamicTransforms;
};

void VulkanContext::FailRTGI(std::string_view reason)
{
    _rtGIFailure = reason;
    _rtGIEnabled = false;
    _recreate = true;
    ResetTemporalHistory();
    if (_rtao)
    {
        _rtao->giDispatched = _rtao->giCompleted = false;
        _rtao->giHistoryValid = false;
        _rtao->scene.clear();
        for (auto& slot : _rtao->slots) slot.previous.clear();
    }
    std::fprintf(stderr, "RTGI unavailable: %s; preserving ordinary lighting\n", _rtGIFailure.c_str());
}

std::string VulkanContext::SetRTGI(std::string_view mode)
{
    if (mode == "status")
    {
        std::ostringstream out;
        out << "RTGI=" << (RTGIEnabled() ? "on" : "off") << " capability=" << _rtaoReason
            << " transport=one-bounce components=" << (_rtGIComponents==0?"primary-sky":
                _rtGIComponents==1?"primary-sky+sun-bounce":_rtGIComponents==2?"primary+secondary-sky":"sun+sky")
            << " skySource=" << (_weatherLighting.options[1]>.5f?"stock-dome+clouds":"stock-gradient");
        if (!_rtGIFailure.empty()) out << " fallback=" << _rtGIFailure;
        if (_rtao)
        {
            const auto& rt = *_rtao;
            const auto c = RTGIEnabled() && rt.giDispatched && rt.giCompleted ? rt.giCounts : std::array<uint32_t, 8>{};
            out << " lastWorldDispatch=" << (RTGIEnabled() && rt.giDispatched ? "yes" : "no")
                << " enhancedInstances=" << std::count_if(rt.scene.begin(),rt.scene.end(),[](const auto& instance) {
                    return std::any_of(instance.ranges.begin(),instance.ranges.end(),[](const auto& range) { return range.enhanced[3]>.5f && range.enhanced[3]<64; }); })
                << " reflectorInstances=" << std::count_if(rt.scene.begin(),rt.scene.end(),[](const auto& instance) {
                    return std::any_of(instance.ranges.begin(),instance.ranges.end(),[](const auto& range) { return range.enhanced[3]>.5f; }); })
                << " completedDispatches=" << rt.giFrames << " bounceRays=" << c[0] << " hits=" << c[1]
                << " eligibleHits=" << c[2] << " rejectedMaterials=" << c[3]
                << " sunRays=" << c[4] << " sunBlocked=" << c[5] << " eligibleReceivers=" << c[6]
                << " dynamicHits=" << c[7];
            const auto s=RTGIEnabled() && rt.giDispatched && rt.giCompleted ? rt.giSources : std::array<uint32_t,8>{};
            out << " sunVisible=" << c[4]-c[5] << " terrain(eligible/sunlit)=" << s[0] << '/' << s[1]
                << " stockPBR(eligible/sunlit)=" << s[2] << '/' << s[3]
                << " catalog(eligible/sunlit)=" << s[4] << '/' << s[5]
                << " backfacing=" << s[6] << " missingColor=" << s[7];
            const auto sky=RTGIEnabled() && rt.giDispatched && rt.giCompleted ? rt.giSkyCounts : std::array<uint32_t,8>{};
            out << " secondarySky(rays/blocked/visible/rejectedHemisphere)=" << sky[0] << '/' << sky[1] << '/' << sky[2] << '/' << sky[3]
                << " primarySky(rays/blocked/visible/belowHorizon)=" << sky[4] << '/' << sky[5] << '/' << sky[6] << '/' << sky[7];
            if (RTGIEnabled() && rt.giCompleted && rt.timingSamples)
                out << " GPUms(trace/filter/temporal/BLAS/TLAS)=" << rt.giRayMs << '/' << rt.giFilterMs << '/'
                    << rt.giTemporalMs << '/'
                    << rt.blasMs << '/' << rt.tlasMs << " CPUrecordMs=" << rt.giCpuMs;
            else out << " GPUtimings=unavailable";
            const auto h = RTGIEnabled() && rt.giCompleted && rt.giTemporal ? rt.giHistoryCounts : std::array<uint32_t, 4>{};
            out << " temporal=" << (RTGIEnabled() && rt.giTemporal ? "on" : "off")
                << " history(accepted/rejected/clamped/dynamic)=" << h[0] << '/' << h[1] << '/' << h[2] << '/' << h[3]
                << " historyResets=" << rt.giHistoryResets << " phase=" << rt.giPhase
                << " historyBytes=" << rt.giHistoryBytes << " historyAllocations=" << rt.giHistoryAllocations;
        }
        else out << " lastWorldDispatch=no completedDispatches=0 bounceRays=0 hits=0 eligibleHits=0"
                    " rejectedMaterials=0 sunRays=0 sunBlocked=0 eligibleReceivers=0 dynamicHits=0 GPUtimings=unavailable";
        out << "; last completed frame; one bounce, half resolution, 25m transport/45m receivers/80m sun+sky visibility; validated stock materials, bounded unmeasured reflectance; ambient replacement; unit transport gain";
        return out.str();
    }
    if (mode != "on" && mode != "off" && mode != "sky-only" && mode != "sun-only" && mode != "no-sun")
        return "INVALID: triRTGI on, off, sky-only, sun-only, no-sun, status";
    const unsigned components=mode=="sky-only"?0:mode=="sun-only"?1:mode=="no-sun"?2:3;
    if(mode!="off") mode="on";
    if (mode == "on" && !_rtaoAvailable) return "UNSUPPORTED: " + _rtaoReason;
    if (mode == "on" && !_rtGIFailure.empty()) return "UNSUPPORTED: " + _rtGIFailure;
    if (mode == "on")
    {
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(_physical, &features);
        if (!features.independentBlend) return "UNSUPPORTED: RTGI requires independent MRT blending";
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(_physical,&properties);
        if(properties.limits.maxColorAttachments<5) return "UNSUPPORTED: RTGI requires five world color attachments";
    }
    if (_rtGIEnabled != (mode == "on"))
    {
        _rtGIEnabled = mode == "on";
        _recreate = true;
        ResetTemporalHistory();
        if (_rtao)
        {
            _rtao->giDispatched = _rtao->giCompleted = false;
            _rtao->giHistoryValid = false;
            if (!RaySceneEnabled()) _rtao->scene.clear();
        }
    }
    if(_rtGIComponents!=components) ResetTemporalHistory();
    _rtGIComponents=components;
    return _rtGIEnabled ? "OK: one-bounce diffuse RTGI enabled (next frame)" : "OK: RTGI off";
}

std::string VulkanContext::SetRTDynamic(std::string_view mode)
{
    if (mode == "status")
    {
        std::ostringstream out;
        out << "RTDynamic=" << (_rtDynamicEnabled ? "on" : "off") << " active=" << (RTDynamicEnabled() ? "yes" : "no")
            << " capability=" << _rtaoReason;
        const auto stats = _rtao && RTDynamicEnabled() ? _rtao->dynamic : RTDynamicStats{};
        out << " instances=" << stats.instances << " updatedTransforms=" << stats.transforms
            << " BLASrebuilds=" << stats.rebuilds << " selected=" << stats.selected
            << " rasterPoses=" << (stats.captured > stats.fallback ? stats.captured - stats.fallback : 0)
            << " offscreenAnimate=" << stats.fallback << " unsupported=" << stats.unsupported
            << " budgetExcluded=" << stats.budget << " excludedProxies=" << stats.proxies
            << " vertices=" << stats.vertices << " uploadedBytes=" << stats.uploadedBytes
            << " hits(AO/sun/water/shaded)=" << stats.hits[0] << '/' << stats.hits[1] << '/' << stats.hits[2] << '/'
            << stats.hits[3] << " CPUms(collection/build)=" << stats.collectionMs << '/' << stats.buildMs;
        if (_rtao && _rtao->timingSamples)
            out << " sharedGPUms(BLAS/TLAS/AO/sun/water)=" << _rtao->blasMs << '/' << _rtao->tlasMs << '/'
                << _rtao->rayMs << '/' << _rtao->sunRayMs << '/' << _rtao->waterRayMs;
        else
            out << " GPUtimings=unavailable";
        out << "; last completed frame; limits=32 objects/12 people/65536 vertices/2048 scanned; "
               "proxies/crew, destroyed objects, blended/dynamic textures and terrain-fitted models excluded";
        return out.str();
    }
    if (mode != "on" && mode != "off")
        return "INVALID: triRTDynamic on, off, status";
    if (mode == "on" && !_rtaoAvailable)
        return "UNSUPPORTED: " + _rtaoReason;
    _rtDynamicEnabled = mode == "on";
    ResetTemporalHistory();
    if (_rtao)
    {
        std::erase_if(_rtao->scene, [](const auto& entry) { return entry.dynamicId != 0; });
        _rtao->dynamic = {};
        _rtao->dynamicTransforms.clear();
    }
    return _rtDynamicEnabled ? "OK: dynamic geometry enabled for the shared RT scene (next frame)"
                             : "OK: static-only RT restored";
}

void VulkanContext::BeginRTDynamic()
{
    if (!_rtao || !_frameOpen)
        return;
    auto& slot = _rtao->slots[_frame]; // BeginFrame already waited for this fence.
    slot.dynamic = {};
    for (auto& [id, mesh] : slot.dynamicMeshes)
        mesh.seen = false;
    if (!RTDynamicEnabled())
    {
        slot.dynamicMeshes.clear();
        _rtao->dynamic = {};
        _rtao->dynamicTransforms.clear();
    }
}

bool VulkanContext::AddRTDynamic(RTAOInstance instance, const void* vertices, size_t vertexBytes, const void* indices,
                                 size_t indexBytes)
{
    if (!RTDynamicEnabled() || !_frameOpen || !_rtao || !instance.dynamicId)
        return false;
    auto& slot = _rtao->slots[_frame];
    if (_rtao->scene.size() >= 1024 || slot.dynamic.instances >= 32 ||
        (!instance.mesh &&
         (!vertexBytes || vertexBytes > 8192 * 32 || vertexBytes % 32 || !indexBytes || indexBytes > 196608)))
    {
        ++slot.dynamic.budget;
        return false;
    }
    auto& mesh = slot.dynamicMeshes[instance.dynamicId];
    if (mesh.seen)
        return false;
    mesh.seen = true;
    if (instance.mesh)
    {
        mesh.mesh = instance.mesh; // existing immutable raster geometry / shared BLAS
        mesh.vertices.clear();
        mesh.indices.clear();
        mesh.deformed = false;
        mesh.revision = 0;
    }
    else
    {
        const bool verticesChanged =
            mesh.vertices.size() != vertexBytes || std::memcmp(mesh.vertices.data(), vertices, vertexBytes) != 0;
        const bool indicesChanged =
            mesh.indices.size() != indexBytes || std::memcmp(mesh.indices.data(), indices, indexBytes) != 0;
        bool positionsChanged = mesh.vertices.size() != vertexBytes;
        for (size_t i = 0; !positionsChanged && i < vertexBytes; i += 32)
            positionsChanged = std::memcmp(mesh.vertices.data() + i, static_cast<const char*>(vertices) + i, 12) != 0;
        if (!mesh.deformed || !mesh.mesh || mesh.mesh->vertices.size < vertexBytes ||
            mesh.mesh->indices.size < indexBytes)
        {
            mesh.mesh = UploadMesh(vertices, vertexBytes, indices, indexBytes);
            slot.dynamic.uploadedBytes += uint32_t(vertexBytes + indexBytes);
        }
        else
        {
            // These allocations are exclusive to this fence slot, including all BLAS
            // and material references. No other in-flight frame ever queries them.
            if (verticesChanged)
            {
                UploadMappedBuffer(mesh.mesh->vertices, vertices, vertexBytes);
                slot.dynamic.uploadedBytes += uint32_t(vertexBytes);
            }
            if (indicesChanged)
            {
                UploadMappedBuffer(mesh.mesh->indices, indices, indexBytes);
                slot.dynamic.uploadedBytes += uint32_t(indexBytes);
            }
        }
        if (positionsChanged || indicesChanged)
            ++mesh.revision;
        if (verticesChanged)
            mesh.vertices.assign(static_cast<const unsigned char*>(vertices),
                                 static_cast<const unsigned char*>(vertices) + vertexBytes);
        if (indicesChanged)
            mesh.indices.assign(static_cast<const unsigned char*>(indices),
                                static_cast<const unsigned char*>(indices) + indexBytes);
        mesh.deformed = true;
    }
    instance.mesh = mesh.mesh;
    instance.kind = 3;
    instance.revision = mesh.revision;
    auto& transform = _rtao->dynamicTransforms[instance.dynamicId];
    if (std::memcmp(&transform, &instance.transform, sizeof(transform)) != 0)
        ++slot.dynamic.transforms;
    transform = instance.transform;
    ++slot.dynamic.instances;
    _rtao->scene.push_back(std::move(instance));
    return true;
}

void VulkanContext::SetRTDynamicStats(const RTDynamicStats& stats)
{
    if (!_rtao || !_frameOpen)
        return;
    auto& slot = _rtao->slots[_frame];
    const auto actual = slot.dynamic;
    slot.dynamic = stats;
    slot.dynamic.instances = actual.instances;
    slot.dynamic.transforms = actual.transforms;
    slot.dynamic.uploadedBytes = actual.uploadedBytes;
    slot.dynamic.budget += actual.budget;
    std::erase_if(slot.dynamicMeshes, [](const auto& pair) { return !pair.second.seen; });
    std::erase_if(_rtao->dynamicTransforms, [&](const auto& pair) { return !slot.dynamicMeshes.contains(pair.first); });
}

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
        if (_rtao && _rtao->hitDiagnostics && _rtao->surfaceMeasured && RTReflectionsEnabled())
            out << " hitSurface(eligible/vertex/face/unavailable/invalid)="
                << _rtao->surface[0] << '/' << _rtao->surface[1] << '/' << _rtao->surface[2] << '/'
                << _rtao->surface[3] << '/' << _rtao->surface[4];
        else
            out << " hitSurface=unavailable (" << (_rtao && _rtao->hitDiagnostics
                ? "no completed water diagnostic dispatch" : "diagnostics disabled") << ')';
        if (_rtao && _rtao->timingSamples)
            out << " lastGPUms(traceShade/filter)=" << _rtao->waterRayMs << '/' << _rtao->waterFilterMs;
        else
            out << " GPUtimings=unavailable";
        out << "; shared " << SetRTAO("status")
            << "; actual water only, 80m receivers/70m rays, textured hits and weather sky fallback";
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

std::string VulkanContext::SetSoftRTShadows(std::string_view mode)
{
    if (mode == "status")
    {
        std::ostringstream out;
        out << "SoftRTShadows=" << (_softRTShadowsEnabled ? "on" : "off")
            << " eligible=" << (SoftRTShadowsEnabled() ? "yes" : "no")
            << " RTShadows=" << (RTShadowsEnabled() ? "on" : "off") << " capability=" << _rtaoReason
            << " samples=" << (SoftRTShadowsEnabled() ? 4 : 1)
            << " sunHalfAngle=0.4deg sampling=world-anchored-disc PCSS=independent";
        if (_rtao)
        {
            out << " lastCompletedSamples=" << _rtao->lastSunSamples << " lastCompletedRays=" << _rtao->lastSunRays
                << " lastCompletedHits=" << _rtao->lastSunHits
                << " lastWorldDispatch=" << (_rtao->sunDispatched ? "yes" : "no");
            if (_rtao->timingSamples)
                out << " lastGPUms(rays/compose)=" << _rtao->sunRayMs << '/' << _rtao->sunFilterMs;
        }
        else
            out << " lastCompletedSamples=0 lastCompletedRays=0 lastWorldDispatch=no";
        return out.str();
    }
    if (mode != "on" && mode != "off")
        return "INVALID: triSoftRTShadows on, off, status";
    if (mode == "on" && !_rtaoAvailable)
        return "UNSUPPORTED: " + _rtaoReason;
    if (mode == "on" && !RTShadowsEnabled())
        return "UNSUPPORTED: enable triRTShadows first; prerequisites are not enabled implicitly";
    if (_softRTShadowsEnabled != (mode == "on"))
    {
        FlushScreenBatch();
        _softRTShadowsEnabled = mode == "on";
        ResetTemporalHistory();
    }
    return _softRTShadowsEnabled ? "OK: soft RT sunlight enabled (four sun-disc rays), PCSS unchanged"
                                 : "OK: original single-ray RT sunlight restored";
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
    // Developer-only, process-local opt-in. Specialize the existing pipelines;
    // production water does not execute full reconstruction or diagnostic atomics.
    const auto* diagnostics = std::getenv("CWR_VK_RT_HIT_DIAGNOSTICS");
    rt.hitDiagnostics = diagnostics && std::string_view(diagnostics) == "1";
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
        out << "; shared static LOD0 scene radius=" << (RTShadowsEnabled() || RTHitDataEnabled() ? 160 : 80)
            << "m; dynamic=" << (_rtDynamicEnabled ? "on" : "off")
            << "; AO radius=4m includes bounded terrain, excludes foliage cards; ambient-only composition; "
               "static collection excludes animation/damage; blended "
               "effects/proxies excluded";
        return out.str();
    }
    if (mode != "on" && mode != "off")
        return "INVALID: triRTAO on, off, status";
    if (mode == "on" && !_rtaoAvailable)
        return "UNSUPPORTED: " + _rtaoReason;
    if (mode == "on")
    {
        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceFeatures(_physical, &features);
        vkGetPhysicalDeviceProperties(_physical, &properties);
        if (!features.independentBlend || properties.limits.maxColorAttachments < 5)
            return "UNSUPPORTED: RTAO ambient capture requires independent blending and five color attachments";
    }
    if (_rtaoEnabled != (mode == "on")) _recreate = true;
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
        rt.lastSunSamples = slot.sunSamples;
        rt.totalSunRays += counts[2];
        rt.totalSunHits += counts[3];
        rt.lastTerrainHits = counts[4];
        rt.lastAlphaHits = counts[5];
        rt.lastWaterRays = counts[6];
        rt.lastWaterHits = counts[7];
        rt.lastShaded = counts[8];
        rt.lastShadedTerrain = counts[9];
        rt.lastShadedAlpha = counts[10];
        std::copy_n(counts + 15, 5, rt.surface.begin());
        rt.surfaceMeasured = slot.surfaceMeasured;
        std::copy_n(counts + 20, 8, rt.giCounts.begin());
        std::copy_n(counts + 32, 8, rt.giSources.begin());
        std::copy_n(counts + 40, 8, rt.giSkyCounts.begin());
        rt.giCompleted = slot.giDispatched;
        rt.giTemporal = slot.giTemporal;
        std::copy_n(counts + 28, 4, rt.giHistoryCounts.begin());
        rt.giFrames += slot.giDispatched;
        rt.giCpuMs = slot.giCpuMs;
        rt.waterRays += counts[6];
        rt.shadedHits += counts[8];
        ++rt.completed;
        rt.blasBuilds += slot.built;
        rt.tlasBuilds += slot.tlasBuilt;
        rt.dynamic = slot.dynamic;
        std::copy_n(counts + 11, 4, rt.dynamic.hits.begin());
        if (slot.queries)
        {
            uint64_t stamps[22]{};
            if (vkGetQueryPoolResults(_device, slot.queries, 0, 22, sizeof(stamps), stamps, sizeof(uint64_t),
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
                rt.giRayMs = ms(16);
                rt.giFilterMs = ms(18);
                rt.giTemporalMs = ms(20);
                if (slot.giDispatched && rt.completed % 120 == 0)
                    std::fprintf(stderr, "Vulkan RTGI GPU: trace=%.4f filter=%.4f temporal=%.4f CPUrecord=%.4f ms; rays=%u hits=%u eligible=%u rejected=%u sunRays=%u sunBlocked=%u receivers=%u dynamic=%u\n",
                        rt.giRayMs, rt.giFilterMs, rt.giTemporalMs, rt.giCpuMs, counts[20], counts[21], counts[22], counts[23],
                        counts[24], counts[25], counts[26], counts[27]);
                ++rt.timingSamples;
                uint64_t frame[2]{};
                if (_frames[_frame].frameQueries &&
                    vkGetQueryPoolResults(_device, _frames[_frame].frameQueries, 0, 2, sizeof(frame), frame,
                                          sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
                    rt.frameMs = double((frame[1] - frame[0]) & mask) * _timestampPeriod / 1e6;
                if ((!_rtDynamicEnabled && (slot.built || slot.tlasBuilt)) || rt.completed % 120 == 0)
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
                if (_rtDynamicEnabled && rt.completed % 120 == 0)
                    std::fprintf(stderr,
                                 "Vulkan RT dynamic: instances=%u transforms=%u rebuilds=%u vertices=%u bytes=%u "
                                 "CPUcollection=%.4f CPUbuild=%.4f BLAS=%.4f TLAS=%.4f AO=%.4f sun=%.4f water=%.4f "
                                 "frame=%.4f ms\n",
                                 rt.dynamic.instances, rt.dynamic.transforms, rt.dynamic.rebuilds, rt.dynamic.vertices,
                                 rt.dynamic.uploadedBytes, rt.dynamic.collectionMs, rt.dynamic.buildMs, rt.blasMs,
                                 rt.tlasMs, rt.rayMs, rt.sunRayMs, rt.waterRayMs, rt.frameMs);
            }
        }
        slot.pending = false;
    }
    slot.buildScratch.clear();
}

void VulkanContext::DrawRTAO(const std::array<float, 4>& projection, bool& ssaoHandled)
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
    slot.sunSamples = 0;
    rt.waterDispatched = false;
    rt.giDispatched = slot.giDispatched = false;
    slot.giTemporal = false;
    slot.giCpuMs = 0;
    if (rt.scene.empty())
    {
        rt.giHistoryValid = false;
        rt.dynamic = slot.dynamic;
        return;
    }
    const VkExtent2D half{(_worldExtent.width + 1) / 2, (_worldExtent.height + 1) / 2};
    const bool giTemporal = RTGIEnabled() && IsTemporal(_aaMode);
    if (!rt.layout)
    {
        if (rt.setLayout)
            vkDestroyDescriptorSetLayout(_device, rt.setLayout, nullptr);
        rt.setLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayoutBinding bindings[14]{};
        const VkDescriptorType types[]{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
        for (uint32_t i = 0; i < 14; ++i)
            bindings[i] = {i, types[i], 1, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        set.bindingCount = 14;
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
    if (std::any_of(rt.compute.begin(), rt.compute.begin() + (RTGIEnabled() ? 8 : _rtWaterTargets ? 6 : 4), [](auto p) { return !p; }) ||
        (giTemporal && !rt.giResolve))
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
        for (int i = 0; i < 8; ++i)
        {
            if (rt.compute[i] || (i >= 6 ? !RTGIEnabled() : i >= 4 && !_rtWaterTargets))
                continue;
            const uint32_t* codes[]{Cwrrtao_comp,        Cwrrtao_ms_comp, Cwrrtshadow_comp,
                                    Cwrrtshadow_ms_comp, Cwrrtwater_comp, Cwrrtwater_ms_comp, Cwrrtgi_comp, Cwrrtgi_ms_comp};
            const size_t sizes[]{sizeof(Cwrrtao_comp),        sizeof(Cwrrtao_ms_comp), sizeof(Cwrrtshadow_comp),
                                 sizeof(Cwrrtshadow_ms_comp), sizeof(Cwrrtwater_comp), sizeof(Cwrrtwater_ms_comp), sizeof(Cwrrtgi_comp), sizeof(Cwrrtgi_ms_comp)};
            auto module = shader(codes[i], sizes[i]);
            VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
            info.layout = rt.layout;
            info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            info.stage.module = module;
            info.stage.pName = "main";
            const VkBool32 diagnostics = rt.hitDiagnostics ? VK_TRUE : VK_FALSE;
            const VkSpecializationMapEntry diagnosticEntry{0, 0, sizeof(diagnostics)};
            const VkSpecializationInfo specialization{1, &diagnosticEntry, sizeof(diagnostics), &diagnostics};
            if (i >= 4 && i < 6)
                info.stage.pSpecializationInfo = &specialization;
            auto result = vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &info, nullptr, &rt.compute[i]);
            vkDestroyShaderModule(_device, module, nullptr);
            Require(result, "create ray query pipeline");
        }
        if (giTemporal && !rt.giResolve)
        {
            auto module = shader(Cwrrtgi_resolve_comp, sizeof(Cwrrtgi_resolve_comp));
            VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
            info.layout = rt.layout;
            info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            info.stage.module = module;
            info.stage.pName = "main";
            const auto result = vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &info, nullptr, &rt.giResolve);
            vkDestroyShaderModule(_device, module, nullptr);
            Require(result, "create GI history pipeline");
        }
    }
    if (std::any_of(rt.filter.begin(), rt.filter.begin() + (RTGIEnabled() ? 8 : _rtWaterTargets ? 6 : 4), [](auto p) { return !p; }))
    {
        VkShaderModuleCreateInfo shader{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shader.codeSize = sizeof(Cwrgamma_vert);
        shader.pCode = Cwrgamma_vert;
        VkShaderModule vertex = VK_NULL_HANDLE;
        Require(vkCreateShaderModule(_device, &shader, nullptr, &vertex), "create fullscreen vertex");
        for (int i = 0; i < 8; ++i)
        {
            if (rt.filter[i] || (i >= 6 ? !RTGIEnabled() : i >= 4 && !_rtWaterTargets))
                continue;
            const uint32_t* codes[]{Cwrrtao_filter_frag,        Cwrrtao_filter_ms_frag, Cwrrtshadow_filter_frag,
                                    Cwrrtshadow_filter_ms_frag, Cwrrtwater_filter_frag, Cwrrtwater_filter_ms_frag, Cwrrtgi_filter_frag, Cwrrtgi_filter_ms_frag};
            const size_t sizes[]{sizeof(Cwrrtao_filter_frag),     sizeof(Cwrrtao_filter_ms_frag),
                                 sizeof(Cwrrtshadow_filter_frag), sizeof(Cwrrtshadow_filter_ms_frag),
                                 sizeof(Cwrrtwater_filter_frag),  sizeof(Cwrrtwater_filter_ms_frag), sizeof(Cwrrtgi_filter_frag), sizeof(Cwrrtgi_filter_ms_frag)};
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
            output.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            output.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            output.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
            output.alphaBlendOp = VK_BLEND_OP_ADD;
            output.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            if (i >= 4)
            {
                output.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                output.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                output.colorBlendOp = VK_BLEND_OP_ADD;
            }
            output.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            if (i >= 6)
            {
                output.srcColorBlendFactor = output.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                output.colorBlendOp = VK_BLEND_OP_ADD;
            }
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
        VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32},
                                     {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 4},
                                     {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8},
                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 12}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = 4;
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
        Require(vkAllocateDescriptorSets(_device, &allocate, &slot.giSet), "allocate GI descriptors");
        CreateAAImage(slot.ao, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, half);
        slot.extent = half;
        if (RTGIEnabled())
            CreateAAImage(slot.gi, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, half);
        if (RTShadowsEnabled())
            CreateAAImage(slot.sunlight, VK_FORMAT_R32G32B32A32_SFLOAT,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, _worldExtent);
        if (RTReflectionsEnabled())
            CreateAAImage(slot.reflection, VK_FORMAT_R32G32B32A32_SFLOAT,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT, _worldExtent);
    }
    if (giTemporal && !rt.giHistory[0].image)
    {
        for (auto& history : rt.giHistory)
        {
            CreateAAImage(history, VK_FORMAT_R16G16B16A16_SFLOAT,
                          VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT, half);
            VkMemoryRequirements memory{};
            vkGetImageMemoryRequirements(_device, history.image, &memory);
            rt.giHistoryBytes += memory.size;
            ++rt.giHistoryAllocations;
        }
    }
    std::array<float, 25> giLighting{};
    for (int i = 0; i < 6; ++i) giLighting[i] = _reflectionLighting[i / 3][i % 3];
    std::copy(_rtSun.begin(), _rtSun.end(), giLighting.begin() + 6);
    giLighting[10] = _weatherLighting.cloudColor[3];
    giLighting[11] = _weatherLighting.sun[3];
    for(int i=0;i<9;++i) giLighting[12+i]=_reflectionLighting[2+i/3][i%3];
    std::copy_n(_weatherLighting.cloudColor.begin(),3,giLighting.begin()+21);
    giLighting[24]=_weatherLighting.options[1];
    bool lightingChanged = rt.giSkyImage != _weatherImages[3].get();
    for (int i = 0; i < 25; ++i)
    {
        const float threshold = i < 6 || (i>=12 && i<24) ? std::max(.002f, std::abs(rt.giLighting[i]) * .05f) : i < 10 ? .01f : .05f;
        lightingChanged |= std::abs(giLighting[i] - rt.giLighting[i]) > threshold;
    }
    // Camera cuts/FOV changes already invalidate TAA/FSR. Also reject a missed
    // GI frame, new lighting/weather, or the absence of the preceding raw slot.
    if (!_taaValid || lightingChanged || rt.giPreviousSlot >= FramesInFlight || rt.giPreviousSlot == _frame ||
        _taaFrame != rt.giLastFrame + 1)
        rt.giHistoryValid = false;
    if (giTemporal && !rt.giHistoryValid) ++rt.giHistoryResets;
    if (!giTemporal) rt.giHistoryValid = false;
    rt.giLighting = giLighting;
    rt.giSkyImage = _weatherImages[3].get();
    auto buffer = [&](VkDeviceSize size, VkBufferUsageFlags usage, bool host = false)
    {
        auto result = std::make_shared<RTBuffer>();
        result->Create(_physical, _device, size, usage, host);
        return result;
    };
    if (!slot.counters)
        slot.counters = buffer(192, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    if (!slot.materials)
        slot.materials = buffer(1024 * sizeof(uint64_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    if (!slot.reflectionLight)
        slot.reflectionLight = buffer(sizeof(_reflectionLighting), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    UploadMappedBuffer(slot.reflectionLight->buffer, _reflectionLighting.data(), sizeof(_reflectionLighting));
    std::memset(slot.counters->buffer.mapped, 0, 192);
    if (!slot.queries && _timestampBits && _profile.enabled)
    {
        VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = 22;
        Require(vkCreateQueryPool(_device, &info, nullptr, &slot.queries), "create GPU timestamps");
    }
    FlushScreenBatch();
    if (_worldPassOpen)
        vkCmdEndRenderPass(command);
    _worldPassOpen = false;
    _commands = {};
    if (slot.queries)
        vkCmdResetQueryPool(command, slot.queries, 0, 22);
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
        if (as.handle && as.storage.buffer.size >= size)
            return;
        if (as.handle)
            as.destroy(as.device, as.handle, nullptr);
        as.handle = VK_NULL_HANDLE;
        DestroyBuffer(_device, as.storage.buffer);
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
    const auto buildStart = std::chrono::steady_clock::now();
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    stamp(0);
    for (const auto& entry : rt.scene)
    {
        // A shared mesh can have more than one conservative material subset.
        // Keep each variant, rather than rebuilding when instances alternate subsets.
        auto found = std::find_if(rt.cache.begin(), rt.cache.end(),
                                  [&](const auto& blas)
                                  {
                                      return blas && blas->mesh == entry.mesh &&
                                             (entry.dynamicGeometry || blas->ranges == entry.ranges) &&
                                             blas->index16 == entry.index16;
                                  });
        if (found == rt.cache.end())
        {
            rt.cache.emplace_back();
            found = rt.cache.end() - 1;
        }
        auto& cached = *found;
        if (!cached ||
            (entry.dynamicGeometry && (cached->revision != entry.revision || cached->ranges != entry.ranges)))
        {
            const bool rebuild =
                !cached || cached->revision != entry.revision || cached->ranges.size() != entry.ranges.size() ||
                !std::equal(entry.ranges.begin(), entry.ranges.end(), cached->ranges.begin(),
                            [](const auto& a, const auto& b)
                            { return a.first == b.first && a.count == b.count && bool(a.alpha) == bool(b.alpha); });
            auto blas = cached ? cached : std::make_shared<BLAS>();
            blas->mesh = entry.mesh;
            blas->ranges = entry.ranges;
            blas->index16 = entry.index16;
            blas->dynamicId = entry.dynamicGeometry ? entry.dynamicId : 0;
            blas->revision = entry.revision;
            blas->triangles = 0;
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
                     range.color ? range.color->rayWidth : 0, range.color ? range.color->rayHeight : 0,
                     range.normalRoughness ? Address(_device, range.normalRoughness->rayAlpha) : 0,
                     range.normalRoughness ? range.normalRoughness->rayWidth : 0,
                     range.normalRoughness ? range.normalRoughness->rayHeight : 0, range.enhanced});
            }
            if (geometries.empty() || geometries.size() > 1024 || blas->triangles > 1000000)
                continue;
            if (blas->materials.buffer.size < materials.size() * sizeof(RayMaterial))
            {
                DestroyBuffer(_device, blas->materials.buffer);
                blas->materials.Create(_physical, _device, materials.size() * sizeof(RayMaterial),
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
            }
            UploadMappedBuffer(blas->materials.buffer, materials.data(), materials.size() * sizeof(RayMaterial));
            if (rebuild)
            {
                VkAccelerationStructureBuildGeometryInfoKHR info{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
                info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                info.flags = entry.dynamicGeometry ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
                                                   : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
                info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                info.geometryCount = uint32_t(geometries.size());
                info.pGeometries = geometries.data();
                VkAccelerationStructureBuildSizesInfoKHR sizes{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
                rt.sizes(_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info, counts.data(), &sizes);
                allocateAS(*blas, info.type, sizes.accelerationStructureSize);
                auto scratch = blas->scratch;
                if (!scratch || scratch->buffer.size < sizes.buildScratchSize + rt.alignment)
                    scratch = buffer(sizes.buildScratchSize + rt.alignment, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
                if (entry.dynamicGeometry)
                    blas->scratch = scratch;
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
                if (entry.dynamicId)
                    ++slot.dynamic.rebuilds;
            }
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
    const bool changed = slot.dynamic.rebuilds != 0 || instances.size() != slot.previous.size() ||
                         (!instances.empty() && std::memcmp(instances.data(), slot.previous.data(),
                                                            instances.size() * sizeof(instances[0])) != 0);
    if (!slot.tlas || changed)
    {
        // Per-frame-slot ownership: replacing TLAS/scratch never races an in-flight query.
        if (!_rtDynamicEnabled)
        {
            slot.tlas.reset();
            slot.scratch.reset();
            slot.instances.reset();
        }
        const auto instanceBytes = std::max<size_t>(1, instances.size()) * sizeof(VkAccelerationStructureInstanceKHR);
        if (!slot.instances || slot.instances->buffer.size < instanceBytes)
            slot.instances =
                buffer(instanceBytes, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true);
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
        if (!slot.tlas)
            slot.tlas = std::make_shared<Acceleration>();
        allocateAS(*slot.tlas, info.type, sizes.accelerationStructureSize);
        if (!slot.scratch || slot.scratch->buffer.size < sizes.buildScratchSize + rt.alignment)
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
    slot.dynamic.buildMs =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - buildStart).count();
    // Cache only local working geometry. Frame slots keep evicted entries alive
    // until every TLAS that references them has completed and been replaced.
    std::erase_if(rt.cache, [&](const auto& item)
                  { return !item || item->used + (item->dynamicId ? FramesInFlight : 120) < rt.serial; });
    // Shared TLAS; compose GI after AO so AO does not count the bounce's
    // visibility a second time. All contributions precede AA and tone mapping.
    for (int effect : {1, 2, 0, 3})
    {
        // Contact-only sunlight must also precede reflections and AO when the
        // shared RT scene is used without hardware sunlight shadows.
        if (effect != 1 && !_contactDrew)
            DrawContactShadows(projection);
        const uint32_t rayStamp = 4 + effect * 4, filterStamp = rayStamp + 2;
        if (effect == 3 ? !RTGIEnabled()
            : effect == 2 ? !(RTReflectionsEnabled() && _rtWaterTargets)
            : effect    ? !(RTShadowsEnabled() && _rtShadowTargets)
                        : !RTAOEnabled())
        {
            stamp(rayStamp);
            stamp(rayStamp + 1);
            stamp(filterStamp);
            stamp(filterStamp + 1);
            if (effect == 3) { stamp(20); stamp(21); }
            continue;
        }
        if (effect == 3 && !RTAOEnabled())
        {
            // Subtract captured sunlight before multiplying by SSAO, then add GI.
            // Report completion even if a later GI failure takes the caller's fallback.
            DrawSSAO(projection);
            ssaoHandled = true;
        }
        const double cpuStart = _profile.enabled ? ProfileClock() : 0;
        const auto set = effect == 3 ? slot.giSet : effect == 2 ? slot.waterSet : effect ? slot.sunSet : slot.set;
        auto& target = effect == 3 ? slot.gi : effect == 2 ? slot.reflection : effect ? slot.sunlight : slot.ao;
        const auto dispatchExtent = effect == 0 || effect == 3 ? half : _worldExtent;
        VkDescriptorImageInfo images[3]{
            {_aaSampler, _aaTargets[_image].sampledDepth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL},
            {VK_NULL_HANDLE, target.view, VK_IMAGE_LAYOUT_GENERAL},
            {_aaSampler, target.view, VK_IMAGE_LAYOUT_GENERAL}};
        VkDescriptorImageInfo direct{
            _aaSampler, effect == 0 || effect == 3 ? _aaTargets[_image].ambient.view :
                _rtShadowTargets ? _aaTargets[_image].sunlight.view : _aaTargets[_image].color.view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo water{_aaSampler,
                                    _rtWaterTargets ? _aaTargets[_image].water.view : _aaTargets[_image].color.view,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorBufferInfo reflectionLight{slot.reflectionLight->buffer.buffer, 0, VK_WHOLE_SIZE};
        VkDescriptorBufferInfo counter{slot.counters->buffer.buffer, 0, slot.counters->buffer.size};
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
        if (effect == 3)
        {
            const auto oldRaw = rt.giPreviousSlot < FramesInFlight && rt.slots[rt.giPreviousSlot].gi.view ?
                rt.slots[rt.giPreviousSlot].gi.view : slot.gi.view;
            const auto output = giTemporal ? rt.giHistory[rt.giHistoryIndex].view : slot.gi.view;
            VkDescriptorImageInfo giImages[]{
                {_aaSampler, giTemporal ? _aaTargets[_image].motion.view : slot.gi.view,
                 giTemporal ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL},
                {_aaSampler, giTemporal ? rt.giHistory[1 - rt.giHistoryIndex].view : slot.gi.view, VK_IMAGE_LAYOUT_GENERAL},
                {VK_NULL_HANDLE, output, VK_IMAGE_LAYOUT_GENERAL},
                {_aaSampler, output, VK_IMAGE_LAYOUT_GENERAL},
                {_aaSampler, oldRaw, VK_IMAGE_LAYOUT_GENERAL}};
            VkWriteDescriptorSet giWrites[5]{};
            for (unsigned i = 0; i < 5; ++i)
            {
                giWrites[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                giWrites[i].dstSet = set;
                giWrites[i].dstBinding = 9 + i;
                giWrites[i].descriptorCount = 1;
                giWrites[i].descriptorType = i == 2 ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                giWrites[i].pImageInfo = &giImages[i];
            }
            vkUpdateDescriptorSets(_device, 5, giWrites, 0, nullptr);
            if (giTemporal)
            {
                VkImageMemoryBarrier histories[2]{};
                for (unsigned i = 0; i < 2; ++i)
                {
                    auto& b = histories[i];
                    b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                    b.oldLayout = rt.giHistoryInitialized ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
                    b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    b.srcAccessMask = rt.giHistoryInitialized ? VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT : 0;
                    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    b.image = rt.giHistory[i].image;
                    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                }
                vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, histories);
                rt.giHistoryInitialized = true;
            }
        }
        VkImageMemoryBarrier image{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        image.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        image.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        image.srcAccessMask = effect == 3 ? VK_ACCESS_SHADER_READ_BIT : 0;
        image.srcQueueFamilyIndex = image.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image.image = target.image;
        image.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(command, effect == 3 ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &image);
        const int ms = (_worldSamples != VK_SAMPLE_COUNT_1_BIT) + effect * 2;
        Push push;
        push.camera = rt.camera;
        push.projection = projection;
        push.sun = _rtSun;
        // Reach ground from standing-height wall/vehicle receivers and retain
        // nearby structures beyond SSAO's short contact neighborhood. Strength,
        // surface bias, ray count and the 60m receiver bound remain unchanged.
        if (effect == 0) push.settings[0] = 4.f;
        if (effect)
            push.settings[3] = 50.f;
        if (effect == 3)
        {
            push.settings = {25.f, .02f, float(_rtGIComponents), 45.f};
            rt.giPhase = _taaFrame % 32;
            push.jitter[2] = giTemporal ? float(1 + rt.giPhase) : 0.f;
            push.jitter[3] = giTemporal && rt.giHistoryValid ? 1.f : 0.f;
        }
        if (effect == 1)
        {
            // Spare component: preserve the 128-byte push ABI and all AO/water settings.
            push.jitter[2] = SoftRTShadowsEnabled() ? 1.f : 0.f;
            slot.sunSamples = SoftRTShadowsEnabled() ? 4 : 1;
        }
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
        ready.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &ready, 0,
                             nullptr, 0, nullptr);
        if (effect == 3)
        {
            stamp(20);
            if (giTemporal)
            {
                vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, rt.giResolve);
                vkCmdDispatch(command, (half.width + 7) / 8, (half.height + 7) / 8, 1);
                vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
                    0, 1, &ready, 0, nullptr, 0, nullptr);
                rt.giHistoryValid = true;
                rt.giLastFrame = _taaFrame;
                rt.giHistoryIndex = 1 - rt.giHistoryIndex;
                rt.giPreviousSlot = _frame;
            }
            stamp(21);
        }
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
        if (effect == 3)
        {
            rt.giDispatched = slot.giDispatched = true;
            slot.giTemporal = giTemporal;
            slot.giCpuMs = _profile.enabled ? ProfileClock() - cpuStart : 0;
        }
        else if (effect == 2)
            rt.waterDispatched = true;
        else if (effect)
            rt.sunDispatched = true;
        else
            rt.worldDispatched = true;
    }
    slot.surfaceMeasured = rt.hitDiagnostics && rt.waterDispatched;
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
        slot.giSet = VK_NULL_HANDLE;
        if (slot.gi.view) vkDestroyImageView(_device, slot.gi.view, nullptr);
        if (slot.gi.image) vkDestroyImage(_device, slot.gi.image, nullptr);
        if (slot.gi.memory) vkFreeMemory(_device, slot.gi.memory, nullptr);
        slot.gi = {};
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
    for (auto& history : _rtao->giHistory)
    {
        if (history.view) vkDestroyImageView(_device, history.view, nullptr);
        if (history.image) vkDestroyImage(_device, history.image, nullptr);
        if (history.memory) vkFreeMemory(_device, history.memory, nullptr);
        history = {};
    }
    _rtao->giHistoryBytes = 0;
    _rtao->giHistoryValid = _rtao->giHistoryInitialized = false;
    _rtao->giHistoryIndex = 0;
    _rtao->giPreviousSlot = FramesInFlight;
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
    if (_rtao->giResolve) vkDestroyPipeline(_device, _rtao->giResolve, nullptr);
    if (_rtao->layout)
        vkDestroyPipelineLayout(_device, _rtao->layout, nullptr);
    if (_rtao->setLayout)
        vkDestroyDescriptorSetLayout(_device, _rtao->setLayout, nullptr);
    _rtao.reset();
    _rtaoAvailable = _rtaoEnabled = false;
}
} // namespace Poseidon::vk
