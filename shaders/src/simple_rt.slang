// simple_rt.slang

import "../modules/bindings";
import "../modules/utils";
import "../modules/rt_utils";
import "../modules/scene_data";
import "../modules/payloads";
import "../modules/materials";
import "../modules/lights";
import "../modules/geometry";

[shader("raygeneration")]
void rayGen()
{
    uint2 imageSize;
    resultTexture.GetDimensions(imageSize.x, imageSize.y);
    uint2 threadIdx = DispatchRaysIndex().xy;
    uint seed = tea(threadIdx.x + imageSize.y, threadIdx.y + imageSize.x + uint(pushed.time * 465851.5));

    if (threadIdx.x >= imageSize.x || threadIdx.y >= imageSize.y)
        return;

    Vec4 result_color = Vec4(0);
    RayDesc ray;
    RayPayload payload;
    for (uint s = 0; s < pushed.spp; s++)
    {
        ray = uni.get_ray(seed, pushed.camera.defocusAngle, threadIdx.x, threadIdx.y);

        payload.color = Vec4(0);
        payload.seed = seed;
        payload.depth = pushed.max_depth;

        TraceRay(scene, RAY_FLAG_NONE, ~0, 0, 0, 0, ray, payload);

        result_color += clamp(payload.color, 0, 1);
        seed = payload.seed;
    }
    result_color /= float(pushed.spp);

    resultTexture[threadIdx.xy] = payload.depth == pushed.max_depth && hit_sphere(ray, Vec3(0), 0.01f) ? Color(1) : result_color;
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = Vec4(0, 0, 0, 1);
    return;
    // uint num_of_light = punctualLightBuffer.getCount();
    // for (uint i = 0; i < num_of_light; i++)
    // {
    //     if (punctualLightBuffer[i].type == LightType::Sun)
    //     {
    //         LightSun sun = punctualLightBuffer[i].get_sun_light();
    //         if (dot(normalize(sun.forward_vec), normalize(WorldRayDirection())) < sun.angle)
    //         {
    //             payload.color += sun.emission;
    //             break;
    //         }
    //     }
    // }
}
