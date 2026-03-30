module rt_utils;

__include "rt_utils/backgrounds";
__include "rt_utils/camera";
__include "rt_utils/pdf";
__include "rt_utils/materials";

public struct RayPayload
{
    public Vec4 color;
    public uint seed;
    public uint depth;
};

public struct RendererInstance {
    public uint32_t materialId;
    public uint32_t indexOffset;

    uint32_t pad1, pad2;
};

public struct Vertex
{
    public float3 position;
    public float u;
    public float3 normal;
    public float v;

    public property uv : Vec2
    {
        get {
            return Vec2(u, v);
        }
        set(Vec2 uv) {
            u = uv.x;
            v = uv.y;
        }
    }
};

public struct HitAttribut
{
    public static constexpr uint NO_MATERIAL_INDEX = ~0u;

    public float time;
    public Vec3 normal;
    public float u;
    public float v;
    public bool front_face;
    public uint material_index = 0;

    public property uv : Vec2
    {
        get {
            return Vec2(u, v);
        }
        set(Vec2 uv) {
            u = uv.x;
            v = uv.y;
        }
    }

    public property position : Vec3
    {
        get {
            return WorldRayOrigin() + time * WorldRayDirection();
        }
    }

    [mutating]
    public void set_face_normal(Vec3 ray_dir, Vec3 out_normal)
    {
        front_face = dot(ray_dir, out_normal) < 0;
        normal = front_face ? out_normal : -out_normal;
    }
};

public struct ScatterAttribut
{
    public Color attenuation = Color(0);
    public float pdf_val;
    public bool skip_pdf;
    public Vec3 out_dir;
};

public bool hit_sphere(RayDesc ray, Vec3 pos, float radius)
{
    // fancy quadratic formula
    Vec3 oc = pos - ray.Origin;
    float a = length_sq(ray.Direction);
    float h = dot(ray.Direction, oc);
    float c = length_sq(oc) - radius * radius;

    float discr = h * h - a * c;
    if (discr < 0.f)
        return false; // No hit

    float delta = sqrt(discr);

    // Nearest root in range
    float root = (h - delta) / a;
    if (!contains_open(ray.TMin, ray.TMax, root))
    {
        root = (h + delta) / a; // testing the conjugate
        if (!contains_open(ray.TMin, ray.TMax, root))
            return false; // roots out of range
    }
    // HIT !
    return true;
}
