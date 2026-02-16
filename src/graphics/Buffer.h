#pragma once

#include "graphics/vulkan_context.h"
#include "types.h"

#include <concepts>
#include <cstddef>
#include <cstring>
#include <vulkan/vulkan_core.h>

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

  ~Buffer() { vmaDestroyBuffer(_ctx_allocator, _buffer, _alloc); }

  NO_COPY(Buffer);

  // -- Methods
  void write_from_cpu(size_t count,const T *data) {
    memcpy(_allocInfo.pMappedData, reinterpret_cast<const uint8_t *>(data),
           count * sizeof(T));
  }

  // -- Attributs
private:
  VmaAllocator _ctx_allocator;

public:
  VkBuffer _buffer;
  VmaAllocation _alloc;
  VmaAllocationInfo _allocInfo;
  size_t _count;
};
