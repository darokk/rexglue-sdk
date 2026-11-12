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
  // Reuploads skipped because the data was unchanged (content fingerprints).
  std::atomic<uint64_t> texture_loads_skipped{0};
  std::atomic<uint64_t> upload_bytes_skipped{0};
  // Texture loads of data the GPU wrote (resolves, memexport).
  std::atomic<uint64_t> texture_loads_gpu_written{0};
  // Render target ownership transfers (copies between host render targets
  // sharing EDRAM): count, EDRAM tiles, and how many have a depth destination.
  std::atomic<uint64_t> rt_transfers{0};
  std::atomic<uint64_t> rt_transfer_tiles{0};
  std::atomic<uint64_t> rt_transfers_to_depth{0};
  // Transfers into color render targets skipped because the draw overwrites
  // every pixel they would copy.
  std::atomic<uint64_t> rt_transfers_skipped{0};
  // Resolves (EDRAM to memory copies): count, EDRAM tiles dumped from host
  // render targets for them, and resolve clears.
  std::atomic<uint64_t> resolves{0};
  std::atomic<uint64_t> resolve_dump_tiles{0};
  std::atomic<uint64_t> resolve_clears{0};
  std::atomic<uint64_t> shader_translations{0};
  std::atomic<uint64_t> pipelines_created{0};
};

inline FrameEvents g_frame_events;

inline void CountFrameEvent(std::atomic<uint64_t>& counter, uint64_t amount = 1) {
  counter.fetch_add(amount, std::memory_order_relaxed);
}

}  // namespace rex::graphics
