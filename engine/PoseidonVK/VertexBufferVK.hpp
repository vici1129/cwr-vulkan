#pragma once

#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <PoseidonVK/VulkanContext.hpp>
#include <PoseidonVK/TerrainRayVK.hpp>

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

  private:
    vk::VulkanContext& _context;
    std::vector<vk::SectionRange> _sections;
    std::shared_ptr<vk::MeshBuffers> _buffers;
    std::vector<vk::TerrainReceiverRange> _terrainReceivers;
    bool _terrainReceiversReady = false;
};
} // namespace Poseidon
