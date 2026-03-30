// simple_rt.slang

import "../modules/utils";
import "../modules/rt_utils";

public struct PushedCst
{
    public Camera camera;
    public Vec4 lightDir;
    public float screenWidth, screenHeight, time;
    public uint max_depth;
    public uint spp;
};

public struct RendererUniforms
{
    public CameraRenderInfo camRenderInfo;

    public RayDesc get_ray(uint seed, float defocusAngle, uint i, uint j)
    {
        float i_f = (float)i;
        float j_f = (float)j;

        Vec4 px_sample = camRenderInfo.px00_loc +
                         i_f * camRenderInfo.dt_u +
                         j_f * camRenderInfo.dt_v;

        Vec3 ray_origine = defocusAngle <= 0
                               ? camRenderInfo.center.xyz
                               : camRenderInfo.defocus_disk_sample(seed, camRenderInfo.center).xyz;

        Vec3 ray_dir = px_sample.xyz - ray_origine;

        RayDesc result;
        result.Direction = ray_dir;
        result.Origin = ray_origine;
        result.TMin = 0.00001f;
        result.TMax = 10000.0f;
        return result;
    }
};

[[vk::push_constant]]
ConstantBuffer<PushedCst> pushed;

[[vk::binding(0, 0)]]
RWTexture2D resultTexture;
[[vk::binding(1, 0)]]
RaytracingAccelerationStructure scene;

[[vk::binding(2, 0)]]
ConstantBuffer<RendererUniforms> uni;

[[vk::binding(3, 0)]]
StructuredBuffer<Brdf> material;

[[vk::binding(4, 0)]]
StructuredBuffer<RendererInstance> instances;

[[vk::binding(5, 0)]]
StructuredBuffer<Vertex> vertexBuffer;

[[vk::binding(6, 0)]]
StructuredBuffer<uint3> indexBuffer;

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

        result_color += payload.color;
        seed = payload.seed;
    }
    result_color /= float(pushed.spp);

    resultTexture[threadIdx.xy] = payload.depth == pushed.max_depth && hit_sphere(ray, Vec3(0), 0.01f) ? Color(1) : result_color;
}

static constexpr float ATTENUATION = 0.5f;
static constexpr float ROUGHNESS = 0.3f;

[shader("closesthit")]
void closestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr_in)
{
    uint tri_index = PrimitiveIndex();
    RendererInstance instance = instances[InstanceID()];
    uint3 idx = indexBuffer[tri_index + instance.indexOffset];
    if (random(payload.seed, 0, 1) < 0.01f)
        printf("index offset : %u\n", instance.indexOffset);

    Vertex v0 = vertexBuffer[idx.x];
    Vertex v1 = vertexBuffer[idx.y];
    Vertex v2 = vertexBuffer[idx.z];

    Vec3 b = Vec3(1.0 - attr_in.barycentrics.x - attr_in.barycentrics.y,
                  attr_in.barycentrics.x,
                  attr_in.barycentrics.y);

    // Simple interpolation
    HitAttribut attr;
    attr.time = RayTCurrent();
    attr.normal = normalize(b.x * v0.normal + b.y * v1.normal + b.z * v2.normal);
    attr.u = b.x * v0.u + b.y * v1.u + b.z * v2.u;
    attr.v = b.x * v0.v + b.y * v1.v + b.z * v2.v;
    attr.front_face = dot(WorldRayDirection(), attr.normal) < 0;
    if (!attr.front_face)
        attr.normal *= -1;
    attr.material_index = instance.materialId;

    RayDesc ray;
    ray.Direction = WorldRayDirection();
    ray.Origin = WorldRayOrigin();
    ray.TMax = 10000;
    ray.TMin = RayTMin();

    shade(ray, payload, attr);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = Color(0.3);
}

void shade(in RayDesc ray_in, inout RayPayload payload, HitAttribut attr)
{
    payload.depth--;
    if (payload.depth <= 0)
        return miss(payload);

    if (attr.material_index >= uint(material.getCount()))
    {
        return;
    }

    let brdf = material[attr.material_index];
    RayDesc scatter_ray;
    scatter_ray.Origin = attr.position;
    scatter_ray.TMin = 0.0001f;
    scatter_ray.TMax = 10000.f;

    ScatterAttribut scattr;
    if (!brdf.scatter(ray_in, attr, payload.seed, scattr))
    {
        payload.color = brdf.emited(ray_in, attr, attr.uv);
        return;
    }

    scatter_ray.Direction = scattr.out_dir;

    TraceRay(scene, RAY_FLAG_NONE, ~0, 0, 0, 0, scatter_ray, payload);
    Color emitted = brdf.emited(ray_in, attr, attr.uv);

    if (!scattr.skip_pdf && scattr.pdf_val > 1e-8f)
        payload.color = payload.color * scattr.attenuation / scattr.pdf_val + emitted;
    else
        payload.color = payload.color * scattr.attenuation + emitted;

    return;
}

void shadeDebug(in RayDesc ray_in, inout RayPayload payload, HitAttribut attr)
{
    float t = float(InstanceID()) * M_PI_F / 8.f;
    payload.color = Color(0.5 + 0.5 * cos(6.2831 * (Vec3(0.0, 0.33, 0.67) + t)), 1.f);

    return;
}
