module payload;

import utils;


public struct RayPayload
{
    public Vec4 color;
    public uint seed;
    public uint depth;
};

public struct PdfPayload {
    public bool hit = false;
    public float min_light_val;

    public float pdf_val = 0;
    public Vec3 hit_point;
};