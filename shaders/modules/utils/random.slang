implementing utils;

__include "types";

public float hash(Vec2 uv) {
    return fract(sin(7.289 * uv.x + 11.23 * uv.y) * 23758.5453);
}

public Vec2 random2(Vec2 p) {
    return Vec2(hash(p + 0.1), hash(p + 0.7));
}

