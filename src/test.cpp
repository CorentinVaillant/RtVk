#include "test.h"
#include "ImageBuffer.h"
#include "graphics/Image.h"
#include "graphics/PipelineDescriptor.h"
#include "graphics/Shaders.h"
#include "graphics/pipelines.h"
#include "graphics/raii_graphic.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <cassert>
#include <stdexcept>

#ifdef NTEST
#include "graphics/utils.h"
#include <vulkan/vulkan_core.h>

struct PushCstTest {
  glm::vec2 start = glm::vec2(0);
  float zoom = 3;
  uint spp = 20;
  uint power = 2;
  float palette_offset = 0;
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

  [[maybe_unused]] auto descriptor = descr_alloc.allocate(descr_set_layout);

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

  ComputePipeline compute_pipeline(ctx, descr, {16, 16, 1});

  DescriptorAllocator::PoolSizeRatio sizes_ratio[] = {
      {.type = StorageImage, .ratio = 3},
  };

  DescriptorAllocator descr_alloc(ctx, 10, sizes_ratio);

  DescriptorLayoutBuilder builder;
  builder.add_binding(0, StorageImage);
  DescriptorSetLayout descr_set_layout =
      builder.build(ctx._device, ComputeShader);

  Raii_VkDescriptorSet descriptor = descr_alloc.allocate(descr_set_layout);

  constexpr size_t IMG_SIZE = 1000;

  PushCstTest push_cst{};

  Image dispatch_result(
      ctx, {IMG_SIZE, IMG_SIZE, 1}, RGBA,
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, General);

  DescriptorWriter writter;
  dispatch_result.write(writter, 0, compute_pipeline.get_sampler(),
                        StorageImage);

  writter.update_set(ctx._device, *descriptor);

  ctx.immediate_submit([&](VkCommandBuffer cmd) {
    compute_pipeline.dispatch(cmd, *descriptor, push_cst,
                              {IMG_SIZE, IMG_SIZE, 1});
  });

  ImageBuffer img_buff(IMG_SIZE, IMG_SIZE, RGBA);
  img_buff.read_from_gpu(ctx, dispatch_result);

  img_buff.write_on_disk("mandelbrot.png", PNG);

  LOGOK("compute_pipeline_build");
}

#endif