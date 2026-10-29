#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace aethernaut {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kDegToRad = kPi / 180.0f;
constexpr float kRadToDeg = 180.0f / kPi;

struct Int3 {
    int x = 0;
    int y = 0;
    int z = 0;

    constexpr bool operator==(const Int3& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
    constexpr bool operator!=(const Int3& o) const {
        return !(*this == o);
    }
    constexpr Int3 operator+(const Int3& o) const {
        return {x + o.x, y + o.y, z + o.z};
    }
    constexpr Int3 operator-(const Int3& o) const {
        return {x - o.x, y - o.y, z - o.z};
    }
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    constexpr Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(float s) const {
        float inv = (s != 0.0f) ? (1.0f / s) : 0.0f;
        return {x * inv, y * inv, z * inv};
    }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec3 normalized() const {
        float len = length();
        if (len < 1e-6f) {
            return {0.0f, 0.0f, 0.0f};
        }
        return *this / len;
    }

    static constexpr float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static constexpr Vec3 cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
    }
};

struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    static Quat identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static Quat fromAxisAngle(const Vec3& axis, float radians);
    static Quat fromEuler(float pitch, float yaw, float roll);

    Quat operator*(const Quat& b) const;
    Quat normalized() const;
    Quat conjugate() const { return {-x, -y, -z, w}; }

    Vec3 rotate(const Vec3& v) const;
    Vec3 forward() const { return rotate({0.0f, 0.0f, -1.0f}); }
    Vec3 up() const { return rotate({0.0f, 1.0f, 0.0f}); }
    Vec3 right() const { return rotate({1.0f, 0.0f, 0.0f}); }
};

struct Ray3D {
    Vec3 origin;
    Vec3 dir; // Normalized
};

struct AABB3D {
    Vec3 min;
    Vec3 max;

    bool contains(const Vec3& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }
};

// Ray vs AABB intersection; returns true on hit and outputs hit distance tMin and normal
bool intersectRayAABB(const Ray3D& ray, const AABB3D& box, float& tHit, Vec3& outNormal);

// Ray vs Sphere intersection
bool intersectRaySphere(const Ray3D& ray, const Vec3& center, float radius, float& tHit);

// Deterministic PCG32 Random Number Generator
class Rng32 {
public:
    explicit Rng32(uint64_t seed = 0x853c49e6748fea9bULL) {
        seedState(seed, 0xda3e39cb94b95bdbULL);
    }

    void seedState(uint64_t initstate, uint64_t initseq) {
        state_ = 0U;
        inc_ = (initseq << 1u) | 1u;
        nextU32();
        state_ += initstate;
        nextU32();
    }

    uint32_t nextU32() {
        uint64_t oldstate = state_;
        state_ = oldstate * 6364136223846793005ULL + inc_;
        uint32_t xorshifted = static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
        uint32_t rot = static_cast<uint32_t>(oldstate >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

    float nextFloat() {
        return static_cast<float>(nextU32() >> 8) * (1.0f / 16777216.0f);
    }

    float range(float minVal, float maxVal) {
        return minVal + (maxVal - minVal) * nextFloat();
    }

    int rangeInt(int minVal, int maxVal) {
        if (maxVal <= minVal) {
            return minVal;
        }
        return minVal + static_cast<int>(nextU32() % static_cast<uint32_t>(maxVal - minVal + 1));
    }

private:
    uint64_t state_ = 0x853c49e6748fea9bULL;
    uint64_t inc_ = 0xda3e39cb94b95bdbULL;
};

} // namespace aethernaut
