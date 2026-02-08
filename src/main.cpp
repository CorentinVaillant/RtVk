#include "Camera.h"
#include "ImageBuffer.h"
#include "glm/ext/vector_float3.hpp"
#include "hittables/Hittable.h"
#include "hittables/sphere.h"

#include "Renderer.h"
#include <memory>
#include <vector>

int main() {
  fmt::println("Starting app !");

  std::vector<Sphere> objects = {Sphere{glm::vec3(0), 5.}};

  Scene scene{Camera(), std::make_unique<HittableVector>(std::move(objects))};

  SimpleRenderer renderer(1000, 1000);

  renderer.render(scene);
  auto &img_buff = renderer.get_img_buff();
  img_buff.write_on_disk("test.png", ImageFormat::PNG);

  fmt::println("Done !");

  return 0;
}