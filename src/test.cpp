#include "test.h"
#include "graphics/PipelineDescriptor.h"
#include "graphics/Shaders.h"
#include "graphics/pipelines.h"
#include "graphics/raii_graphic.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#ifdef NTEST
#include "graphics/utils.h"
#include <vulkan/vulkan_core.h>

struct PushCstTest {
  glm::vec2 resolution;
  float time;
};

void test_descriptor_allocator(VulkanContext &ctx) {
  DescriptorAllocator::PoolSizeRatio sizes_ratio[] = {
      {.type = StorageImage, .ratio = 3},
      {.type = UniformBuffer, .ratio = 3},
  };

  DescriptorAllocator descr_alloc(ctx, 10, sizes_ratio);

  DescriptorLayoutBuilder builder;
  builder.add_binding(0, StorageImage);
  builder.add_binding(1, UniformBuffer);
  DescriptorSetLayout descr_set_layout =
      builder.build(ctx._device, ComputeShader);

  [[maybe_unused]] auto descriptor =
      descr_alloc.allocate(descr_set_layout);

  LOGOK("descriptor_allocator");
}

void test_pipeline_build(VulkanContext &ctx) {
  test_compute_pipeline_build(ctx);
}

void test_shader_loading(VulkanContext &ctx) {
  Shader loaded_shader = Shader(ctx, "./shaders/mandelbrot.slang.spv");
  LOGOK("shader_loading");
}

void test_compute_pipeline_build(VulkanContext &ctx) {

  Shader loaded_shader = Shader(ctx, "./shaders/mandelbrot.slang.spv");

  PipelineDescriptor descr;
  descr.add_shader_stage(ComputeShader, loaded_shader)
      .add_binding(0, StorageImage, ComputeShader)
      .set_push_cst(ComputeShader, 0, sizeof(PushCstTest));

  ComputePipeline compute_pipeline(ctx, descr);

  DescriptorAllocator::PoolSizeRatio sizes_ratio[] = {
      {.type = StorageImage, .ratio = 3},
  };

  DescriptorAllocator descr_alloc(ctx, 10, sizes_ratio);

  DescriptorLayoutBuilder builder;
  builder.add_binding(0, StorageImage);
  DescriptorSetLayout descr_set_layout =
      builder.build(ctx._device, ComputeShader);

  Raii_VkDescriptorSet descriptor = descr_alloc.allocate(descr_set_layout);

  ctx.immediate_submit([&](VkCommandBuffer cmd) { compute_pipeline.bind(cmd, *descriptor); });

  LOGOK("compute_pipeline_build");
}

#endif