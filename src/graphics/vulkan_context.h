#pragma once
#include "SDL3/SDL_video.h"
#include "delqueue.h"
#include "types.h"
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

class VulkanContext {
public:
  static VulkanContext *get();

  NO_COPY(VulkanContext);

  VulkanContext() {}

  // -- Methods
  void init(const char *app_name = nullptr);

  void clean();

private:
  void init_sdl(const char *app_name);
  void init_vulkan(const char *app_name);

  // -- Attributs
  bool _isInit = false;

  DeletationQueue _mainDelQueue;

  // SDL

  SDL_Window *_window;

  // vulkan
  VkInstance _instance;
  VkDebugUtilsMessengerEXT _debugUtilMsg;
  VkSurfaceKHR _surface;
  VkPhysicalDevice _physicalDevice;
  VkDevice _device;
  VkQueue _graphicQueue;
  uint32_t _graphicQueueFamily;
  VkQueue _computeQueue;
  uint32_t _computeQueueFamily;
  VmaAllocator _memAllocator;

  VkExtent2D _windowExtent = {1080, 720};
};
