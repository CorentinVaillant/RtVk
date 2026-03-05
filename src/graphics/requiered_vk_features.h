#pragma once
#include <initializer_list>
#include <volk.h>

// features
constexpr VkPhysicalDeviceAccelerationStructureFeaturesKHR REQUIRED_ACC_STRUCT_FEATURES = {
    .sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
    .accelerationStructure = VK_TRUE,
};

constexpr VkPhysicalDeviceRayTracingPipelineFeaturesKHR REQUIRED_RT_FEATURES = {
    .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
    .rayTracingPipeline = VK_TRUE,
}; 

// vk 1.3 features
constexpr VkPhysicalDeviceVulkan13Features REQUIRED_VULKAN_13_FEATURES = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .synchronization2 = true,
    .dynamicRendering = true,
};

// vk 1.2 features
constexpr VkPhysicalDeviceVulkan12Features REQUIRED_VULKAN_12_FEATURES = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .bufferDeviceAddress = true,
};

// vk 1.1 features
constexpr VkPhysicalDeviceVulkan11Features REQUIRED_VULKAN_11_FEATURES = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
};

// extensions

constexpr std::initializer_list<const char *> REQUIRED_DEVICE_EXTENSIONS = {
    // ray tracing pipeline extensions
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
};
