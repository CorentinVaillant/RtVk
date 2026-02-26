#pragma once
#include "graphics/PipelineDescriptor.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <vulkan/vulkan_core.h>

// -- ComputePipeline --

class ComputePipeline {
  // -- Constructors --
public:
  ComputePipeline(VulkanContext &ctx, PipelineDescriptor &descriptor,
                  VkPipelineCreateFlags flags = {});

  NO_COPY(ComputePipeline);

  ~ComputePipeline();

  // -- Methods --

  void bind(VkCommandBuffer cmd, VkDescriptorSet descriptor) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _layout, 0, 1,
                            &descriptor, 0, nullptr);
  }

private:
  void init_sampler(PipelineDescriptor &descriptor);
  void init_descr_set_layout(VulkanContext &ctx,
                             PipelineDescriptor &descriptor);
  void init_layout(PipelineDescriptor &descriptor);
  // -- Attributs --
private:
  VkDevice _deviceCtx;
  VkDescriptorSetLayout _descrSetLayout;
  VkSampler _sampler;
  VkPipelineLayout _layout;

  VkPipeline _pipeline;
};
