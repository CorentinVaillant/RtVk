#pragma once

#include "graphics/vulkan_context.h"
#include "types.h"
#include <type_traits>

// -- Utils functions

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);

void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout curr_layout, VkImageLayout new_layout);

// -- Utils Classes

class DescriptorAllocator;

class DescriptorSetLayout {
public:
  DescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descr_set_layout);
  NO_COPY(DescriptorSetLayout);

  // -- Move constructor --
  DescriptorSetLayout(DescriptorSetLayout &&rval)
      : _descrSetLayout(rval._descrSetLayout), _ctxDevice(rval._ctxDevice) {}

  DescriptorSetLayout &operator=(DescriptorSetLayout &&other) {
    if(this != &other){
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
    VkDescriptorType type;
    float ratio;
  };

  DescriptorAllocator(VulkanContext &ctx, uint32_t init_size,
                      std::span<PoolSizeRatio> sizes);
  NO_COPY(DescriptorAllocator);

  ~DescriptorAllocator();

  // Methods
  void clear();
  VkDescriptorSet allocate(VkDescriptorSetLayout layout, void *pNext = nullptr);
  VkDescriptorSet allocate(DescriptorSetLayout &layout) {
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

// -- Utils Builders

struct DescriptorLayoutBuilder {
  std::vector<VkDescriptorSetLayoutBinding> _bindings;

  DescriptorLayoutBuilder &add_binding(uint32_t binding, VkDescriptorType type);
  void clear() { _bindings.clear(); }
  DescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages,
                            void *pNext = nullptr,
                            VkDescriptorSetLayoutCreateFlags flags = 0);
};

// -- Utils Interfaces

class IDescriptable {
public:
  static DescriptorSetLayout describe(VkDevice device,
                                      VkShaderStageFlags shader_stages) {
    DescriptorLayoutBuilder builder;
    builder.clear();
    return builder.build(device, shader_stages);
  };
};

// -- Concepts

template <typename T>
concept Descriptable = std::is_base_of<IDescriptable, T>::value;