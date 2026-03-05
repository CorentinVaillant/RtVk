#pragma once

#include "graphics/vulkan_context.h"
#include "types.h"

#include <concepts>
#include <cstddef>
#include <cstring>
#include <volk.h>
// ~ : There is a lot of parameters that are defaulted, maybe make
// an
// aditional struct builder, that register all of this
// informations before building and allocating the buffer

template <std::copy_constructible T> class Buffer {
public:
  // -- Constructors
  Buffer(VulkanContext &ctx, size_t alloc_count, VkBufferUsageFlags usage,
         VmaMemoryUsage mem_usage)
      : _ctx_allocator(ctx._memAllocator), _count(alloc_count) {
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0, // ~
        .size = alloc_count * sizeof(T),
        .usage = usage,
        .sharingMode = {},              //~
        .queueFamilyIndexCount{},       //~
        .pQueueFamilyIndices = nullptr, //~
    };

    VmaAllocationCreateInfo vma_alloc_create_info{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = mem_usage,
        // ~
    };

    VK_CHECK(vmaCreateBuffer(ctx._memAllocator, &buffer_create_info,
                             &vma_alloc_create_info, &_buffer, &_alloc,
                             &_allocInfo));
  }

  // move constructor
  Buffer(Buffer &&rval)
      : _ctx_allocator(rval._ctx_allocator), _buffer(rval._buffer),
        _alloc(rval._alloc), _allocInfo(rval._allocInfo), _count(rval._count) {

    rval._ctx_allocator = nullptr;
    rval._buffer = VK_NULL_HANDLE;
    rval._alloc = nullptr;
    rval._allocInfo = {};
    rval._count = 0;
  }

  // move asigment
  Buffer &operator=(Buffer &&rval) {
    if (this != &rval) {
      _ctx_allocator = rval._ctx_allocator;
      _buffer = rval._buffer;

      _alloc = rval._alloc;
      _allocInfo = rval._allocInfo;
      _count = rval._count;
    }
    return *this;
  }

  ~Buffer() { vmaDestroyBuffer(_ctx_allocator, _buffer, _alloc); }

  NO_COPY(Buffer);

  // -- Methods
  void write(size_t count, const T *data) {
    memcpy(_allocInfo.pMappedData, reinterpret_cast<const uint8_t *>(data),
           count * sizeof(T));
  }

  void write(std::span<T> data) { write(data.size(), data.data()); }

  void read(size_t count, T *dst) {
    memcpy(dst, _allocInfo.pMappedData, count * sizeof(T));
  }

  VkDeviceAddress get_device_adresse(VkDevice device) const {
    VkBufferDeviceAddressInfo info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = _buffer,
    };

    return vkGetBufferDeviceAddress(device, &info);
  }

  // -- Attributs
protected:
  VmaAllocator _ctx_allocator;

public:
  VkBuffer _buffer;
  VmaAllocation _alloc;
  VmaAllocationInfo _allocInfo;
  size_t _count;
};

template <std::copy_constructible T> class BlasBuffer : public Buffer<T> {
  static constexpr VkBufferUsageFlags USAGE =
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

public:
  BlasBuffer(VulkanContext &ctx, size_t alloc_count, VmaMemoryUsage mem_usage)
      : Buffer<T>(ctx, alloc_count, USAGE, mem_usage) {}
};

template <std::copy_constructible T> class TlasBuffer : public Buffer<T> {
  static constexpr VkBufferUsageFlags USAGE =
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

public:
  TlasBuffer(VulkanContext &ctx, size_t alloc_count, VmaMemoryUsage mem_usage)
      : Buffer<T>(ctx, alloc_count, USAGE, mem_usage) {}
};
