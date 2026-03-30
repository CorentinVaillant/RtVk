implementing rt_utils;

import "../utils";

public Vec4 sky_func(Vec3 direction)
{
    Vec3 unit_dir = normalize(direction);
    float a = (unit_dir.y + 1);
    return (1.f - a) * Vec4(1) + a * Vec4(1., 0.7, 0.5, 1);
    // return (1.f - a) * Vec4(1) + a * Vec4(0.5, 0.7, 1, 1);
}
