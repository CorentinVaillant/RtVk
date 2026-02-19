#include "graphics/utils.h"
#include "types.h"
#include <vulkan/vulkan_core.h>

// -- Utils functions --

VkImageSubresourceRange
image_subresource_range(VkImageAspectFlags aspect_mask) {
  VkImageSubresourceRange subImage{};
  subImage.aspectMask = aspect_mask;
  subImage.baseMipLevel = 0;
  subImage.levelCount = VK_REMAINING_MIP_LEVELS;
  subImage.baseArrayLayer = 0;
  subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

  return subImage;
}

void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout curr_layout, VkImageLayout new_layout) {

  VkImageAspectFlags aspect =
      new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
          ? VK_IMAGE_ASPECT_DEPTH_BIT
          : VK_IMAGE_ASPECT_COLOR_BIT;

  VkImageMemoryBarrier2 img_barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .pNext = nullptr,

      .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
      .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
      .dstAccessMask =
          VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,

      .oldLayout = curr_layout,
      .newLayout = new_layout,
      //   .srcQueueFamilyIndex = 0,
      //   .dstQueueFamilyIndex = 0,
      .image = image,
      .subresourceRange = image_subresource_range(aspect),
  };

  VkDependencyInfo dep_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .pNext = nullptr,
      .dependencyFlags = {},
      .memoryBarrierCount = 0,
      .pMemoryBarriers = nullptr,
      .bufferMemoryBarrierCount = 0,
      .pBufferMemoryBarriers = nullptr,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &img_barrier,
  };
  vkCmdPipelineBarrier2(cmd, &dep_info);
}

// -- Utils Builders --

// -- DescriptorLayoutBuilder

DescriptorLayoutBuilder& DescriptorLayoutBuilder::add_binding(uint32_t binding,
                                          VkDescriptorType type) {
  VkDescriptorSetLayoutBinding new_bind = {
      .binding = binding,
      .descriptorType = type,
      .descriptorCount = 1,
      .stageFlags = {},              // Will be init when building
      .pImmutableSamplers = nullptr, // defaulted
  };

  _bindings.push_back(new_bind);
  return *this;
}

DescriptorSetLayout DescriptorLayoutBuilder::build(
    VkDevice device, VkShaderStageFlags shader_stages,
    void *pNext /* = nullptr */,
    VkDescriptorSetLayoutCreateFlags flags /* = 0 */) {
  for (auto &b : _bindings)
    b.stageFlags |= shader_stages;

  VkDescriptorSetLayoutCreateInfo infos = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = pNext,
      .flags = flags,
      .bindingCount = static_cast<uint32_t>(_bindings.size()),
      .pBindings = _bindings.data(),
  };

  VkDescriptorSetLayout result_set;
  VK_CHECK(vkCreateDescriptorSetLayout(device, &infos, nullptr, &result_set));
  return DescriptorSetLayout(device, result_set);
}

// -- Utils Classes --

// -- DescriptorLayout
DescriptorSetLayout::DescriptorSetLayout(VkDevice device,
                                         VkDescriptorSetLayout descr_set_layout)
    : _descrSetLayout(descr_set_layout), _ctxDevice(device) {}

// -- DescriptorAllocator

DescriptorAllocator::DescriptorAllocator(VulkanContext &ctx, uint32_t init_size,
                                         std::span<PoolSizeRatio> sizes)
    : _ctxDevice(ctx._device),
      _ratios(sizes.data(), sizes.data() + sizes.size()),_setPerPools(init_size) {

  VkDescriptorPool new_pool = create_pool();

  _setPerPools = init_size * 1.5; // grow for the next allocation

  _readyPools.push_back(new_pool);
}

DescriptorAllocator::~DescriptorAllocator() {
  for (auto p : _readyPools) {
    vkDestroyDescriptorPool(_ctxDevice, p, 0);
  }
  for (auto p : _fullPools) {
    vkDestroyDescriptorPool(_ctxDevice, p, 0);
  }
  _fullPools.clear();
  _readyPools.clear();


}

void DescriptorAllocator::clear() {
  for (auto p : _readyPools) {
    vkResetDescriptorPool(_ctxDevice, p, 0);
  }
  for (auto p : _fullPools) {
    vkResetDescriptorPool(_ctxDevice, p, 0);
    _readyPools.push_back(p);
  }
  _fullPools.clear();
}

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout,
                                              void *pNext /* = nullptr */) {

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
  return result_descr_set;
}

VkDescriptorPool DescriptorAllocator::get_pool() {

  VkDescriptorPool new_pool;
  if (_readyPools.size() > 0) {
    new_pool = _readyPools.back();
    _readyPools.pop_back();
  } else {
    // need to create a pool
    new_pool = create_pool();
    _setPerPools *= 1.5; // grow for the next allocation
    if (_setPerPools > MAX_SIZE)
      _setPerPools = MAX_SIZE;
  }

  return new_pool;
}

VkDescriptorPool DescriptorAllocator::create_pool() {
  std::vector<VkDescriptorPoolSize> pool_sizes;
  for (PoolSizeRatio ratio : _ratios)
    pool_sizes.push_back(VkDescriptorPoolSize{
        .type = ratio.type,
        .descriptorCount = static_cast<uint32_t>(ratio.ratio * _setPerPools),
    });

  VkDescriptorPoolCreateInfo pool_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = {}, // defaulted
      .maxSets = _setPerPools,
      .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
      .pPoolSizes = pool_sizes.data(),
  };

  VkDescriptorPool new_pool;
  VK_CHECK(vkCreateDescriptorPool(_ctxDevice, &pool_info, nullptr, &new_pool));
  return new_pool;
}
