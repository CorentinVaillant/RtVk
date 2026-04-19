#pragma once

#include "Buffer.h"
#include "graphics/Buffer.h"
#include "graphics/vma_usage.h"
#include "graphics/vulkan_context.h"
#include "hittables/TriangleRef.h"
#include "renderer/renderer_utils.h"
#include "types.h"
#include <cstdint>
#include <optional>

#include <volk.h>
#include <vulkan/vulkan_core.h>

class Blas {
  // -- Attributs
private:
  static constexpr VkBufferUsageFlags BLAS_BUFFER_USAGE =
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  VkDevice _ctxDevice;

public:
  std::optional<Buffer<>> _geoBuffer;
  std::optional<BlasBuffer<uint8_t>> _blasBuffer;
  VkAccelerationStructureKHR _blas = VK_NULL_HANDLE;

  uint32_t _instanceCustomIndex = 0;
  glm::mat4 _transform;

  // -- Constructors
public:
  NO_COPY(Blas);

  Blas() = delete;

  Blas(VulkanContext &ctx, std::span<VkAabbPositionsKHR> aabbs,
       glm::mat4 transform)
      : _ctxDevice(ctx._device), _geoBuffer(upload_buffer(ctx, aabbs)),
        _blasBuffer(std::nullopt), _transform(transform) {

    VkDeviceOrHostAddressConstKHR data_adress = {
        _geoBuffer->get_device_adresse(_ctxDevice)};
    VkAccelerationStructureGeometryAabbsDataKHR aabb_data{
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,

        .pNext = nullptr,
        .data = data_adress,
        .stride = sizeof(VkAabbPositionsKHR),
    };

    VkAccelerationStructureGeometryKHR geometry{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext = nullptr,
        .geometryType = VK_GEOMETRY_TYPE_AABBS_KHR,
        .geometry =
            {
                .aabbs = aabb_data,
            },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    build_blas(ctx, aabbs, geometry);
  }

  Blas(VulkanContext &ctx, std::span<const TriangleIndices> indices,
       const Buffer<> &vbuff, const Buffer<> &ibuff, size_t instance_id,
       size_t index_offset, glm::mat4 transform)
      : _ctxDevice(ctx._device), _geoBuffer(std::nullopt), // no owned buffer
        _blasBuffer(std::nullopt), _instanceCustomIndex(instance_id),
        _transform(transform) {

    static_assert(sizeof(TriangleIndices) == 3 * sizeof(uint32_t),
                  "TriangleIndices must be tightly packed");

    VkAccelerationStructureGeometryTrianglesDataKHR trianle_data{
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat = Vertex::VK_FORMAT,
        .vertexData = {.deviceAddress = vbuff.get_device_adresse(_ctxDevice)},
        .vertexStride = sizeof(Vertex),
        .maxVertex = (uint32_t)(vbuff._count / sizeof(Vertex) - 1),
        .indexType = VK_INDEX_TYPE_UINT32,
        .indexData = {.deviceAddress = ibuff.get_device_adresse(_ctxDevice) +
                                       index_offset * sizeof(TriangleIndices)},
        .transformData = {},
    };

    VkAccelerationStructureGeometryKHR blas_geometry{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext = nullptr,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry = {.triangles = trianle_data},
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    build_blas(ctx, indices, blas_geometry);
  }

  Blas(Blas &&rval)
      : _ctxDevice(rval._ctxDevice), _geoBuffer(std::move(rval._geoBuffer)),
        _blasBuffer(std::move(rval._blasBuffer)), _blas((rval._blas)),
        _instanceCustomIndex(rval._instanceCustomIndex),
        _transform(rval._transform) {
    rval._ctxDevice = VK_NULL_HANDLE;
    rval._blas = VK_NULL_HANDLE;
  }

  Blas &operator=(Blas &&rval) {
    if (this != &rval) {
      this->_ctxDevice = rval._ctxDevice;
      this->_geoBuffer = std::move(rval._geoBuffer);
      this->_blasBuffer = std::move(rval._blasBuffer);
      this->_blas = rval._blas;
      this->_instanceCustomIndex = rval._instanceCustomIndex;
      this->_transform = rval._transform;

      rval._blas = VK_NULL_HANDLE;
      rval._ctxDevice = VK_NULL_HANDLE;
    }
    return *this;
  }

  ~Blas() {
    if (_ctxDevice != VK_NULL_HANDLE && _blas != VK_NULL_HANDLE)
      vkDestroyAccelerationStructureKHR(_ctxDevice, _blas, nullptr);
    _blas = VK_NULL_HANDLE;
  }

  // -- Methods
public:
  VkDeviceAddress get_device_addres() {
    VkAccelerationStructureDeviceAddressInfoKHR info{
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .pNext = nullptr,
        .accelerationStructure = _blas,
    };
    return vkGetAccelerationStructureDeviceAddressKHR(_ctxDevice, &info);
  }

private:
  template <std::copy_constructible T>
  void build_blas(VulkanContext &ctx, std::span<T> data,
                  VkAccelerationStructureGeometryKHR geometry) {

    VkAccelerationStructureBuildGeometryInfoKHR build_info{
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext = nullptr,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = {},
        .srcAccelerationStructure = {},
        .dstAccelerationStructure = {},
        .geometryCount = 1,
        .pGeometries = &geometry,
    };

    uint32_t primitive_count = static_cast<uint32_t>(data.size());

    VkAccelerationStructureBuildSizesInfoKHR size_info{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    vkGetAccelerationStructureBuildSizesKHR(
        _ctxDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info, &primitive_count, &size_info);

    // blass creation
    _blasBuffer =
        BlasBuffer<uint8_t>(ctx, size_info.accelerationStructureSize, {});

    VkAccelerationStructureCreateInfoKHR create_info{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .createFlags = {},
        .buffer = _blasBuffer.value()._buffer,
        .size = size_info.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .deviceAddress = {},
    };

    vkCreateAccelerationStructureKHR(_ctxDevice, &create_info, nullptr, &_blas);

    VkDeviceAddress scratch_alignment =
        ctx.get_physical_device_acc_struct_properties()
            .minAccelerationStructureScratchOffsetAlignment;
    constexpr VkBufferUsageFlags SCRATCH_BUFFER_USAGE =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    Buffer<uint8_t> scratch_buffer(
        ctx, size_info.buildScratchSize + scratch_alignment,
        SCRATCH_BUFFER_USAGE, VMA_MEMORY_USAGE_GPU_ONLY);

    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.dstAccelerationStructure = _blas;

    VkDeviceAddress scratch_addr =
        scratch_buffer.get_device_adresse(_ctxDevice);
    scratch_addr =
        (scratch_addr + scratch_alignment - 1) & ~(scratch_alignment - 1);
    build_info.scratchData = {.deviceAddress = scratch_addr};

    VkAccelerationStructureBuildRangeInfoKHR range_info{primitive_count, 0, 0,
                                                        0};
    const VkAccelerationStructureBuildRangeInfoKHR *ptr_range_info =
        &range_info;

    ctx.immediate_submit([&build_info, &ptr_range_info](auto cmd) {
      vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, &ptr_range_info);
    });
  }

  template <std::copy_constructible T>
  static Buffer<> upload_buffer(VulkanContext &ctx, std::span<T> data) {
    auto result = Buffer<>(ctx, data.size() * sizeof(T), BLAS_BUFFER_USAGE,
                           VMA_MEMORY_USAGE_CPU_TO_GPU);
    result.write(data.size() * sizeof(T),
                 reinterpret_cast<const uint8_t *>(data.data()));
    return result;
  }
};

class Tlas {
public:
  NO_COPY(Tlas);
  Tlas() = delete;

  Tlas(VulkanContext &ctx, std::vector<Blas> &&blas_vec)
      : _ctxDevice(ctx._device), _blasVec(std::move(blas_vec)),
        _instanceBuffer(
            ctx, _blasVec.size(),
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU),
        _tlasBuffer(std::nullopt) {

    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.reserve(_blasVec.size());
    for (size_t i = 0; i < _blasVec.size(); i++) {
      Blas &blas = _blasVec[i];
      instances.emplace_back(VkAccelerationStructureInstanceKHR{
          .transform = glm_to_vk_matrix(blas._transform),   // ! TEMPVAL
          .instanceCustomIndex = blas._instanceCustomIndex, // InstanceId() call
          .mask = 0xff,
          .instanceShaderBindingTableRecordOffset = 0, // hit group 0
          .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
          .accelerationStructureReference = blas.get_device_addres(),
      });
    }

    _instanceBuffer.write(instances);

    // Wating for blas to being construct
    ctx.immediate_submit([](auto cmd) {
      VkMemoryBarrier barrier{
          .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
          .pNext = nullptr,
          .srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
          .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
      };
      vkCmdPipelineBarrier(
          cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
          VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
          &barrier, 0, nullptr, 0, nullptr);
    });

    VkAccelerationStructureGeometryInstancesDataKHR instance_data{
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .pNext = nullptr,
        .arrayOfPointers = VK_FALSE,
        .data = {_instanceBuffer.get_device_adresse(_ctxDevice)},
    };

    VkAccelerationStructureGeometryKHR geometry{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext = nullptr,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = {.instances = instance_data},
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    uint32_t primitive_count = static_cast<uint32_t>(instances.size());

    VkAccelerationStructureBuildGeometryInfoKHR build_info{
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext = nullptr,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = 1,
        .pGeometries = &geometry,
    };

    VkAccelerationStructureBuildSizesInfoKHR size_info{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    vkGetAccelerationStructureBuildSizesKHR(
        _ctxDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info, &primitive_count, &size_info);

    _tlasBuffer =
        TlasBuffer<uint8_t>(ctx, size_info.accelerationStructureSize, {});

    VkAccelerationStructureCreateInfoKHR create_info{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .createFlags = {},
        .buffer = _tlasBuffer->_buffer,
        .size = size_info.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .deviceAddress = {}};
    vkCreateAccelerationStructureKHR(_ctxDevice, &create_info, nullptr, &_tlas);

    constexpr VkBufferUsageFlags SCRATCH_BUFFER_USAGE =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VkDeviceAddress scratch_alignment =
        ctx.get_physical_device_acc_struct_properties()
            .minAccelerationStructureScratchOffsetAlignment;
    Buffer<uint8_t> scratch_buffer(
        ctx, size_info.buildScratchSize + scratch_alignment,
        SCRATCH_BUFFER_USAGE, VMA_MEMORY_USAGE_GPU_ONLY);

    VkDeviceAddress scratch_addr =
        scratch_buffer.get_device_adresse(_ctxDevice);
    scratch_addr =
        (scratch_addr + scratch_alignment - 1) & ~(scratch_alignment - 1);

    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.dstAccelerationStructure = _tlas;
    build_info.scratchData = {scratch_addr};

    VkAccelerationStructureBuildRangeInfoKHR range_info{primitive_count, 0, 0,
                                                        0};
    const VkAccelerationStructureBuildRangeInfoKHR *ptr_range = &range_info;

    ctx.immediate_submit([&build_info, &ptr_range](auto cmd) {
      vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, &ptr_range);
    });
  }

  Tlas(Tlas &&rval)
      : _ctxDevice(rval._ctxDevice), _blasVec(std::move(rval._blasVec)),
        _instanceBuffer(std::move(rval._instanceBuffer)),
        _tlasBuffer(std::move(rval._tlasBuffer)), _tlas(rval._tlas) {
    rval._tlas = VK_NULL_HANDLE;
    rval._ctxDevice = VK_NULL_HANDLE;
  }

  Tlas &operator=(Tlas &&rval) {
    if (this != &rval) {
      this->_ctxDevice = rval._ctxDevice;
      this->_blasVec = std::move(rval._blasVec);
      this->_instanceBuffer = std::move(rval._instanceBuffer);
      this->_tlasBuffer = std::move(rval._tlasBuffer);
      this->_tlas = rval._tlas;

      rval._tlas = VK_NULL_HANDLE;
      rval._ctxDevice = VK_NULL_HANDLE;
    }
    return *this;
  }

  ~Tlas() {
    if (_ctxDevice == VK_NULL_HANDLE || _tlas == VK_NULL_HANDLE)
      return;
    vkDestroyAccelerationStructureKHR(_ctxDevice, _tlas, nullptr);
    _ctxDevice = VK_NULL_HANDLE;
    _tlas = VK_NULL_HANDLE;
  }

  // -- Getters
  VkAccelerationStructureKHR get_tlas() const { return _tlas; }
  const TlasBuffer<uint8_t> &get_buffer() const { return _tlasBuffer.value(); }

  VkWriteDescriptorSetAccelerationStructureKHR get_write_descr_alloc() const {
    return VkWriteDescriptorSetAccelerationStructureKHR{
        .sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .pNext = nullptr,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &_tlas,
    };
  }

private:
  VkTransformMatrixKHR glm_to_vk_matrix(glm::mat4 m) {
    return VkTransformMatrixKHR{.matrix = {
                                    {m[0][0], m[1][0], m[2][0], m[3][0]},
                                    {m[0][1], m[1][1], m[2][1], m[3][1]},
                                    {m[0][2], m[1][2], m[2][2], m[3][2]},
                                }};
  }

  static constexpr VkTransformMatrixKHR IDENTITY_TRANSFORM =
      VkTransformMatrixKHR{.matrix = {
                               {1, 0, 0, 0},
                               {0, 1, 0, 0},
                               {0, 0, 1, 0},
                           }};

  // -- Attributs
private:
  VkDevice _ctxDevice;
  std::vector<Blas> _blasVec;
  Buffer<VkAccelerationStructureInstanceKHR> _instanceBuffer;
  std::optional<TlasBuffer<uint8_t>> _tlasBuffer;
  VkAccelerationStructureKHR _tlas = VK_NULL_HANDLE;
};