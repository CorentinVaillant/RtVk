#include "vulkan_context.h"
#include "SDL.h"
#include "SDL_video.h"
#include "SDL_vulkan.h"
#include "graphics/requiered_vk_features.h"
#include "types.h"
#include <VkBootstrap.h>
#include <cassert>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#ifdef NDEBUG
bool s_use_validation_layers = true;
#else
bool s_use_validation_layers = false;
#endif

// -- Static impl --
VulkanContext *static_context = nullptr;
bool static_is_init = false;

VulkanContext *VulkanContext::get() { return static_context; }

void VulkanContext::init(const char *app_name /* = nullptr */) {
  LOG(1, "Initialasing the context...");
  assert(static_context == nullptr);
  static_context = this;

  const char *use_app_name = app_name ? app_name : "Vulkan app";

  init_sdl(use_app_name);
  init_vulkan(use_app_name);

  // ...

  LOG(1, "Context init at {}.", static_cast<void *>(static_context));
}

void VulkanContext::clean() {
  assert(_isInit);

  _mainDelQueue.flush();
}

// -- Init functions --
void VulkanContext::init_sdl(const char *app_name) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_WindowFlags window_flags =
      static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  _window = SDL_CreateWindow(app_name, SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, _windowExtent.width,
                             _windowExtent.height, window_flags);

  _mainDelQueue.push_function([this]() { SDL_DestroyWindow(_window); });
}

void VulkanContext::init_vulkan(const char *app_name) {
  // init instance
  vkb::InstanceBuilder inst_builder;
  auto inst_ret = inst_builder.set_app_name(app_name)
                      .request_validation_layers(s_use_validation_layers)
                      .require_api_version(1, 3, 0) // ? maybe change version
                      .use_default_debug_messenger()
                      .build();

  if (!inst_ret)
    LOGERR("Could not build instance with vkb : {}",
           inst_ret.error().message());

  _instance = inst_ret.value().instance;
  _debugUtilMsg = inst_ret.value().debug_messenger;

  LOG(2, "Instance init.");

  // init surface
  SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
  LOG(2, "Surface init.");

  // init physical device
  vkb::PhysicalDeviceSelector selector(inst_ret.value());
  auto selector_ret = selector.set_minimum_version(1, 3)
                          .set_required_features_13(REQUIRED_VULKAN_13_FEATURES)
                          .set_required_features_12(REQUIRED_VULKAN_12_FEATURES)
                          .set_required_features_11(REQUIRED_VULKAN_11_FEATURES)
                          .set_surface(_surface)
                          .select();
  if (!selector_ret)
    LOGERR("Could select the device with vkb : {}",
           selector_ret.error().message());

  _physicalDevice = selector_ret.value().physical_device;
  LOG(2, "physical device init.\n => Loaded physical device :{}",
      selector_ret.value().name);

  // init device
  vkb::DeviceBuilder device_builder{selector_ret.value()};
  auto device_ret = device_builder.build();
  if (!device_ret)
    LOGERR("Could not build the device with vkb : {}",
           device_ret.error().message());
  vkb::Device vkb_device = device_ret.value();
  _device = vkb_device.device;
  LOG(2, "Device init.");

  // init queues
  auto graphic_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
  if (!graphic_queue_ret)
    LOGERR("Could not get the graphic queue with vkb : {}",
           graphic_queue_ret.error().message());
  _graphicQueue = graphic_queue_ret.value();
  auto queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics);
  if (!queue_family)
    LOGERR("Could not get the graphic queue index with vkb : {}",
           queue_family.error().message());

  auto compute_queue_ret = vkb_device.get_queue(vkb::QueueType::compute);
  if (!compute_queue_ret)
    LOGERR("Could not get the graphic queue with vkb : {}",
           graphic_queue_ret.error().message());
  _computeQueue = compute_queue_ret.value();
  queue_family = vkb_device.get_queue_index(vkb::QueueType::compute);
  if (!queue_family)
    LOGERR("Could not get the compute queue index with vkb : {}",
           queue_family.error().message());

  // init VMA allocator
  VmaAllocatorCreateInfo allocator_create_info{};
  allocator_create_info.device = _device;
  allocator_create_info.physicalDevice = _physicalDevice;
  allocator_create_info.instance = _instance;
  allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&allocator_create_info, &_memAllocator);

  _mainDelQueue.push_function([this]() {
    vmaDestroyAllocator(_memAllocator);
    // Queues don't need to be destroyed
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debugUtilMsg);
    vkDestroyInstance(_instance, nullptr);
  });
}