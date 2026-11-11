#pragma once

#include <atomic>
#include <cstdint>

namespace rex::graphics {

// Running totals of host work that can make a single frame much slower than
// the frames around it. The guest frame rate log reports how much of each the
// slowest frame of every 5 s window did. Pipelines can be created on worker
// threads, hence atomics (all updates are relaxed, and they are rare events
// compared to draws).
struct FrameEvents {
  std::atomic<uint64_t> texture_loads{0};
  std::atomic<uint64_t> upload_bytes{0};  // guest memory copied to the GPU buffer
  std::atomic<uint64_t> shader_translations{0};
  std::atomic<uint64_t> pipelines_created{0};
};

inline FrameEvents g_frame_events;

inline void CountFrameEvent(std::atomic<uint64_t>& counter, uint64_t amount = 1) {
  counter.fetch_add(amount, std::memory_order_relaxed);
}

}  // namespace rex::graphics
