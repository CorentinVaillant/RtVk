#include "pipelines.h"
#include "glm/ext/vector_uint3.hpp"
// -- ComputePipeline --

// -- Constructors --
// public:
ComputePipeline::ComputePipeline(VulkanContext &ctx,
                                 PipelineDescriptor &descriptor,
                                 glm::uvec3 dispatch_groupe,
                                 VkPipelineCreateFlags flags /* = {} */)
    : _deviceCtx(ctx._device), _dispatchGroup(dispatch_groupe) {

  auto stages = descriptor.get_stages_infos();
  assert(stages.size() == 1);

  init_sampler(descriptor);
  init_descr_set_layout(ctx, descriptor);
  init_layout(descriptor);

  VkComputePipelineCreateInfo create_info = VkComputePipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = {},
      .stage = stages[0],
      .layout = _layout,
      .basePipelineHandle = {}, // No inheritance
      .basePipelineIndex = {},
  };
  
  VK_CHECK(vkCreateComputePipelines(_deviceCtx, VK_NULL_HANDLE, 1, &create_info,
                                    nullptr, &_pipeline));
}

ComputePipeline::~ComputePipeline() {
  vkDestroyPipeline(_deviceCtx, _pipeline, nullptr);
  vkDestroyPipelineLayout(_deviceCtx, _layout, nullptr);
  vkDestroySampler(_deviceCtx, _sampler, nullptr);
  vkDestroyDescriptorSetLayout(_deviceCtx, _descrSetLayout, nullptr);
  ;
}

// -- Methods --

void ComputePipeline::bind(VkCommandBuffer cmd, VkDescriptorSet descriptor) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _layout, 0, 1,
                          &descriptor, 0, nullptr);
}

// private:
void ComputePipeline::init_sampler(PipelineDescriptor &descriptor) {
  VK_CHECK(vkCreateSampler(_deviceCtx, &descriptor._samplerInfo, nullptr,
                           &_sampler));
}

void ComputePipeline::init_descr_set_layout(VulkanContext &ctx,
                                            PipelineDescriptor &descriptor) {
  _descrSetLayout = descriptor.create_descriptor_binding(ctx, _sampler);
}

void ComputePipeline::init_layout(PipelineDescriptor &descriptor) {
  VkPipelineLayoutCreateInfo create_info = descriptor._layoutInfo;
  create_info.pSetLayouts = &_descrSetLayout;
  create_info.pPushConstantRanges = &descriptor._pushCstRange;

  vkCreatePipelineLayout(_deviceCtx, &create_info, nullptr, &_layout);
}
