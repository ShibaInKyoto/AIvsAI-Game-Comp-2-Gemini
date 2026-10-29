#include "core/Math3D.h"

namespace aethernaut {

Quat Quat::fromAxisAngle(const Vec3& axis, float radians) {
    Vec3 n = axis.normalized();
    float half = radians * 0.5f;
    float s = std::sin(half);
    return {n.x * s, n.y * s, n.z * s, std::cos(half)};
}

Quat Quat::fromEuler(float pitch, float yaw, float roll) {
    Quat qYaw = fromAxisAngle({0.0f, 1.0f, 0.0f}, yaw);
    Quat qPitch = fromAxisAngle({1.0f, 0.0f, 0.0f}, pitch);
    Quat qRoll = fromAxisAngle({0.0f, 0.0f, 1.0f}, roll);
    return (qYaw * qPitch * qRoll).normalized();
}

Quat Quat::operator*(const Quat& b) const {
    return {
        w * b.x + x * b.w + y * b.z - z * b.y,
        w * b.y - x * b.z + y * b.w + z * b.x,
        w * b.z + x * b.y - y * b.x + z * b.w,
        w * b.w - x * b.x - y * b.y - z * b.z
    };
}

Quat Quat::normalized() const {
    float len = std::sqrt(x * x + y * y + z * z + w * w);
    if (len < 1e-6f) {
        return identity();
    }
    float inv = 1.0f / len;
    return {x * inv, y * inv, z * inv, w * inv};
}

Vec3 Quat::rotate(const Vec3& v) const {
    Vec3 u{x, y, z};
    Vec3 uv = Vec3::cross(u, v);
    Vec3 uuv = Vec3::cross(u, uv);
    return v + ((uv * w) + uuv) * 2.0f;
}

bool intersectRayAABB(const Ray3D& ray, const AABB3D& box, float& tHit, Vec3& outNormal) {
    float tMin = 0.0f;
    float tMax = 1e30f;
    Vec3 normal{0.0f, 0.0f, 0.0f};

    const float origin[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
    const float dir[3] = {ray.dir.x, ray.dir.y, ray.dir.z};
    const float bMin[3] = {box.min.x, box.min.y, box.min.z};
    const float bMax[3] = {box.max.x, box.max.y, box.max.z};

    for (int i = 0; i < 3; ++i) {
        if (std::abs(dir[i]) < 1e-6f) {
            if (origin[i] < bMin[i] || origin[i] > bMax[i]) {
                return false;
            }
        } else {
            float invD = 1.0f / dir[i];
            float t1 = (bMin[i] - origin[i]) * invD;
            float t2 = (bMax[i] - origin[i]) * invD;
            float sign = -1.0f;
            if (t1 > t2) {
                std::swap(t1, t2);
                sign = 1.0f;
            }
            if (t1 > tMin) {
                tMin = t1;
                normal = {0.0f, 0.0f, 0.0f};
                if (i == 0) normal.x = sign;
                if (i == 1) normal.y = sign;
                if (i == 2) normal.z = sign;
            }
            tMax = std::min(tMax, t2);
            if (tMin > tMax) {
                return false;
            }
        }
    }

    tHit = tMin;
    outNormal = normal;
    return true;
}

bool intersectRaySphere(const Ray3D& ray, const Vec3& center, float radius, float& tHit) {
    Vec3 oc = ray.origin - center;
    float b = Vec3::dot(oc, ray.dir);
    float c = Vec3::dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0f) {
        return false;
    }
    float s = std::sqrt(disc);
    float t = -b - s;
    if (t < 0.0f) {
        t = -b + s;
        if (t < 0.0f) {
            return false;
        }
    }
    tHit = t;
    return true;
}

} // namespace aethernaut
