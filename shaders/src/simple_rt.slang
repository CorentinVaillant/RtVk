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
void main()
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

        TraceRay(scene, RAY_FLAG_NONE, ~0u, 0, 0, 0, ray, payload);

        result_color += clamp(payload.color, 0, 1);
        seed = payload.seed;
    }
    result_color /= float(pushed.spp);

    let nan_eval = isnan(result_color);
    if (nan_eval.x || nan_eval.y || nan_eval.z || nan_eval.w)
        resultTexture[threadIdx.xy] = Color(1., 0, 0, 1);
    else
        resultTexture[threadIdx.xy] = result_color;

}
