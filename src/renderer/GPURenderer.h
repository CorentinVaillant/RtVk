#pragma once

#include "Camera.h"
#include "graphics/pipelines.h"
#include "graphics/utils.h"
#include "graphics/vulkan_context.h"
#include "renderer/Renderer.h"
#include <vulkan/vulkan_core.h>

struct RendererPushCst {
  Camera _cam;
  glm::vec4 _lightDir;
  float _renderWidth, _renderHeight;
};

struct RendererUniforms {
  Camera::CameraRenderInfo _camRenderInfo;
};

class GPURenderer : public Renderer {

public:
  GPURenderer(VulkanContext &ctx, ImageBuffer &&img_buffer);
  GPURenderer(VulkanContext &ctx, size_t img_width, size_t img_heigth,
              ImgFormat format)
      : GPURenderer(ctx, ImageBuffer(img_width, img_heigth, format)) {}

  // -- Methods --
private:
  RendererPushCst get_push_cst(const Scene &scene)const;

  RendererUniforms get_uniform(const Scene &scene)const;

  static RtPipeline create_pipeline(VulkanContext &ctx);
  static DescriptorAllocator create_descr_aloc(VulkanContext &ctx);

  // -- Renderer impl
public:
  void render(const Scene &scene) override;

  // -- Attributs --
private:
  VulkanContext &_ctx;
  RtPipeline _pipeline;

  DescriptorAllocator _descr_aloc;
};