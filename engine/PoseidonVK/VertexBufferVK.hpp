#pragma once

#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/TerrainRayVK.hpp>
#include <PoseidonVK/MaterialQualificationVK.hpp>

namespace Poseidon
{
class VertexBufferVK final : public VertexBuffer
{
  public:
    VertexBufferVK(vk::VulkanContext& context, const Shape& shape, VBType type);
    void Update(const Shape&, bool dynamic) override;
    const std::vector<vk::SectionRange>& Sections() const { return _sections; }
    const std::shared_ptr<vk::MeshBuffers>& Buffers() const { return _buffers; }
    const std::vector<vk::TerrainReceiverRange>& TerrainReceivers();
    const std::vector<vk::MaterialRange>& MaterialRanges(const Shape&);

  private:
    vk::VulkanContext& _context;
    std::vector<vk::SectionRange> _sections;
    std::shared_ptr<vk::MeshBuffers> _buffers;
    std::vector<vk::TerrainReceiverRange> _terrainReceivers;
    bool _terrainReceiversReady = false;
    std::vector<vk::MaterialRange> _materialRanges;
};
} // namespace Poseidon
