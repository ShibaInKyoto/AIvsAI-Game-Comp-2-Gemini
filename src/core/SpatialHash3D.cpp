#include "core/SpatialHash3D.h"

namespace aethernaut {

void SpatialHash3D::clear() {
    for (auto& kv : buckets_) {
        kv.second.clear();
    }
}

void SpatialHash3D::insert(const SpatialEntry& entry) {
    int minX = static_cast<int>(std::floor((entry.position.x - entry.radius) * invCellSize_));
    int maxX = static_cast<int>(std::floor((entry.position.x + entry.radius) * invCellSize_));
    int minY = static_cast<int>(std::floor((entry.position.y - entry.radius) * invCellSize_));
    int maxY = static_cast<int>(std::floor((entry.position.y + entry.radius) * invCellSize_));
    int minZ = static_cast<int>(std::floor((entry.position.z - entry.radius) * invCellSize_));
    int maxZ = static_cast<int>(std::floor((entry.position.z + entry.radius) * invCellSize_));

    for (int z = minZ; z <= maxZ; ++z) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                buckets_[hashCell(x, y, z)].push_back(entry);
            }
        }
    }
}

void SpatialHash3D::querySphere(const Vec3& center, float radius, std::vector<SpatialEntry>& outResults) const {
    outResults.clear();
    int minX = static_cast<int>(std::floor((center.x - radius) * invCellSize_));
    int maxX = static_cast<int>(std::floor((center.x + radius) * invCellSize_));
    int minY = static_cast<int>(std::floor((center.y - radius) * invCellSize_));
    int maxY = static_cast<int>(std::floor((center.y + radius) * invCellSize_));
    int minZ = static_cast<int>(std::floor((center.z - radius) * invCellSize_));
    int maxZ = static_cast<int>(std::floor((center.z + radius) * invCellSize_));

    for (int z = minZ; z <= maxZ; ++z) {
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                auto it = buckets_.find(hashCell(x, y, z));
                if (it == buckets_.end()) {
                    continue;
                }
                for (const auto& item : it->second) {
                    float combined = radius + item.radius;
                    if ((item.position - center).lengthSq() <= combined * combined) {
                        bool duplicate = false;
                        for (const auto& existing : outResults) {
                            if (existing.entityId == item.entityId &&
                                existing.entityCategory == item.entityCategory) {
                                duplicate = true;
                                break;
                            }
                        }
                        if (!duplicate) {
                            outResults.push_back(item);
                        }
                    }
                }
            }
        }
    }
}

} // namespace aethernaut
