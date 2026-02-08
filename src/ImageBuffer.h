#pragma once

#include "types.h"

#include <cstddef>
#include <vulkan/vulkan_core.h>

enum ColorFormat {
  RGBA = VK_FORMAT_R8G8B8A8_UNORM,
  RGB = VK_FORMAT_R8G8B8_UNORM,
  R = VK_FORMAT_R8_UNORM,
};

enum ImageFormat {
  PNG,
  BMP,
  TGA,
  // HDR, TODO
  JPG,
};

class ImageBuffer {
public:
  ImageBuffer(size_t width, size_t height,
              ColorFormat format = ColorFormat::RGBA);

  // move constructors
  ImageBuffer(ImageBuffer &&other)
      : _width(other._width), _heigth(other._heigth), _format(other._format),
        _imgData(std::move(other._imgData)) {}

  ImageBuffer &operator=(ImageBuffer &&other) {
    if (this != &other) {
      _width = other._width;
      _heigth = other._heigth;
      _format = other._format;
      _imgData = std::move(other._imgData);
    }

    return *this;
  }

  NO_COPY(ImageBuffer);

  void write_pixel(size_t px, size_t py, Color color);

  int write_on_disk(const char *filename, ImageFormat format,
                    uint8_t jpg_quality = 8) const;

  // -- Getters
  size_t get_width() const { return _width; }
  size_t get_height() const { return _heigth; }
  ColorFormat get_format() const {return _format;}

private:
  size_t _width, _heigth;
  ColorFormat _format;
  std::vector<uint8_t> _imgData;
};