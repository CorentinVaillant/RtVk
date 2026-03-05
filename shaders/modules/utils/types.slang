implementing utils;

public typealias Mat2 = matrix<float, 2, 2>;
public typealias Mat3 = matrix<float, 3, 3>;
public typealias Mat4 = matrix<float, 4, 4>;
public typealias Vec2 = vector<float, 2>;
public typealias Vec3 = vector<float, 3>;
public typealias Vec4 = vector<float, 4>;
public typealias Reel = float;

// -- Complexe -- //

public struct Complexe
{
    public Vec2 vec;

    public __init(Vec2 v)
    {
        vec = v;
    }

    public __init(float r)
    {
        vec = Vec2(r, 0);
    }

    public __init()
    {
        vec = Vec2(0);
    }

    public Reel mod()
    {
        return length(vec);
    }

    public Reel mod_sq()
    {
        return vec.x * vec.x + vec.y * vec.y;
    }
};

public void operator *=(inout Complexe c1, Complexe c2)
{
    float ax = c1.vec.x;
    float ay = c1.vec.y;
    float bx = c2.vec.x;
    float by = c2.vec.y;

    c1.vec = Vec2(
        ax * bx - ay * by,
        ax * by + ay * bx);
}

public Complexe operator +(Complexe c1, Complexe c2)
{
    return { c1.vec + c2.vec };
}

public Complexe operator *(Complexe c1, Complexe c2)
{
    return { Vec2(c1.vec.x * c2.vec.x - c1.vec.y * c2.vec.y, c1.vec.x * c2.vec.y + c1.vec.y * c2.vec.x) };
}

public Complexe pow_u(Complexe c, uint p)
{

    Complexe acc = { Vec2(1, 0) };

    while (p > 0)
    {
        if (p % 2 == 1)
            acc = acc * c;
        c *= c;
        p >>= 1;
    }

    return acc;
}

public bool contains_open(float min, float max, float t)
{
    return min < t && t < max;
}
