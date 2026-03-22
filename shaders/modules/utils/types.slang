implementing utils;

// -- Constants -- //

///@return $ \pi $
public static constexpr float M_PI_F = 3.14159265358979323846;
///@return $ 2\pi $
public static constexpr float M_TWO_PI_F = 2 * M_PI_F;
///@return $ 3\pi $
public static constexpr float M_THREE_PI_F = 3 * M_PI_F;
///@return $ 4\pi $
public static constexpr float M_FOUR_PI_F = 4 * M_PI_F;
///@return $ \fract{1}{\pi} $
public static constexpr float M_1_PI_F = 1 / M_PI_F;
///@return $ \fract{2}{\pi} $
public static constexpr float M_2_PI_F = 2 / M_PI_F;
///@return $ \fract{3}{\pi} $
public static constexpr float M_3_PI_F = 3 / M_PI_F;
///@return $ \fract{4}{\pi} $
public static constexpr float M_4_PI_F = 4 / M_PI_F;

// -- Types --

public typealias Mat2 = matrix<float, 2, 2>;
public typealias Mat3 = matrix<float, 3, 3>;
public typealias Mat4 = matrix<float, 4, 4>;
public typealias Vec2 = vector<float, 2>;
public typealias Vec3 = vector<float, 3>;
public typealias Vec4 = vector<float, 4>;
public typealias Reel = float;

public typealias Color = vector<float, 4>;
// -- Complexe

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

// -- ONB (OrthoNormal Base)
public struct ONB
{
    public readonly Vec3 u, v, w;

    public property uvw : Mat3
    {
        get {
            return Mat3(u, v, w);
        }
    }

    public __init(Vec3 normal)
    {
        w = normalize(normal);
        Vec3 a = (abs(w.x) > 0.9) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        v = normalize(cross(w, a));
        u = cross(w, v);
    }

    public Vec3 transform(Vec3 vector)
    {
        return mul(vector, uvw);
    }
};

// -- Common functions -- //

public bool contains_open(float min, float max, float t)
{
    return min < t && t < max;
}

public float lenght_sq(Vec3 v) {
    return dot(v, v);
}

public float sq(float x)
{
    return x * x;
}

public float cos2theta(const in Vec3 w)
{
    return w.z * w.z;
}
public float sin2theta(const in Vec3 w)
{
    return max(0.f, 1.f - cos2theta(w));
}
public float tan2theta(const in Vec3 w)
{
    return sin2theta(w) / cos2theta(w);
}
public float cos_phi(Vec3 w)
{
    let sin_theta = sqrt(sin2theta(w));
    return sin_theta == 0
               ? 1
               : clamp(w.x / sin_theta, -1, 1);
}

public float sin_phi(Vec3 w)
{
    let sin_theta = sqrt(sin2theta(w));
    return sin_theta == 0
               ? 0
               : clamp(w.y / sin_theta, -1, 1);
}
