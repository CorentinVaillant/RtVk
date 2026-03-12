implementing utils;

public struct Camera
{
    public float aspectRatio = 1.0;
    public float vFov = M_PI_F / 2;

    public float defocusAngle = 0;
    public float focusDist = 10;

    public Vec4 lookFrom = Vec4(0, 0, -10,1);
    public Vec4 lookAt = Vec4(0, 0, 0,1);
    public Vec4 vUp = Vec4(0, 1, 0,1);

    public void debug_display() {
        printf("-- aspectRatio = %f", aspectRatio);
        printf("-- vFov = %f", vFov);
        printf("-- defocusAngle = %f", defocusAngle);
        printf("-- focusDist = %f", focusDist);
        printf("-- lookFrom = Vec4(%f, %f, %f, %f)", lookFrom.x, lookFrom.y, lookFrom.z, lookFrom.w);
        printf("-- lookAt = Vec4(%f, %f, %f, %f)", lookAt.x, lookAt.y, lookAt.z, lookAt.w);
        printf("-- vUp = Vec4(%f, %f, %f, %f)", vUp.x, vUp.y, vUp.z, vUp.w);
    }
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

    public Vec4 defocus_disk_sample(Vec2 seed, Vec4 center)
    {
        Vec2 p = random_in_unit_disk(seed);
        return center + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
    }
};