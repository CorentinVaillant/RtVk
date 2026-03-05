#pragma once
#include "glm/ext/vector_uint3.hpp"
#include "graphics/PipelineDescriptor.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <concepts>
#include <volk.h>

// -- ComputePipeline --

class ComputePipeline {
  // -- Constructors --
public:
  ComputePipeline(VulkanContext &ctx, PipelineDescriptor &descriptor,
                  glm::uvec3 dispatch_groupe, VkPipelineCreateFlags flags = {});

  NO_COPY(ComputePipeline);

  ~ComputePipeline();

  // -- Getters --
  VkSampler get_sampler() { return _sampler; }

  // -- Methods --

  template <typename PushCst>
    requires std::copy_constructible<PushCst>
  void dispatch(VkCommandBuffer cmd, VkDescriptorSet descriptor,
                PushCst push_cst, glm::uvec3 groups) {
    bind(cmd, descriptor);

    vkCmdPushConstants(cmd, _layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(PushCst), &push_cst);

    vkCmdDispatch(cmd, groups.x / _dispatchGroup.x, groups.y / _dispatchGroup.y,
                  groups.z/_dispatchGroup.z);
  }

private:
  void init_sampler(PipelineDescriptor &descriptor);
  void init_descr_set_layout(VulkanContext &ctx,
                             PipelineDescriptor &descriptor);
  void init_layout(PipelineDescriptor &descriptor);

  void bind(VkCommandBuffer cmd, VkDescriptorSet descriptor);
  // -- Attributs --
private:
  VkDevice _deviceCtx;
  VkDescriptorSetLayout _descrSetLayout;
  VkSampler _sampler;
  VkPipelineLayout _layout;
  glm::uvec3 _dispatchGroup;

  VkPipeline _pipeline;
};
