// simple_rt.slang

import "../modules/utils";

public struct PushedCst
{
    public Camera camera;
    public Vec4 lightDir;
    public float screenWidth, screenHeight, time;
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

struct HitAttribut
{
    Vec3 position;
    float time;
    Vec3 normal;
    bool front_face;

    [mutating]
    void set_face_normal(Vec3 ray_dir, Vec3 out_normal)
    {
        front_face = dot(ray_dir, out_normal) < 0;
        normal = front_face ? out_normal : -out_normal;
    }
};

struct RayPayload
{
    Vec4 color;
    uint seed;
    uint depth = 10;
};

[[vk::binding(0, 0)]]
uniform RWTexture2D resultTexture;
[[vk::binding(1, 0)]]
uniform RaytracingAccelerationStructure scene;

[[vk::binding(2, 0)]]
uniform ConstantBuffer<RendererUniforms> uni;

[[vk::push_constant]]
ConstantBuffer<PushedCst> pushed;

[shader("raygeneration")]
void rayGen()
{
    uint2 threadIdx = DispatchRaysIndex().xy;
    uint2 imageSize;
    resultTexture.GetDimensions(imageSize.x, imageSize.y);
    uint seed = tea(threadIdx.x + imageSize.y, threadIdx.y + imageSize.x + uint(pushed.time * 465851.5));

    if (threadIdx.x >= imageSize.x || threadIdx.y >= imageSize.y)
        return;

    Vec4 result_color = Vec4(0);
    for (uint s = 0; s < pushed.spp; s++) {

        RayDesc ray = uni.get_ray(seed, pushed.camera.defocusAngle, threadIdx.x, threadIdx.y);
        
        RayPayload payload = {};
        payload.seed = seed;
        TraceRay(scene, RAY_FLAG_NONE, ~0, 0, 0, 0, ray, payload);

        result_color += payload.color;
    }
    resultTexture[threadIdx.xy] = result_color / float(pushed.spp);
}

static constexpr float ATTENUATION = 0.9;
static constexpr float ROUGHNESS = 0.3f;

[shader("closesthit")]
void closestHit(inout RayPayload payload, in HitAttribut attr)
{
    payload.depth--;
    if (payload.depth == 0)
        return miss(payload);

    let jitter = ONB(attr.normal).transform(random_cosine_hemisphere(payload.seed));

    RayDesc new_ray;
    new_ray.Direction = (1.f - ROUGHNESS) * reflect(WorldRayDirection(), attr.normal) + ROUGHNESS * jitter;
    new_ray.Origin = attr.position;
    new_ray.TMin = 0.001f;
    new_ray.TMax = 1000.f;
    TraceRay(scene, RAY_FLAG_NONE, ~0, 0, 0, 0, new_ray, payload);
    payload.color = Vec4(payload.color.xyz * 0.9, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = sky_func(WorldRayDirection());
}

// -- Intersection part --

struct Sphere
{
    Vec4 position;
    float radius;
};

[[vk::binding(3, 0)]]
uniform StructuredBuffer<Sphere> primitiveBuffer;

// -- Sphere

[shader("intersection")]
void sphereIntersection()
{
    HitAttribut hit_record;

    uint sphere_index = InstanceID() + GeometryIndex();
    Sphere sphere = primitiveBuffer[sphere_index];
    Vec4 deb_pos = sphere.position;

    float t_min = RayTMin();
    float t_max = RayTCurrent();

    Vec3 ray_origin = WorldRayOrigin();
    Vec3 ray_direction = WorldRayDirection();

    // fancy quadratic formula
    Vec3 oc = sphere.position.xyz - ray_origin;
    float a = dot(ray_direction, ray_direction);
    float h = dot(ray_direction, oc);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;

    float discr = h * h - a * c;
    if (discr < 0.f)
        return; // No hit

    float delta = sqrt(discr);

    // Nearest root in range
    float root = (h - delta) / a;
    if (!contains_open(t_min, t_max, root))
    {
        root = (h + delta) / a; // testing the conjugate
        if (!contains_open(t_min, t_max, root))
            return; // roots out of range
    }

    // HIT !
    // HitAttribut hit_record;
    hit_record.time = root;
    hit_record.position = ray_origin + root * ray_direction;
    Vec3 out_normal = (hit_record.position - sphere.position.xyz) / sphere.radius;
    hit_record.set_face_normal(ray_direction, out_normal);

    ReportHit(root, 0, hit_record);
}
