#pragma once

#include "graphics/utils.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <cstddef>
#include <volk.h>

class VulkanContext;

enum ImgFormat {
  // Color format
  RGBA = VK_FORMAT_R8G8B8A8_UNORM,
  RGB = VK_FORMAT_R8G8B8_UNORM,
  R = VK_FORMAT_R8_UNORM,

  // Depth format
  DEPTH = VK_FORMAT_D32_SFLOAT,
};

enum ImgLayout {
  Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
  General = VK_IMAGE_LAYOUT_GENERAL,
  ColorAttachmentOpt = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  TransferSrcOpt = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
  TransferDstOpt = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
  // ...
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
        VkImageUsageFlags mem_usage, ImgLayout layout, bool mipmapped = false);

  Image(VulkanContext &ctx, const unsigned char *data, VkExtent3D size,
        ImgFormat format, VkImageUsageFlags mem_usage, ImgLayout layout,
        bool mipmapped = false);

  NO_COPY(Image);

  // TODO move constr

  ~Image() {
    vkDestroyImageView(_device, _view, nullptr);
    vkDestroyImage(_device, _vkImage, nullptr);
  }

  // -- Getters --
  VkExtent3D get_size() const { return _extent; }
  ImgFormat get_format() const { return _format; }
  ImgLayout get_layout() const { return _layout; }

  // -- Methods --
  void transition(VkCommandBuffer cmd, ImgLayout next_layout);

  void write(DescriptorWriter &writter, uint32_t binding, VkSampler sampler,
             DescriptorType descr_type) {
    writter.write_image(binding, _view, sampler,
                        static_cast<VkImageLayout>(_layout),
                        static_cast<VkDescriptorType>(descr_type));
  }

private:
  // -- Methods --
  VkImageCreateInfo create_image_create_info(VkImageUsageFlags usage,
                                             bool mipmapped);
  VkImageViewCreateInfo create_image_view_create_info(uint32_t mip_level_count);

  // -- Atributs --
public:
  VkImage _vkImage;
  VkImageView _view;
  VmaAllocation _allocation;
  VkExtent3D _extent;
  ImgFormat _format;
  ImgLayout _layout;

private:
  VkDevice _device;
};
