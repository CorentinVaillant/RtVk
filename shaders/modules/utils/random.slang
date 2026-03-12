implementing utils;

__include "types";

public float hash(Vec2 seed)
{
    return fract(sin(7.289 * seed.x + 11.23 * seed.y) * 23758.5453);
}

public Vec2 random2(Vec2 seed)
{
    return Vec2(hash(seed + 0.1), hash(seed + 0.7));
}

public float random(Vec2 seed, float min, float max)
{
    return min + (max - min) * hash(seed + 0.5);
}

public Vec2 random_in_unit_disk(Vec2 seed)
{
    float theta = random(seed, 0, M_2_PI_F);
    float ro = random(seed + 0.2, 0, 1);
    return ro * Vec2(cos(theta), sin(theta));
}
