#include "ImageBuffer.h"
#include "Scene.h"
class Renderer {

public:
  Renderer(ImageBuffer &&img_buffer) : _imgBuffer(std::move(img_buffer)) {}
  Renderer(size_t img_width, size_t img_heigth,
           ImgFormat format = ImgFormat::RGBA)
      : Renderer(ImageBuffer(img_width, img_heigth, format)) {}

  virtual ~Renderer() = default;

  virtual void render(const Scene &scene) = 0;

  const ImageBuffer &get_img_buff() const { return _imgBuffer; }

protected:
  ImageBuffer _imgBuffer;
};