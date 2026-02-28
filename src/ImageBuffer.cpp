#include "ImageBuffer.h"
#include "graphics/Buffer.h"
#include "graphics/Image.h"
#include "graphics/vulkan_context.h"
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#ifdef NDEBUG
#define STBIW_ASSERT(x)                                                        \
  do {                                                                         \
    bool stbiw_assert_eval = static_cast<bool>(x);                             \
    if (!stbiw_assert_eval)                                                    \
      throw std::runtime_error(#x);                                            \
  } while (0)
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

constexpr size_t MAX_COLOR_SIZE = 4;

uint8_t floatnorm_to_unorm(float t) {
  if (std::isnan(t) || t < 0.)
    t = 0;
  return static_cast<uint8_t>(std::min(t, 1.f) * 255);
}

size_t color_to_format(Color color, ImgFormat format,
                       std::array<uint8_t, MAX_COLOR_SIZE> *written_color) {
  switch (format) {
  case RGBA: /*  = VK_FORMAT_R8G8B8A8_UNORM */
    written_color->at(0) = floatnorm_to_unorm(color[0]);
    written_color->at(1) = floatnorm_to_unorm(color[1]);
    written_color->at(2) = floatnorm_to_unorm(color[2]);
    written_color->at(3) = floatnorm_to_unorm(color[3]);
    break;
  case RGB: /*  = VK_FORMAT_R8G8B8_UNORM */
    written_color->at(0) = floatnorm_to_unorm(color[0] * color[3]);
    written_color->at(1) = floatnorm_to_unorm(color[1] * color[3]);
    written_color->at(2) = floatnorm_to_unorm(color[2] * color[3]);
    break;

  case R:     /*  = VK_FORMAT_R8_UNORM */
  case DEPTH: /* = VK_FORMAT_D32_SFLOAT */
  default:
    written_color->at(0) =
        floatnorm_to_unorm(((color[0] + color[1] + color[2]) / 3.f) * color[3]);
    break;
  }
  return format_size(format);
}

// -- Image buffer impl --

// -- Constructors

ImageBuffer::ImageBuffer(size_t width, size_t height,
                         ImgFormat format /*= ColorFormat::RGBA */)
    : _width(width), _heigth(height), _format(format) {
  size_t pixel_size = format_size(format);
  size_t buffer_size = pixel_size * width * height;

  _imgData.resize(buffer_size);
}

// -- Methods

void ImageBuffer::write_pixel(size_t px, size_t py, Color color) {
  size_t buffer_pos = (px + _width * py) * format_size(_format);
  std::array<uint8_t, MAX_COLOR_SIZE> formated_color;
  size_t format_size = color_to_format(color, _format, &formated_color);

  for (size_t i = 0; i < format_size; i++)
    _imgData[buffer_pos + i] = formated_color[i];
}

int ImageBuffer::write_on_disk(const char *filename, ImageFormat img_format,
                               uint8_t jpg_quality /* = 8 */) const {
  int iwidth = static_cast<int>(_width);
  int iheigth = static_cast<int>(_heigth);
  int iformat_size = static_cast<int>(format_size(_format));

  switch (img_format) {
  case PNG:
    return stbi_write_png(filename, iwidth, iheigth, iformat_size,
                          _imgData.data(), 0);
  case BMP:
    return stbi_write_bmp(filename, iwidth, iheigth, iformat_size,
                          _imgData.data());
  case TGA:
    return stbi_write_tga(filename, iwidth, iheigth, iformat_size,
                          _imgData.data());
  // case HDR:
  // todo
  case JPG:
  default:
    return stbi_write_jpg(filename, iwidth, iheigth, iformat_size,
                          _imgData.data(), jpg_quality);
  }
}

Image ImageBuffer::write_to_gpu(VulkanContext &ctx) const {
  VkExtent3D extent = {.width = static_cast<uint32_t>(_width),
                       .height = static_cast<uint32_t>(_heigth),
                       .depth = 1};
  return Image(ctx, _imgData.data(), extent, _format,
               VK_IMAGE_USAGE_SAMPLED_BIT, ImgLayout::General);
}

void ImageBuffer::read_from_gpu(VulkanContext &ctx, Image &src_image) {

  auto img_size = src_image.get_size();
  auto buffer_size = img_size.width * img_size.height * img_size.depth *
                     format_size(src_image._format);

  Buffer<uint8_t> dst_buffer(ctx, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VMA_MEMORY_USAGE_GPU_TO_CPU);

  VkBufferImageCopy2 buff_img_copy = VkBufferImageCopy2{
      .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
      .pNext = nullptr,
      .bufferOffset = 0,
      .bufferRowLength = 0,   //| If either of these values is zero, that aspect
                              //| of the buffer memory is considered to be
                              //|  tightlypacked according to the imageExtent
      .bufferImageHeight = 0, //|
      .imageSubresource =
          VkImageSubresourceLayers{
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // ! Harcoded to change
              .mipLevel = 0,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
      .imageOffset = {0, 0, 0},
      .imageExtent = src_image._extent,

  };

  VkCopyImageToBufferInfo2 copy_info = VkCopyImageToBufferInfo2{
      .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2,
      .pNext = nullptr,
      .srcImage = src_image._vkImage,
      .srcImageLayout = static_cast<VkImageLayout>(src_image._layout),
      .dstBuffer = dst_buffer._buffer,
      .regionCount = 1,
      .pRegions = &buff_img_copy,
  };

  ctx.immediate_submit([&copy_info](VkCommandBuffer cmd) {
    vkCmdCopyImageToBuffer2(cmd, &copy_info);
  });

  dst_buffer.read(buffer_size, _imgData.data());
}
