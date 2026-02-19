#pragma once

#include "graphics/vulkan_context.h"
#include "types.h"
#ifdef NTEST
#include <vulkan/vulkan_core.h>

void test_descriptor_allocator(VulkanContext &ctx);

void test_pipeline_build(VulkanContext &ctx);
void test_shader_loading(VulkanContext& ctx);
void test_compute_pipeline_build(VulkanContext &ctx);

inline void test(VulkanContext &ctx) {
  LOG(1, "Testing...");
  test_descriptor_allocator(ctx);
  test_shader_loading(ctx);
  test_pipeline_build(ctx);

  LOGOK("All test OK !");
}

#else
inline void test(VulkanContext &ctx [[maybe_unused]]) {}
#endif