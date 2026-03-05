#pragma once

#include "graphics/utils.h"
#include "graphics/vulkan_context.h"
#include "renderer/Renderer.h"

struct Uniforms {
  float _screenWidth, _screenHeight;
  float _focalLength, _frameHeight;
  glm::vec4 _cameraDir;
  glm::vec4 _cameraUp;
  glm::vec4 _cameraRight;
  glm::vec4 _cameraPosition;
  glm::vec4 _lightDir;
};

class GPURenderer : public Renderer {

public:
  GPURenderer(VulkanContext &ctx, ImageBuffer &&img_buffer);
  GPURenderer(VulkanContext &ctx, size_t img_width, size_t img_heigth,
              ImgFormat format)
      : GPURenderer(ctx, ImageBuffer(img_width, img_heigth, format)) {

    // TODO : init the rt pipeline


    // ...
  }

  // -- Methods --

  // -- Renderer impl
  void render(const Scene &scene) override;

private:
  static DescriptorSetLayout init_descr_set_layout(VulkanContext &ctx);

  // -- Attributs --
private:
  DescriptorSetLayout _descrSetLayout;
};