#include "Camera.h"
#include "ImageBuffer.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_scancode.h"

#include "types.h"

#include "graphics/vulkan_context.h"

#include "renderer/CPURenderer.h"
#include "renderer/GPURenderer.h"

#include "hittables/Hittable.h"
#include "hittables/Sphere.h"

#include "test.h"

constexpr bool GPU_RENDER = true;

int main() {
  LOG(1, "Starting app !");

  VulkanContext::init("RtVk");

  VulkanContext::set_event_callbacks([](VulkanContext &ctx, SDL_Event &event) {
    if (event.type == SDL_EVENT_QUIT)
      VulkanContext::stop();

    if (event.type == SDL_EVENT_KEY_DOWN &&
        event.key.scancode == SDL_SCANCODE_ESCAPE)
      VulkanContext::stop();
  });

  std::unique_ptr<Renderer> renderer = nullptr;

  VulkanContext::run([&renderer](VulkanContext &ctx) {
    static size_t runned = 0;
    static float dt = 1.f / 60.f;
    static Microsecond runtime;

    float runtime_second = float(runtime.count()) / 1e6;
    TimePoint begin = steady_clock::now();

    float pos_x = 100.f * std::cos(runtime_second) * dt;
    float pos_y = 100.f * std::sin(runtime_second) * dt;

    LOG(5, "Running ray tracer...");
    std::vector<Sphere> objects = {
        Sphere{glm::vec3(0., 0., 106), 100},
        Sphere{glm::vec3(2 * pos_y, 2 * pos_x, 3), 2},
        Sphere{glm::vec3(0.1f * pos_x, 0.1f * pos_y, 3), 1},
    };

    Scene scene{._camera = Camera(),
                ._accStruct = std::make_unique<HittableVector<Sphere>>(
                    std::move(objects)),
                ._time = runtime_second, };

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

    renderer->render(scene);
    LOG(5, "Running ray done !");
    LOG(5, "Drawing the image...");
    auto &img_buff = renderer->get_img_buff();
    Image result = img_buff.write_to_gpu(ctx);

    // display on screen the result :
    ctx.draw(result);

    if (runned == 0)
      img_buff.write_on_disk("test.png", ImageFormat::PNG);
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
