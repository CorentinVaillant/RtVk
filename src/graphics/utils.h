#pragma once

#include <vulkan/vulkan_core.h>

inline VkImageSubresourceRange
image_subresource_range(VkImageAspectFlags aspect_mask) {
  VkImageSubresourceRange subImage{};
  subImage.aspectMask = aspect_mask;
  subImage.baseMipLevel = 0;
  subImage.levelCount = VK_REMAINING_MIP_LEVELS;
  subImage.baseArrayLayer = 0;
  subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

  return subImage;
}

inline void transition_image(VkCommandBuffer cmd, VkImage image,
                             VkImageLayout curr_layout,
                             VkImageLayout new_layout) {

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