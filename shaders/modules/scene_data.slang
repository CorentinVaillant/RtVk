module scene_data;

import "./utils";

// -- Uniforms data

public enum PrimitiveType {
    Light,
    Mesh,
}

public struct RendererInstance {
    public uint32_t materialId;
    public uint32_t indexOffset;
    public uint32_t _indexCount;

    uint32_t pad2;
};

public struct Vertex
{
    public Vec3 position;
    public float u;
    public Vec3 normal;
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


public struct Camera
{
    public float aspectRatio = 1.0;
    public float vFov = M_PI_F / 2;

    public float defocusAngle = 0;
    public float focusDist = 10;

    public Vec4 lookFrom = Vec4(0, 0, -10, 1);
    public Vec4 lookAt = Vec4(0, 0, 0, 1);
    public Vec4 vUp = Vec4(0, 1, 0, 1);
};

public struct CameraRenderInfo
{
    public Vec4 center;
    public Vec4 view_u, view_v;
    public Vec4 dt_u, dt_v;                     // differances between two pixel
    public Vec4 u, v, w;                        // Camera frame basis
    public Vec4 defocus_disk_u, defocus_disk_v; // Defocus disk
    public Vec4 viewport_uper_left;
    public Vec4 px00_loc;

    public Vec4 defocus_disk_sample(uint seed, Vec4 center)
    {
        Vec2 p = random_in_unit_disk(seed);
        return center + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
    }
};

public struct PushedCst
{
    public Camera camera;
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
