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

    public property position : Vec3 {
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
