#pragma once
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "delqueue.h"
#include "graphics/utils.h"
#include "types.h"
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class Image;

class VulkanContext {

public:
  using ImediatFunc = std::function<void(VkCommandBuffer cmd)>;
  using RunFunc = std::function<void(VulkanContext &context)>;
  using DrawFunc = std::function<void(VulkanContext &context)>;
  using EventCallbackFunc = std::function<void(VulkanContext &, SDL_Event &)>;

  static void init(const char *app_name = nullptr);
  static int run(RunFunc run_func);
  static void set_event_callbacks(EventCallbackFunc callbacks);
  static void cleanup();

  VulkanContext() {}
  NO_COPY(VulkanContext);

  static void stop(int exit_code = 0);

  void immediate_submit(ImediatFunc &&func);
  void draw(Image& img);

private:
  // -- Methods
  void init_context(const char *app_name);
  void clean_context();
  void init_sdl(const char *app_name);
  void init_vulkan(const char *app_name);
  void init_commands();

  void create_swapchain();
  void destroy_swapchain();

  // -- Attributs
  bool _isInit = false;
  bool _shouldRun = false;
  int _stopReturnCode = 0;

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
  // Callbacks
  RunFunc _runFunc = [](VulkanContext &context [[maybe_unused]]) { return; };

  EventCallbackFunc _eventCallback = []([[maybe_unused]] VulkanContext &ctx,
                                        [[maybe_unused]] SDL_Event &e) {};
  DeletationQueue _mainDelQueue;

  // Immediates
  VkFence _immediateFence;
  VkCommandPool _immediateCmdPool;
  VkCommandBuffer _immediateCmd;

  VkExtent2D _windowExtent = {1080, 720};

  // Swapchain
  VkSwapchainKHR _swapchain;
  std::vector<VkImage> _swapchainImages;
  std::vector<VkImageView> _swapchainImageViews;

  VkFormat _swapchainFormat;
  VkExtent2D _swapchainExtent;

  VkSemaphore _imageAvailable;
  VkSemaphore _renderFinished;
  VkFence _inFlightFence; // todo > rename
};
