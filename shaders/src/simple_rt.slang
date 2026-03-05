// simple_rt.slang

import "../modules/utils";

struct Uniforms
{
    float screenWidth, screenHeight;
    float focalLength, frameHeight;
    Vec4 cameraDir;
    Vec4 cameraUp;
    Vec4 cameraRight;
    Vec4 cameraPosition;
    Vec4 lightDir;
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
};

uniform RWTexture2D resultTexture;
uniform RaytracingAccelerationStructure scene;
uniform Uniforms uniforms;

[shader("raygeneration")]
void rayGen()
{
    uint2 threadIdx = DispatchRaysIndex().xy;
    if (threadIdx.x >= (int)uniforms.screenWidth || threadIdx.y >= (int)uniforms.screenHeight)
        return;

    float frameWidth = uniforms.screenWidth / uniforms.screenHeight * uniforms.frameHeight;
    float imageY = (threadIdx.y / uniforms.screenHeight - 0.5f) * uniforms.frameHeight;
    float imageX = (threadIdx.x / uniforms.screenWidth - 0.5f) * frameWidth;
    float imageZ = uniforms.focalLength;
    Vec3 rayDir = normalize(uniforms.cameraDir.xyz * imageZ - uniforms.cameraUp.xyz * imageY - uniforms.cameraRight.xyz * imageX);

    // trace the ray
    RayDesc ray;
    ray.Origin = uniforms.cameraPosition.xyz;
    ray.Direction = rayDir;
    ray.TMin = 0.001;
    ray.TMax = 10000.0f;

    RayPayload payload = {};
    TraceRay(scene, RAY_FLAG_NONE, ~0, 0, 0, 0, ray, payload);

    resultTexture[threadIdx.xy] = payload.color;
}

[shader("closesthit")]
void closestHit(inout RayPayload payload, in HitAttribut attr)
{
    payload.color = Vec4(attr.normal / 2.f + 0.5, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = Vec4(normalize(WorldRayDirection()) / 2.f + 0.5f, 1);
}

// -- Intersection part --

// -- Sphere

struct Sphere
{
    Vec3 position;
    float radius;
};

uniform StructuredBuffer<Sphere> primitiveBuffer;

[shader("intersection")]
void sphereIntersection()
{
    uint sphere_index = InstanceID() + GeometryIndex();
    Sphere sphere = primitiveBuffer[sphere_index];

    float t_min = RayTMin();
    float t_max = RayTCurrent();

    Vec3 ray_origin = WorldRayOrigin();
    Vec3 ray_direction = WorldRayDirection();

    // fancy quadratic formula
    Vec3 oc = sphere.position - ray_origin;
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
    HitAttribut hit_record;
    hit_record.time = root;
    hit_record.position = ray_origin + root * ray_direction;
    Vec3 out_normal = (hit_record.position - sphere.position) / sphere.radius;
    hit_record.set_face_normal(ray_direction, out_normal);

    ReportHit(root, 0, hit_record);
}
