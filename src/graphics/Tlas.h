#pragma once

#include "graphics/Blas.h"
#include "graphics/vulkan_context.h"
#include "types.h"
class Tlas {
public:
  NO_COPY(Tlas);
  Tlas() = delete;

  // -- Attributs
private:
  VkDevice _ctxDevice;
  Buffer<VkAccelerationStructureInstanceKHR> _instanceBuffer;
  std::optional<TlasBuffer<uint8_t>> _tlasBuffer;
  VkAccelerationStructureKHR _tlas = VK_NULL_HANDLE;

  
  public:
  using BlasSpanTab = std::span<std::span<Blas>>;
  Tlas(VulkanContext &ctx, BlasSpanTab blas_span_tab)
      : _ctxDevice(ctx._device),
        _instanceBuffer(make_instance_buffer(ctx, blas_span_tab)),
        _tlasBuffer(std::nullopt) {

    std::vector<VkAccelerationStructureInstanceKHR> instances;
    for (size_t i = 0; i < blas_span_tab.size(); i++)
      for (size_t j = 0; j < blas_span_tab[i].size(); j++) {
        const Blas &blas = blas_span_tab[i][j];
        instances.emplace_back(VkAccelerationStructureInstanceKHR{
            .transform = glm_to_vk_matrix(blas._transform),
            .instanceCustomIndex =
                blas._instanceCustomIndex, // InstanceId() call
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
      : _ctxDevice(rval._ctxDevice),
        _instanceBuffer(std::move(rval._instanceBuffer)),
        _tlasBuffer(std::move(rval._tlasBuffer)), _tlas(rval._tlas) {
    rval._tlas = VK_NULL_HANDLE;
    rval._ctxDevice = VK_NULL_HANDLE;
  }

  Tlas &operator=(Tlas &&rval) {
    if (this != &rval) {
      this->_ctxDevice = rval._ctxDevice;
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
  static Buffer<VkAccelerationStructureInstanceKHR>
  make_instance_buffer(VulkanContext &ctx, BlasSpanTab blas_span_tab) {
    size_t total_instance_count = 0;
    for (const auto &span : blas_span_tab)
      total_instance_count += span.size();

    return Buffer<VkAccelerationStructureInstanceKHR>(
        ctx, total_instance_count,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
  }

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
};