implementing utils;

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

// -- Funcs

public Vec4 sky_func(Vec3 direction)
{
    Vec3 unit_dir = normalize(direction);
    float a = (unit_dir.y + 1);
    return (1.f - a) * Vec4(1) + a * Vec4(1., 0.7, 0.5, 1);
    // return (1.f - a) * Vec4(1) + a * Vec4(0.5, 0.7, 1, 1);
}
