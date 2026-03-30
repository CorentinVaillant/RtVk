#include "ImageBuffer.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_scancode.h"

#include "Scene.h"
#include "types.h"

#include "graphics/vulkan_context.h"

#include "renderer/CPURenderer.h"
#include "renderer/GPURenderer.h"

#include "test.h"

constexpr bool GPU_RENDER = true;

int main() {
  LOG(1, "Starting app !");

  std::unique_ptr<Renderer> renderer = nullptr;

  auto scenes = Scene::load_from_gltf("../resources/test_scene.gltf");
  if (scenes.empty())
    LOGERR("No scene were load !");
  size_t curr_scene_index = 0;
  auto *curr_scene = &scenes[curr_scene_index];

  size_t runned = 0;
  float dt = 1.f / 60.f;
  Microsecond runtime;
  bool render_changed = true;

  VulkanContext::init("RtVk");

  VulkanContext::set_event_callbacks([&](VulkanContext &ctx, SDL_Event &event) {
    if (event.type == SDL_EVENT_QUIT)
      VulkanContext::stop();

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_ESCAPE)
      VulkanContext::stop();

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_LEFT) {
      curr_scene_index =
          std::clamp((curr_scene_index - 1)%scenes.size(), 0lu, scenes.size() - 1);
      LOG(4, "Switched to scene {}", curr_scene_index);
      curr_scene = &scenes[curr_scene_index];
      render_changed = true;
    }

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_RIGHT) {
      curr_scene_index =
          std::clamp((curr_scene_index + 1)%scenes.size(), 0lu, scenes.size() - 1);
      LOG(4, "Switched to scene {}", curr_scene_index);
      curr_scene = &scenes[curr_scene_index];
      render_changed = true;
    }

    constexpr float MOVE_SPEED = 10.f;

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_W) {
      auto &curr_cam = curr_scene->_camera[curr_scene->_active_camera];
      curr_cam.move_forward(MOVE_SPEED * dt);
      render_changed = true;
    }

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_S) {
      auto &curr_cam = curr_scene->_camera[curr_scene->_active_camera];
      curr_cam.move_backward(MOVE_SPEED * dt);
      render_changed = true;
    }

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_A) {
      auto &curr_cam = curr_scene->_camera[curr_scene->_active_camera];
      curr_cam.move_left(MOVE_SPEED * dt);
      render_changed = true;
    }

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_D) {
      auto &curr_cam = curr_scene->_camera[curr_scene->_active_camera];
      curr_cam.move_right(MOVE_SPEED * dt);
      render_changed = true;
    }

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_LSHIFT) {
      auto &curr_cam = curr_scene->_camera[curr_scene->_active_camera];
      curr_cam.move_upward(MOVE_SPEED * dt);
      render_changed = true;
    }

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_LCTRL) {
      auto &curr_cam = curr_scene->_camera[curr_scene->_active_camera];
      curr_cam.move_downward(MOVE_SPEED * dt);
      render_changed = true;
    }
  });

  VulkanContext::run([&](VulkanContext &ctx) {
    // runned once
    if (runned == 0) {
    }

    // float runtime_second = float(runtime.count()) / 1e6;
    TimePoint begin = steady_clock::now();

    if (runned == 0) {
      LOG(1, "Init engine...");
      test(ctx);
      if (GPU_RENDER)
        renderer =
            std::make_unique<GPURenderer>(ctx, ctx.get_window_size().width,
                                          ctx.get_window_size().height, RGBA);
      else
        renderer = std::make_unique<SimpleCPURenderer>(
            ctx.get_window_size().width, ctx.get_window_size().height);
      LOG(1, "Engine init !");
    }

    if (render_changed) {
      LOG(6, "Running ray tracer...");
      renderer->render(*curr_scene);
      auto &img_buff = renderer->get_img_buff();
      Image result = img_buff.write_to_gpu(ctx);

      // display on screen the result :
      ctx.draw(result);

      if (runned == 0)
        img_buff.write_on_disk("test.png", ImageFormat::PNG);

      render_changed = false;
      LOG(6, "Render done !");
    }

    runned++;

    TimePoint end = steady_clock::now();

    Microsecond time_spent = duration_cast<Microsecond>(end - begin);
    dt = std::min((1.f / time_spent.count()) * 1e6f, 1.f / 60.f);

    constexpr Microsecond TARGET_DT = Microsecond(1'000'000 / 60);

    if (time_spent < TARGET_DT)
      sleep_for(Microsecond(TARGET_DT - time_spent));
    runtime += duration_cast<Microsecond>(steady_clock::now() - begin);
  });

  renderer.reset(nullptr);

  VulkanContext::cleanup();

  LOG(1, "Stopped !");

  return 0;
}
