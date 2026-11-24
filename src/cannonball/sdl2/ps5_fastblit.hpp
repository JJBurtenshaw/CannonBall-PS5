// CannonBall-PS5 experimental fast nearest-neighbour scaler.
// Local patch to Cannonball; original Cannonball project/licensing unchanged.
// Pixel format must already match between source and destination (32-bit pixels).
#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <limits>

namespace ps5fast {

struct Rect {
    int x, y, w, h;
};

class NearestPlan {
public:
    NearestPlan() : source_width_(0), source_height_(0), framebuffer_width_(0),
                    framebuffer_height_(0), rectangle_{0, 0, 0, 0} {}

    bool configure(int source_width, int source_height,
                   int framebuffer_width, int framebuffer_height, Rect r) {
        if (source_width <= 0 || source_height <= 0 ||
            framebuffer_width <= 0 || framebuffer_height <= 0 ||
            r.x < 0 || r.y < 0 || r.w <= 0 || r.h <= 0 ||
            r.x > framebuffer_width || r.y > framebuffer_height ||
            r.w > framebuffer_width - r.x || r.h > framebuffer_height - r.y ||
            r.w > 16384 || r.h > 16384) {
            return false;
        }
        // Use 64-bit multiplication to avoid overflow in map generation.
        std::vector<int> map(static_cast<size_t>(r.w));
        for (int x = 0; x < r.w; ++x) {
            map[static_cast<size_t>(x)] = static_cast<int>(
                (static_cast<int64_t>(x) * source_width) / r.w);
        }
        xmap_.swap(map);
        source_width_ = source_width;
        source_height_ = source_height;
        framebuffer_width_ = framebuffer_width;
        framebuffer_height_ = framebuffer_height;
        rectangle_ = r;
        return true;
    }

    bool draw(const void *source_pixels, int source_pitch,
              void *framebuffer_pixels, int framebuffer_pitch) const {
        if (!source_pixels || !framebuffer_pixels ||
            source_pitch < source_width_ * 4 ||
            framebuffer_pitch < framebuffer_width_ * 4 ||
            source_pitch % 4 || framebuffer_pitch % 4 ||
            source_width_ <= 0 || source_height_ <= 0 ||
            xmap_.size() != static_cast<size_t>(rectangle_.w)) {
            return false;
        }
        const uint8_t *srcbytes = static_cast<const uint8_t *>(source_pixels);
        uint8_t *dstbytes = static_cast<uint8_t *>(framebuffer_pixels);
        int previous_source_y = -1;
        uint32_t *previous_destination = nullptr;
        const size_t row_bytes = static_cast<size_t>(rectangle_.w) * sizeof(uint32_t);

        for (int y = 0; y < rectangle_.h; ++y) {
            const int sy = static_cast<int>(
                (static_cast<int64_t>(y) * source_height_) / rectangle_.h);
            uint32_t *drow = reinterpret_cast<uint32_t *>(
                dstbytes + static_cast<size_t>(rectangle_.y + y) * framebuffer_pitch) + rectangle_.x;
            if (sy == previous_source_y) {
                // Most 1080p rows repeat a previous 224p row. Copy them in bulk.
                std::memcpy(drow, previous_destination, row_bytes);
            } else {
                const uint32_t *srow = reinterpret_cast<const uint32_t *>(
                    srcbytes + static_cast<size_t>(sy) * source_pitch);
                const int *map = xmap_.data();
                for (int x = 0; x < rectangle_.w; ++x) {
                    drow[x] = srow[map[x]];
                }
            }
            previous_source_y = sy;
            previous_destination = drow;
        }
        return true;
    }

    Rect rectangle() const { return rectangle_; }

private:
    int source_width_, source_height_;
    int framebuffer_width_, framebuffer_height_;
    Rect rectangle_;
    std::vector<int> xmap_;
};

} // namespace ps5fast
