#pragma once

#include "graphics/raii_graphic.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <volk.h>

class VulkanContext;

// -- Utils functions

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);

void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout curr_layout, VkImageLayout new_layout);

// -- Utils Classes

enum DescriptorType {
  StorageImage = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
  UniformBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  StorageBuffer = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
  AccelerationStruct = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
  // ...
};

enum SinglePipelineStage {
  ComputeStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,

  VertexStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
  FragmentStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
  // ...
};

struct PipelineStages {
  VkPipelineStageFlags _vkPipelineStageFlags = 0x0;

  PipelineStages(const std::initializer_list<SinglePipelineStage> &stages) {
    for (auto stage : stages)
      _vkPipelineStageFlags |= static_cast<VkPipelineStageFlagBits>(stage);
  }
  PipelineStages(SinglePipelineStage stage) : PipelineStages({stage}) {}
};

enum SingleShaderStage {
  ComputeShader = VK_SHADER_STAGE_COMPUTE_BIT,

  VertexShader = VK_SHADER_STAGE_VERTEX_BIT,
  FragmentShader = VK_SHADER_STAGE_FRAGMENT_BIT,

  Raygen = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
  ClosestHit = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
  Miss = VK_SHADER_STAGE_MISS_BIT_KHR,
  Intersection = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,

};

struct ShaderStages {
  VkShaderStageFlags _vkShaderStageFlags = 0x0;

  ShaderStages(const std::initializer_list<SingleShaderStage> &stages) {
    for (auto stage : stages)
      _vkShaderStageFlags |= static_cast<VkShaderStageFlagBits>(stage);
  }
  ShaderStages(SingleShaderStage stage) : ShaderStages({stage}) {}
};

class DescriptorSetLayout {
public:
  DescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descr_set_layout);
  NO_COPY(DescriptorSetLayout);

  // -- Move constructor --
  DescriptorSetLayout(DescriptorSetLayout &&rval)
      : _descrSetLayout(rval._descrSetLayout), _ctxDevice(rval._ctxDevice) {}

  DescriptorSetLayout &operator=(DescriptorSetLayout &&other) {
    if (this != &other) {
      _descrSetLayout = other._descrSetLayout;
      _ctxDevice = other._ctxDevice;
    }

    return *this;
  }

  ~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(_ctxDevice, _descrSetLayout, nullptr);
  }

public:
  VkDescriptorSetLayout _descrSetLayout;

private:
  VkDevice _ctxDevice;
};

class DescriptorAllocator {
public:
  struct PoolSizeRatio {
    DescriptorType type;
    float ratio;
  };

  DescriptorAllocator(VulkanContext &ctx, uint32_t init_size,
                      std::span<const PoolSizeRatio> sizes);
  NO_COPY(DescriptorAllocator);

  ~DescriptorAllocator();

  // Methods
  void clear();
  Raii_VkDescriptorSet allocate(VkDescriptorSetLayout layout,
                                void *pNext = nullptr) {

    VkDescriptorPool pool_to_use = get_pool();

    VkDescriptorSetAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = pNext,
        .descriptorPool = pool_to_use,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet result_descr_set;

    VkResult result =
        vkAllocateDescriptorSets(_ctxDevice, &alloc_info, &result_descr_set);

    // alloc failed
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY ||
        result == VK_ERROR_FRAGMENTED_POOL) {
      _fullPools.push_back(pool_to_use);
      pool_to_use = get_pool();
      alloc_info.descriptorPool = pool_to_use;

      VK_CHECK(
          vkAllocateDescriptorSets(_ctxDevice, &alloc_info, &result_descr_set));
    } else {
      VK_CHECK(result);
    }

    _readyPools.push_back(pool_to_use);
    return Raii_VkDescriptorSet(result_descr_set, {_ctxDevice, pool_to_use});
  }
  Raii_VkDescriptorSet allocate(DescriptorSetLayout &layout) {
    return allocate(layout._descrSetLayout);
  }

private:
  VkDescriptorPool get_pool();
  VkDescriptorPool create_pool();

  // Members
  VkDevice _ctxDevice;
  std::vector<PoolSizeRatio> _ratios;
  std::vector<VkDescriptorPool> _fullPools;
  std::vector<VkDescriptorPool> _readyPools;
  uint32_t _setPerPools;

  static constexpr size_t MAX_SIZE = 4096;
};

// -- DescriptorWritter

class DescriptorWriter {

public:
  DescriptorWriter() {};
  NO_COPY(DescriptorWriter);

  // Methods
  void write_image(uint32_t binding, VkImageView view, VkSampler sampler,
                   VkImageLayout layout, VkDescriptorType descriptor_type);
  void write_buffer(uint32_t binding, VkBuffer buffer, size_t size,
                    size_t offset, VkDescriptorType descriptor_type);

  void clear();
  void update_set(VkDevice device, VkDescriptorSet set);

  // Attributs
private:
  std::deque<VkDescriptorImageInfo> _img_infos;
  std::deque<VkDescriptorBufferInfo> _buffer_infos;
  std::vector<VkWriteDescriptorSet> _writes;
};

// -- Utils Builders

struct DescriptorLayoutBuilder {
  std::vector<VkDescriptorSetLayoutBinding> _bindings;

  DescriptorLayoutBuilder &add_binding(uint32_t binding, DescriptorType type,
                                       ShaderStages stages = {});
  void clear() { _bindings.clear(); }
  DescriptorSetLayout build(VkDevice device, ShaderStages stages,
                            void *pNext = nullptr,
                            VkDescriptorSetLayoutCreateFlags flags = 0);
};
