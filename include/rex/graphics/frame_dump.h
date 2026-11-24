#pragma once

// One-frame event dump (cvar log_frame_dump): every 20 s the GPU command
// thread records what one guest frame does with render targets - bindings,
// ownership transfers, resolves, texture loads of GPU-written data - with the
// draws between them summarized, and appends it to frame_dump.txt next to the
// executable. For working out which parts of the EDRAM emulation a game's
// frame really needs. Everything here runs on the GPU command thread.

#include <cstdint>
#include <cstdio>
#include <string>

#include <fmt/format.h>

namespace rex::graphics {

struct FrameDump {
  bool active = false;
  std::string text;
  // Current run of draws (flushed into text by the next other event).
  uint32_t run_draws = 0;
  uint64_t run_vertices = 0;
  uint32_t run_tessellated = 0;
  uint32_t run_memexport = 0;
  uint32_t run_scissor_changes = 0;
  uint32_t run_window_offset = 0;
  uint64_t run_bin_select = 0;
  uint64_t run_bin_mask = 0;
  uint32_t run_scissor_tl = 0;
  uint32_t run_scissor_br = 0;
  uint32_t run_last_scissor_tl = 0;
  uint32_t run_last_scissor_br = 0;
  uint64_t run_first_vs = 0;
  uint64_t run_first_ps = 0;
  // Depth/stencil state of the run's first draw (FrameDumpSetDrawState).
  uint32_t next_depth_control = 0;
  uint32_t next_stencil_ref_mask = 0;
  uint32_t next_color_mask = 0;
  // Tessellated draws: mode | host vertex shader type << 4, and the factor range.
  uint32_t next_tess = 0;
  float next_tess_min = 0.0f, next_tess_max = 0.0f;
  uint32_t run_first_tess = 0;
  float run_first_tess_min = 0.0f, run_first_tess_max = 0.0f;
  uint32_t run_first_depth_control = 0;
  uint32_t run_first_stencil_ref_mask = 0;
  uint32_t run_first_color_mask = 0;
  // Totals of the frame.
  uint32_t frame_draws = 0;
  uint32_t frame_transfers = 0;
  uint32_t frame_transfers_skipped = 0;
  uint32_t frame_resolves = 0;
  uint32_t frame_direct_resolves = 0;
  uint32_t frame_texture_loads = 0;
  uint32_t frame_clears_forwarded = 0;
  uint32_t frame_tile_draws_merged = 0;
  uint32_t frame_draw_failures = 0;
  uint32_t frame_missing_textures = 0;
  // Capture scheduling.
  uint32_t captures = 0;
  double next_capture_time = 0.0;
};

inline FrameDump g_frame_dump;

inline void FrameDumpFlushRun() {
  FrameDump& d = g_frame_dump;
  if (!d.run_draws) {
    return;
  }
  d.text += fmt::format(
      "    {} draws ({} vertices{}{}), window offset {},{}, scissor {},{}-{},{}{}, bin select "
      "{:016X} mask {:016X}, first vs {:016X} ps {:016X} depth {:08X} stencil {:08X} cmask "
      "{:X}\n",
      d.run_draws, d.run_vertices,
      d.run_tessellated ? fmt::format(", {} tessellated", d.run_tessellated) : std::string(),
      d.run_memexport ? fmt::format(", {} memexport", d.run_memexport) : std::string(),
      int32_t(d.run_window_offset << 17) >> 17, int32_t(d.run_window_offset << 1) >> 17,
      d.run_scissor_tl & 0x7FFF, (d.run_scissor_tl >> 16) & 0x7FFF, d.run_scissor_br & 0x7FFF,
      (d.run_scissor_br >> 16) & 0x7FFF,
      d.run_scissor_changes ? fmt::format(" (+{} other scissors)", d.run_scissor_changes)
                            : std::string(),
      d.run_bin_select, d.run_bin_mask, d.run_first_vs, d.run_first_ps, d.run_first_depth_control,
      d.run_first_stencil_ref_mask, d.run_first_color_mask);
  if (d.run_first_tess) {
    // Before the line's newline.
    d.text.pop_back();
    d.text += fmt::format(" tess mode {} host type {} factors {}-{}\n", d.run_first_tess & 0xF,
                          d.run_first_tess >> 4, d.run_first_tess_min, d.run_first_tess_max);
  }
  d.run_draws = 0;
  d.run_vertices = 0;
  d.run_tessellated = 0;
  d.run_memexport = 0;
  d.run_scissor_changes = 0;
}

inline void FrameDumpDraw(uint32_t window_offset, uint32_t scissor_tl, uint32_t scissor_br,
                          uint64_t bin_select, uint64_t bin_mask, uint64_t vs_hash,
                          uint64_t ps_hash, uint32_t vertices, bool tessellated, bool memexport) {
  FrameDump& d = g_frame_dump;
  if (d.run_draws && (d.run_window_offset != window_offset || d.run_bin_select != bin_select ||
                      d.run_bin_mask != bin_mask)) {
    FrameDumpFlushRun();
  }
  if (!d.run_draws) {
    d.run_window_offset = window_offset;
    d.run_bin_select = bin_select;
    d.run_bin_mask = bin_mask;
    d.run_scissor_tl = scissor_tl;
    d.run_scissor_br = scissor_br;
    d.run_first_vs = vs_hash;
    d.run_first_ps = ps_hash;
    d.run_first_depth_control = d.next_depth_control;
    d.run_first_stencil_ref_mask = d.next_stencil_ref_mask;
    d.run_first_color_mask = d.next_color_mask;
    d.run_first_tess = d.next_tess;
    d.run_first_tess_min = d.next_tess_min;
    d.run_first_tess_max = d.next_tess_max;
  } else if (d.run_last_scissor_tl != scissor_tl || d.run_last_scissor_br != scissor_br) {
    ++d.run_scissor_changes;
  }
  d.run_last_scissor_tl = scissor_tl;
  d.run_last_scissor_br = scissor_br;
  ++d.run_draws;
  ++d.frame_draws;
  d.run_vertices += vertices;
  d.run_tessellated += tessellated;
  d.run_memexport += memexport;
}

// An event other than a draw: ends the current run of draws.
inline void FrameDumpEvent(const std::string& line) {
  FrameDumpFlushRun();
  g_frame_dump.text += line;
  g_frame_dump.text += '\n';
}

}  // namespace rex::graphics
