#pragma once

#include "Camera.h"
#include "graphics/pipelines.h"
#include "graphics/utils.h"
#include "graphics/vulkan_context.h"
#include "hittables/TriangleRef.h"
#include "renderer/Renderer.h"
#include "renderer/renderer_utils.h"
#include "types.h"

struct RendererPushCst {
  Camera _cam;
  float _renderWidth, _renderHeight, _time;
  uint32_t max_depth = 5;
  uint32_t spp = 100;
};

struct alignas(STD140_ALIGNEMENT) RendererUniforms {
  Camera::CameraRenderInfo _camRenderInfo;
};

class GPURenderer : public Renderer {

  struct RenderBuffers {
    Buffer<RendererUniforms> uniforms;
    Buffer<Material> materials;
    size_t material_size;
  };

public:
  GPURenderer(VulkanContext &ctx, ImageBuffer &&img_buffer);
  GPURenderer(VulkanContext &ctx, size_t img_width, size_t img_heigth,
              ImgFormat format)
      : GPURenderer(ctx, ImageBuffer(img_width, img_heigth, format)) {}

  // -- Methods --
private:
  RendererPushCst get_push_cst(const Scene &scene) const;

  RendererUniforms get_uniform(const Scene &scene) const;

  RenderBuffers make_render_buffer(const Scene &scene) const;

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

  static constexpr uint32_t RESULT_IMAGE_BINDING = 0;
  static constexpr uint32_t SCENE_TLAS_BINDING = 1;
  static constexpr uint32_t UNIFORMS_BINDING = 2;
  static constexpr uint32_t MATERIAL_BINDING = 3;
  static constexpr uint32_t INSTANCES_BINDING = Instance::BINDING;
  static constexpr uint32_t VERTEX_BINDING = Vertex::BINDING;
  static constexpr uint32_t INDEX_BINDING = TriangleIndices::BINDING;
};