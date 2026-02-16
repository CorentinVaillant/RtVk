#include "Image.h"
#include "graphics/Buffer.h"
#include "graphics/utils.h"
#include "types.h"
#include <vulkan/vulkan_core.h>

// -- Contructors --

Image::Image(VulkanContext &ctx, VkExtent3D size, ImgFormat format,
             VkImageUsageFlags usage, bool mipmapped /* = false */)
    : _extent(size), _format(format), _device(ctx._device) {

  VkImageCreateInfo img_create_info =
      create_image_create_info(usage, mipmapped);

  VmaAllocationCreateInfo alloc_create_info = {};
  alloc_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY; // Only use one the GPU
  alloc_create_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VK_CHECK(vmaCreateImage(ctx._memAllocator, &img_create_info,
                          &alloc_create_info, &_vkImage, &_allocation,
                          nullptr));

  // Building the image view
  VkImageViewCreateInfo imgview_create_info =
      create_image_view_create_info(img_create_info.mipLevels);
  VK_CHECK(
      vkCreateImageView(ctx._device, &imgview_create_info, nullptr, &_view));
}

Image::Image(VulkanContext &ctx, const unsigned char *data, VkExtent3D size,
             ImgFormat format, VkImageUsageFlags usage,
             bool mipmapped /* = false */)
    : Image(ctx, size, format,
            usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            mipmapped) {

  size_t data_size =
      size.depth * size.width * size.height * format_size(format);
  Buffer<uint8_t> upload_buffer(ctx, data_size,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VMA_MEMORY_USAGE_CPU_TO_GPU);
  upload_buffer.write_from_cpu(data_size, data);

  ctx.immediate_submit([&upload_buffer, this](VkCommandBuffer cmd) {
    transition_image(cmd, _vkImage, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copy_region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .imageOffset = {},
        .imageExtent = _extent,
    };

    // copy the buffer into the image
    vkCmdCopyBufferToImage(cmd, upload_buffer._buffer, _vkImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &copy_region);

    transition_image(cmd, _vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  });
}
// -- Methods --

// -- public

// -- private
VkImageCreateInfo Image::create_image_create_info(VkImageUsageFlags usage,
                                                  bool mipmapped) {

  uint32_t mip_levels =
      mipmapped ? static_cast<uint32_t>(std::floor(
                      std::log2(std::max(_extent.width, _extent.height)))) +
                      1
                : 1;

  // ~ : There is a lot of parameters that are defaulted, maybe make
  // an
  // aditional struct builder, that register all of this
  // informations before building and allocating the image
  return {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,                               // ~
      .imageType = VK_IMAGE_TYPE_2D,            // ~
      .format = static_cast<VkFormat>(_format), // ImgFormat values correspond
                                                // to the vulkan ones
      .extent = _extent,
      .mipLevels = mip_levels,
      .arrayLayers = 1,                  // ~
      .samples = VK_SAMPLE_COUNT_1_BIT,  // ~
      .tiling = VK_IMAGE_TILING_OPTIMAL, // ~
      .usage = usage,
  };
}

VkImageViewCreateInfo
Image::create_image_view_create_info(uint32_t mip_level_count) {
  VkImageAspectFlags aspect_flags = _format == ImgFormat::DEPTH
                                        ? VK_IMAGE_ASPECT_DEPTH_BIT
                                        : VK_IMAGE_ASPECT_COLOR_BIT;
  return {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0, // ~
      .image = _vkImage,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,        // ~
      .format = static_cast<VkFormat>(_format), // ImgFormat values correspond
      .components = {},                         // ~
      .subresourceRange =
          {
              .aspectMask = aspect_flags,
              .baseMipLevel = 0,             //~
              .levelCount = mip_level_count, //~
              .baseArrayLayer = 0,           //~
              .layerCount = 1,               //~
          },
  };
}
