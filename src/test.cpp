#include "test.h"
#include "graphics/Shaders.h"
#include "graphics/pipeline.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#ifdef NTEST
#include "graphics/utils.h"
#include <vulkan/vulkan_core.h>

struct PushCstTest : IDescriptable {
  glm::vec2 resolution;
  float time;

  static DescriptorSetLayout describe(VkDevice device,
                                      VkShaderStageFlags shader_stages) {

    DescriptorLayoutBuilder builder;
    builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    builder.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    return builder.build(device, shader_stages);
  }
};

void test_descriptor_allocator(VulkanContext &ctx) {
  DescriptorAllocator::PoolSizeRatio sizes_ratio[] = {
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .ratio = 3},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .ratio = 3},
  };

  DescriptorAllocator descr_alloc(ctx, 10, sizes_ratio);

  DescriptorLayoutBuilder builder;
  builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  builder.add_binding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  DescriptorSetLayout descr_set_layout =
      builder.build(ctx._device, VK_SHADER_STAGE_COMPUTE_BIT);

  [[maybe_unused]] VkDescriptorSet descriptor =
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

  // alloc descr

  DescriptorAllocator::PoolSizeRatio sizes_ratio[] = {
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .ratio = 3},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .ratio = 3},
  };

  DescriptorAllocator descr_alloc(ctx, 10, sizes_ratio);
  // shader
  Shader loaded_shader = Shader(ctx, "./shaders/mandelbrot.slang.spv");

  ComputePipeline<PushCstTest> pipeline(ctx, descr_alloc,
                                        std::move(loaded_shader));

  LOGOK("compute_pipeline_build");
}

#endif