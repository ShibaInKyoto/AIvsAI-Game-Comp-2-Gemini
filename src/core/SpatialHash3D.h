#pragma once

#include "core/Math3D.h"
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace aethernaut {

struct SpatialEntry {
    int entityId = -1;
    int entityCategory = 0; // 0: Airship, 1: LeviathanSegment, 2: Island, 3: Salvage
    Vec3 position;
    float radius = 1.0f;
};

class SpatialHash3D {
public:
    explicit SpatialHash3D(float cellSize = 40.0f) : cellSize_(cellSize), invCellSize_(1.0f / cellSize) {}

    void clear();
    void insert(const SpatialEntry& entry);
    void querySphere(const Vec3& center, float radius, std::vector<SpatialEntry>& outResults) const;

private:
    int64_t hashCell(int cx, int cy, int cz) const {
        const uint64_t p1 = 73856093ULL;
        const uint64_t p2 = 19349663ULL;
        const uint64_t p3 = 83492791ULL;
        return static_cast<int64_t>(
            (static_cast<uint64_t>(cx) * p1) ^
            (static_cast<uint64_t>(cy) * p2) ^
            (static_cast<uint64_t>(cz) * p3)
        );
    }

    float cellSize_;
    float invCellSize_;
    std::unordered_map<int64_t, std::vector<SpatialEntry>> buckets_;
};

} // namespace aethernaut
