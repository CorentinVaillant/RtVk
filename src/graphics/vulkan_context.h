#pragma once
#include "SDL3/SDL_video.h"
#include "delqueue.h"
#include "types.h"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class VulkanContext {

public:
  using ImediatFunc = std::function<void(VkCommandBuffer cmd)>;
  using RunFunc = std::function<void(VulkanContext &context)>;
  static VulkanContext *get();

  static void init(const char *app_name = nullptr);
  static int run(RunFunc run_func);
  static void cleanup();

  VulkanContext() {}
  NO_COPY(VulkanContext);

  static void stop(int exit_code = 0);

  void immediate_submit(ImediatFunc &&func);
  

private:
  // -- Methods
  void init_context(const char *app_name = nullptr);
  void clean_context();
  void init_sdl(const char *app_name);
  void init_vulkan(const char *app_name);
  void init_commands(); // TODO, init cmd buffers (_immediateComandBuffer)

  // -- Attributs
  bool _isInit = false;
  bool _shouldRun = false;
  int _stopReturnCode = 0;
  RunFunc _runFunc = [](VulkanContext &context [[maybe_unused]]) { return; };

  DeletationQueue _mainDelQueue;

public:
  // SDL
  SDL_Window *_window;

  // -- vulkan
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

private:
  // Immediates
  VkFence _immediateFence;
  VkCommandPool _immediateCmdPool;
  VkCommandBuffer _immediateCmd;

  VkExtent2D _windowExtent = {1080, 720};
};
