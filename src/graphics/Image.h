#pragma once

#include "graphics/vulkan_context.h"
#include "types.h"
#include <cstddef>
#include <vulkan/vulkan_core.h>

enum ImgFormat {
  // Color format
  RGBA = VK_FORMAT_R8G8B8A8_UNORM,
  RGB = VK_FORMAT_R8G8B8_UNORM,
  R = VK_FORMAT_R8_UNORM,

  // Depth format
  DEPTH = VK_FORMAT_D32_SFLOAT,
};

inline size_t format_size(ImgFormat format) {
  switch (format) {
  case RGBA:  /*  = VK_FORMAT_R8G8B8A8_UNORM */
  case DEPTH: /* VK_FORMAT_D32_SFLOAT */
    return 4;
  case RGB: /*  = VK_FORMAT_R8G8B8_UNORM */
    return 3;
  case R: /*  = VK_FORMAT_R8_UNORM */
  default:
    return 1;
  }
}

class Image {
public:
  Image() = delete;
  Image(VulkanContext &ctx, VkExtent3D size, ImgFormat format,
        VkImageUsageFlags mem_usage, bool mipmapped = false);

  Image(VulkanContext &ctx, unsigned char *data, VkExtent3D size, ImgFormat format,
        VkImageUsageFlags mem_usage, bool mipmapped = false);

  NO_COPY(Image);

  // TODO move constr

  ~Image() { 
    vkDestroyImageView(_device, _view, nullptr);
    vkDestroyImage(_device, _vkImage, nullptr); }

private:
  // -- Methods --
  VkImageCreateInfo create_image_create_info(VkImageUsageFlags usage,bool mipmapped);
  VkImageViewCreateInfo create_image_view_create_info(uint32_t mip_level_count);

  // -- Atributs --
  VkImage _vkImage;
  VkImageView _view;
  VmaAllocation _allocation;
  VkExtent3D _extent;
  ImgFormat _format;

  VkDevice _device;
};
