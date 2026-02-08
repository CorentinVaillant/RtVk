#include "ImageBuffer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

constexpr size_t MAX_COLOR_SIZE = 4;

size_t format_size(ColorFormat format) {
  switch (format) {
  case RGBA: /*  = VK_FORMAT_R8G8B8A8_UNORM */
    return 4;
  case RGB: /*  = VK_FORMAT_R8G8B8_UNORM */
    return 3;
  case R: /*  = VK_FORMAT_R8_UNORM */
  default:
    return 1;
  }
}

uint8_t floatnorm_to_unorm(float t) {
  if (std::isnan(t) || t < 0.)
    t = 0;
  return static_cast<uint8_t>(std::min(t, 1.f) * 255);
}

size_t color_to_format(Color color, ColorFormat format,
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
  case R: /*  = VK_FORMAT_R8_UNORM */
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
                         ColorFormat format /*= ColorFormat::RGBA */)
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
