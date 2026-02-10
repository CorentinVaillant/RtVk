#pragma once

// vk 1.3 features
#include <vulkan/vulkan_core.h>
constexpr VkPhysicalDeviceVulkan13Features REQUIRED_VULKAN_13_FEATURES = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .synchronization2 = true,
    .dynamicRendering = true,
};

// vk 1.2 features
#include <vulkan/vulkan_core.h>
constexpr VkPhysicalDeviceVulkan12Features REQUIRED_VULKAN_12_FEATURES = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
};

// vk 1.1 features
#include <vulkan/vulkan_core.h>
constexpr VkPhysicalDeviceVulkan11Features REQUIRED_VULKAN_11_FEATURES = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
};
