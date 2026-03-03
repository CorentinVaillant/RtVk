#include "vulkan_context.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
#include "graphics/requiered_vk_features.h"
#include "graphics/utils.h"
#include "types.h"
#include <VkBootstrap.h>
#include <cassert>
#include <vulkan/vulkan_core.h>

#include "Image.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#ifdef NDEBUG
bool s_use_validation_layers = true;
#else
bool s_use_validation_layers = false;
#endif

// -- Static impl --
static VulkanContext static_context;
bool static_is_init = false;

void VulkanContext::init(const char *app_name /* = nullptr */) {
  static_context.init_context(app_name);
}
int VulkanContext::run(RunFunc run_func) {
  assert(static_is_init);
  static_context._shouldRun = true;
  while (static_context._shouldRun) {
    run_func(static_context);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      static_context._eventCallback(static_context, e);
    }
  }
  return static_context._stopReturnCode;
}
void VulkanContext::set_event_callbacks(EventCallbackFunc callbacks) {
  assert(static_is_init);
  static_context._eventCallback = callbacks;
}

void VulkanContext::cleanup() {
  assert(static_is_init);
  static_context.clean_context();
}

void VulkanContext::stop(int exit_code /*= 0*/) {
  assert(static_is_init);
  static_context._shouldRun = false;
  static_context._stopReturnCode = exit_code;
}

// -- Public impl --

void VulkanContext::immediate_submit(ImediatFunc &&func) {
  assert(_isInit);
  VK_CHECK(vkResetFences(_device, 1, &_immediateFence));
  VK_CHECK(vkResetCommandBuffer(_immediateCmd, 0));

  VkCommandBuffer cmd = _immediateCmd;

  VkCommandBufferBeginInfo cmd_begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr,
  };

  VK_CHECK(vkBeginCommandBuffer(_immediateCmd, &cmd_begin_info));
  func(cmd);
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmd_submit_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .pNext = nullptr,
      .commandBuffer = cmd,
      .deviceMask = 0};
  VkSubmitInfo2 submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .pNext = nullptr,
      .flags = 0,
      .waitSemaphoreInfoCount = 0,
      .pWaitSemaphoreInfos = nullptr,

      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &cmd_submit_info,

      .signalSemaphoreInfoCount = 0,
      .pSignalSemaphoreInfos = nullptr,
  };

  VK_CHECK(vkQueueSubmit2(_graphicQueue, 1, &submit_info, _immediateFence));
  VK_CHECK(vkWaitForFences(_device, 1, &_immediateFence, VK_TRUE, 999'999'999));
}

void VulkanContext::draw(Image &img) {
  VK_CHECK(vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX));
  VK_CHECK(vkResetFences(_device, 1, &_inFlightFence));

  uint32_t image_index;
  VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX,
                                 _imageAvailable, VK_NULL_HANDLE,
                                 &image_index));

  VkCommandBuffer &cmd = _immediateCmd; // ? maybe make it's own cmd

  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = {},
      .pInheritanceInfo = nullptr,
  };

  VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

  // Copy the image to the swapchain one

  transition_image(cmd, _swapchainImages[image_index],
                   VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  ImgLayout img_layout = img.get_layout();

  img.transition(cmd, TransferSrcOpt);

  VkExtent3D img_extent = img.get_size();
  VkImageCopy copy_region{};
  copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.srcSubresource.mipLevel = 0;
  copy_region.srcSubresource.layerCount = 1;
  copy_region.srcSubresource.baseArrayLayer = 0;
  copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.dstSubresource.mipLevel = 0;
  copy_region.dstSubresource.layerCount = 1;
  copy_region.dstSubresource.baseArrayLayer = 0;
  copy_region.extent.width = std::min(img_extent.width, _swapchainExtent.width);
  copy_region.extent.height =
      std::min(img_extent.height, _swapchainExtent.height);
  copy_region.extent.depth = 1;

  vkCmdCopyImage(cmd, img._vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 _swapchainImages[image_index],
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

  transition_image(cmd, _swapchainImages[image_index],
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  img.transition(cmd, img_layout);

  VK_CHECK(vkEndCommandBuffer(cmd));

  // Submit

  const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  VkSubmitInfo sumbit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &_imageAvailable,
      .pWaitDstStageMask = &wait_stage,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &_renderFinished,
  };

  VK_CHECK(vkQueueSubmit(_graphicQueue, 1, &sumbit_info, _inFlightFence));

  // Present

  VkResult swapchain_result = VK_SUCCESS;

  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &_renderFinished,
      .swapchainCount = 1,
      .pSwapchains = &_swapchain,
      .pImageIndices = &image_index,
      .pResults = &swapchain_result,
  };

  VK_CHECK(vkQueuePresentKHR(_graphicQueue, &present_info));

  VK_CHECK(swapchain_result);

  VK_CHECK(vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX));
}

// -- Private impl --
void VulkanContext::init_context(const char *app_name) {
  LOG(1, "Initialasing the context...");
  LOG(2, "Use of validation layers is set to {}", s_use_validation_layers);
  assert(static_is_init == false);
  assert(_isInit == false);

  const char *use_app_name = app_name ? app_name : "Vulkan app";

  init_sdl(use_app_name);
  init_vulkan(use_app_name);
  init_commands();
  create_swapchain();

  // ...

  LOG(1, "Context init at {}.", static_cast<void *>(&static_context));
  static_is_init = true;
  _isInit = true;
}

void VulkanContext::clean_context() {
  vkDeviceWaitIdle(_device);

  _mainDelQueue.flush();
}

// -- Init functions --
void VulkanContext::init_sdl(const char *app_name) {

  SDL_Init(SDL_INIT_VIDEO);
  SDL_WindowFlags window_flags =
      static_cast<SDL_WindowFlags>(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  _window = SDL_CreateWindow(app_name, _windowExtent.width,
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
  SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);
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
  LOG(2, "physical device init.");
  LOG(2, "   => Loaded physical device :{}", selector_ret.value().name);

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

  // init fence
  VkFenceCreateInfo fence_create_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0};

  vkCreateFence(_device, &fence_create_info, nullptr, &_immediateFence);

  // init comand buffer

  _mainDelQueue.push_function([this]() {
    vkDestroyFence(_device, _immediateFence, nullptr);
    vmaDestroyAllocator(_memAllocator);
    // Queues don't need to be destroyed
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debugUtilMsg);
    vkDestroyInstance(_instance, nullptr);
  });
}

void VulkanContext::init_commands() {
  // init immediate cmd
  VkCommandPoolCreateInfo cmd_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = _graphicQueueFamily,
  };

  VK_CHECK(vkCreateCommandPool(_device, &cmd_pool_create_info, nullptr,
                               &_immediateCmdPool));

  VkCommandBufferAllocateInfo cmd_buff_alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = _immediateCmdPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};
  VK_CHECK(
      vkAllocateCommandBuffers(_device, &cmd_buff_alloc_info, &_immediateCmd));

  _mainDelQueue.push_function(
      [this]() { vkDestroyCommandPool(_device, _immediateCmdPool, nullptr); });
}

void VulkanContext::create_swapchain() {
  vkb::SwapchainBuilder vkb_builder(_physicalDevice, _device, _surface);

  auto vkb_swapchain_result =
      vkb_builder
          .set_desired_format(VkSurfaceFormatKHR{
              .format = _swapchainFormat,
              .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_extent(_windowExtent.width, _windowExtent.height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build();
  if (!vkb_swapchain_result)
    LOGERR("Could not build the swapchain : {}",
           vkb_swapchain_result.error().message());

  vkb::Swapchain vkb_swapchain = vkb_swapchain_result.value();

  _swapchainExtent = vkb_swapchain.extent;
  _swapchain = vkb_swapchain.swapchain;

  auto swapchain_img_result = vkb_swapchain.get_images();
  if (!swapchain_img_result)
    LOGERR("Could not get the swapchain's images : {}",
           swapchain_img_result.error().message());
  _swapchainImages = swapchain_img_result.value();

  auto swapchain_imgviews_result = vkb_swapchain.get_image_views();
  if (!swapchain_imgviews_result)
    LOGERR("Could not get the swapchain's image views : {}",
           swapchain_imgviews_result.error().message());
  _swapchainImageViews = swapchain_imgviews_result.value();

  VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  vkCreateSemaphore(_device, &semInfo, nullptr, &_imageAvailable);
  vkCreateSemaphore(_device, &semInfo, nullptr, &_renderFinished);

  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFence);

  _mainDelQueue.push_function([&, this]() {
    vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, 1'000'000);
    vkDestroyFence(_device, _inFlightFence, nullptr);

    vkDestroySemaphore(_device, _imageAvailable, nullptr);
    vkDestroySemaphore(_device, _renderFinished, nullptr);

    destroy_swapchain();
  });
}

void VulkanContext::destroy_swapchain() {
  for (VkImageView img_view : _swapchainImageViews)
    vkDestroyImageView(_device, img_view, nullptr);

  vkDestroySwapchainKHR(_device, _swapchain, nullptr);
}
