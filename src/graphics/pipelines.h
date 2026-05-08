#pragma once
#include "glm/ext/vector_uint3.hpp"
#include "graphics/Buffer.h"
#include "graphics/PipelineDescriptor.h"
#include "graphics/utils.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <cassert>
#include <concepts>
#include <cstdint>
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
                  groups.z / _dispatchGroup.z);
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

// -- RT pipeline --

struct RtPipelineCreateInfos {
  uint32_t max_rt_depth = 10;
  uint32_t raygen_count, miss_count, hit_count;
  uint32_t hit_stride_count;
  std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

  uint32_t group_count() const { return raygen_count + miss_count + hit_count; }
};

class RtPipeline {
  static constexpr ShaderStages ALL_STAGES = {Raygen, ClosestHit, Miss,
                                              Intersection};

public:
  NO_COPY(RtPipeline);

  RtPipeline(VulkanContext &ctx, PipelineDescriptor &descriptor,
             RtPipelineCreateInfos &pipeline_infos);

  ~RtPipeline();

  // -- Getters
  const Buffer<uint8_t> &get_sbt_buffer() const { return _sbtBuffer.value(); }
  const VkSampler get_sampler() const { return _sampler; }
  const VkDescriptorSetLayout get_descr_set_layout() const {
    return _desrSetLayout;
  }

  // -- Methods --
  void bind(VkCommandBuffer cmd, VkDescriptorSet descriptor);

  template <typename PushCst>
    requires std::copy_constructible<PushCst>
  void dispatch(VkCommandBuffer cmd, VkDescriptorSet descriptor,
                glm::uvec3 draw_region, PushCst push_cst) {
    bind(cmd, descriptor);

    vkCmdPushConstants(cmd, _layout, ALL_STAGES._vkShaderStageFlags, 0,
                       sizeof(PushCst), &push_cst);

    vkCmdTraceRaysKHR(cmd, &_raygen_region, &_miss_region, &_hit_region,
                      &_callable_region, draw_region.x, draw_region.y,
                      draw_region.z);
  }

private:
  void init_sampler(PipelineDescriptor &descriptor);

  void init_descr_set_layout(VulkanContext &ctx,
                             PipelineDescriptor &descriptor);

  void init_layout(PipelineDescriptor &descriptor);

private:
  VkDevice _ctxDevice;

  std::optional<Buffer<uint8_t>> _sbtBuffer = std::nullopt;

  VkSampler _sampler;
  VkDescriptorSetLayout _desrSetLayout;
  VkPipelineLayout _layout;

  VkPipeline _rtPipeline;

  VkStridedDeviceAddressRegionKHR _raygen_region, _miss_region, _hit_region,
      _callable_region;
};
