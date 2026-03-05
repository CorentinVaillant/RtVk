#pragma once

#include "Buffer.h"
#include "graphics/Buffer.h"
#include "graphics/vma_usage.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <cstdint>
#include <optional>

#include <volk.h>
#include <vulkan/vulkan_core.h>

class Blas {
public:
  NO_COPY(Blas);

  Blas() = delete;

  Blas(VulkanContext &ctx, std::span<VkAabbPositionsKHR> aabbs)
      : _ctxDevice(ctx._device), _aabbsBuffer(upload_buffer(ctx, aabbs)),
        _blasBuffer(std::nullopt) {

    VkDeviceOrHostAddressConstKHR data_adress = {
        _aabbsBuffer.get_device_adresse(_ctxDevice)};
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

    uint32_t primitive_count = static_cast<uint32_t>(aabbs.size());

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

    constexpr VkBufferUsageFlags SCRATCH_BUFFER_USAGE =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    Buffer<uint8_t> scratch_buffer(ctx, size_info.buildScratchSize,
                                   SCRATCH_BUFFER_USAGE,
                                   VMA_MEMORY_USAGE_GPU_ONLY);

    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.dstAccelerationStructure = _blas;
    build_info.scratchData = {
        .deviceAddress = scratch_buffer.get_device_adresse(_ctxDevice)};

    VkAccelerationStructureBuildRangeInfoKHR range_info{primitive_count, 0, 0,
                                                        0};
    const VkAccelerationStructureBuildRangeInfoKHR *ptr_range_info =
        &range_info;

    ctx.immediate_submit([&build_info, &ptr_range_info](auto cmd) {
      vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, &ptr_range_info);
    });
  }

  Blas(Blas &&rval)
      : _ctxDevice(rval._ctxDevice), _aabbsBuffer(std::move(rval._aabbsBuffer)),
        _blasBuffer(std::move(rval._blasBuffer)), _blas((rval._blas)) {
    rval._ctxDevice = VK_NULL_HANDLE;
    rval._blas = VK_NULL_HANDLE;
  }

  ~Blas() {
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
  static Buffer<VkAabbPositionsKHR>
  upload_buffer(VulkanContext &ctx, std::span<VkAabbPositionsKHR> aabbs) {
    auto result = Buffer<VkAabbPositionsKHR>(
        ctx, aabbs.size(), AABB_BUFFER_USAGE, VMA_MEMORY_USAGE_CPU_TO_GPU);
    result.write(aabbs.size(), aabbs.data());
    return result;
  }

private:
  static constexpr VkBufferUsageFlags AABB_BUFFER_USAGE =
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  VkDevice _ctxDevice;

public:
  Buffer<VkAabbPositionsKHR> _aabbsBuffer;
  std::optional<BlasBuffer<uint8_t>> _blasBuffer;
  VkAccelerationStructureKHR _blas = VK_NULL_HANDLE;
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
          .transform = IDENTITY_TRANSFORM,
          .instanceCustomIndex = static_cast<uint32_t>(i), // InstanceId() call
          .mask = 0xff,
          .instanceShaderBindingTableRecordOffset = 0, // hit group 0
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

    Buffer<uint8_t> scratch(ctx, size_info.buildScratchSize,
                            SCRATCH_BUFFER_USAGE, VMA_MEMORY_USAGE_GPU_ONLY);

    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.dstAccelerationStructure = _tlas;
    build_info.scratchData = {scratch.get_device_adresse(_ctxDevice)};

    VkAccelerationStructureBuildRangeInfoKHR range_info{primitive_count, 0, 0,
                                                        0};
    const VkAccelerationStructureBuildRangeInfoKHR *ptr_range = &range_info;

    ctx.immediate_submit([&build_info, &ptr_range](auto cmd) {
      vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, &ptr_range);
    });
  }

  ~Tlas() {
    vkDestroyAccelerationStructureKHR(_ctxDevice, _tlas, nullptr);
    _tlas = VK_NULL_HANDLE;
  }

  // -- Getters
  VkAccelerationStructureKHR get_tlas() const { return _tlas; }

private:
  static constexpr VkTransformMatrixKHR IDENTITY_TRANSFORM = {
      1, 0, 0, 0, //
      0, 1, 0, 0, //
      0, 0, 1, 0, //
  };

  // -- Attributs
private:
  VkDevice _ctxDevice;
  std::vector<Blas> _blasVec;
  Buffer<VkAccelerationStructureInstanceKHR> _instanceBuffer;
  std::optional<TlasBuffer<uint8_t>> _tlasBuffer;
  VkAccelerationStructureKHR _tlas = VK_NULL_HANDLE;
};