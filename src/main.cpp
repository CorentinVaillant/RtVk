#include "Camera.h"
#include "ImageBuffer.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_scancode.h"
#include "glm/ext/vector_float3.hpp"
#include "graphics/vulkan_context.h"
#include "hittables/Hittable.h"
#include "hittables/Sphere.h"

#include <cassert>

#include "graphics/vulkan_context.h"
#include "Renderer.h"
#include "test.h"
#include "types.h"

int main() {
  LOG(1, "Starting app !");

  VulkanContext::init("RtVk");

  VulkanContext::set_event_callbacks([](VulkanContext &ctx, SDL_Event &event) {
    if(event.type == SDL_EVENT_QUIT)
      VulkanContext::stop();

    if(event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
      VulkanContext::stop();
  });

  VulkanContext::run([](VulkanContext &ctx) {
    static bool runned_once = false;
    if (runned_once)
      return;
    // test(ctx);

    LOG(1, "Running ray tracer...");
    std::vector<Sphere> objects = {Sphere{glm::vec3(-5.05, 0, 0), 5.},
                                   Sphere{glm::vec3(5.05, 0, 0), 5.}};

    Scene scene{Camera(), std::make_unique<HittableVector>(std::move(objects))};

    SimpleRenderer renderer(500, 500);

    renderer.render(scene);
    LOG(1, "Running ray done !");
    LOG(1, "Drawing the image...");
    auto &img_buff = renderer.get_img_buff();
    img_buff.write_on_disk("test.png", ImageFormat::PNG);
    Image result = img_buff.write_to_gpu(ctx);

    // display on screen the result :
    ctx.draw(result);

    runned_once = true;
    LOG(1, "Done !");
  });

  VulkanContext::cleanup();

  LOG(1, "Stopped !");

  return 0;
}
