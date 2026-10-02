/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>

namespace yolodex {

struct Settings {
  /* Any coordinate is valid (x or y < 0 on a monitor left of / above the
   * primary), so whether a position was saved is its own flag. */
  bool has_position = false;
  int window_x = 0;
  int window_y = 0;
  int window_w = 720;
  int window_h = 480;
  int paned = 220;
  std::string last_path;
  int last_id = -1;
  double last_scroll = 0;
  std::string view = "list";
  std::string font_family = "Serif";
  int font_size = 12;
  int font_weight = 400;
  int palette = 1;  // 0 white, 1 eggshell, 2 dark

  void load();
  void save() const;
};

}  // namespace yolodex
