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

  VulkanContext context;
  context.init();

  std::vector<Sphere> objects = {Sphere{glm::vec3(-5.05, 0, 0), 5.},
                                 Sphere{glm::vec3(5.05, 0, 0), 5.}};

  Scene scene{Camera(), std::make_unique<HittableVector>(std::move(objects))};

  SimpleRenderer renderer(1000, 1000);

  renderer.render(scene);
  auto &img_buff = renderer.get_img_buff();
  img_buff.write_on_disk("test.png", ImageFormat::PNG);

  fmt::println("Done !");

  context.clean();
  return 0;
}