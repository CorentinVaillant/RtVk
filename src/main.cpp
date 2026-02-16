#include <ranges>

#include "Camera.h"
#include "ImageBuffer.h"
#include "fmt/core.h"
#include "glm/ext/vector_float3.hpp"
#include "graphics/vulkan_context.h"
#include "hittables/Hittable.h"
#include "hittables/Sphere.h"

#include <cassert>

#include "Renderer.h"
#include "types.h"

int main() {
  fmt::println("Starting app !");

  VulkanContext::init("RtVk");

  VulkanContext::run([](VulkanContext &ctx) {
    LOG(1, "Running ray tracer...");
    std::vector<Sphere> objects = {Sphere{glm::vec3(-5.05, 0, 0), 5.},
                                   Sphere{glm::vec3(5.05, 0, 0), 5.}};

    Scene scene{Camera(), std::make_unique<HittableVector>(std::move(objects))};

    SimpleRenderer renderer(500,500);

    renderer.render(scene);
    auto &img_buff = renderer.get_img_buff();
    img_buff.write_on_disk("test.png", ImageFormat::PNG);
    img_buff.write_to_gpu(ctx);

    LOG(1, "Running ray done !\n Stop");
    VulkanContext::stop();
  });

  VulkanContext::cleanup();

  fmt::println("Stopped !");

  return 0;
}