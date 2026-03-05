#include "GPURenderer.h"
#include "graphics/utils.h"

GPURenderer::GPURenderer(VulkanContext &ctx, ImageBuffer &&img_buffer)
    : Renderer(std::move(img_buffer)),
      _descrSetLayout(init_descr_set_layout(ctx)) {

  // todo ...
}

// -- Methods --

// -- Renderer impl
void GPURenderer::render(const Scene &scene) {
    
}


DescriptorSetLayout GPURenderer::init_descr_set_layout(VulkanContext &ctx) {
  return DescriptorLayoutBuilder()
      .add_binding(0, StorageImage, Raygen)       // result
      .add_binding(1, AccelerationStruct, Raygen) // acceleration struct
      .add_binding(2, UniformBuffer, Raygen)      // Uniforms
      .add_binding(3, StorageBuffer,
                   {Intersection, ClosestHit}) // primitive buffer
      .build(ctx._device, {});
}
