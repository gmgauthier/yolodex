/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>

namespace yolodex {

struct Settings {
  int window_x = -1;
  int window_y = -1;
  int window_w = 720;
  int window_h = 480;
  int paned = 220;
  std::string last_path;
  int last_id = -1;
  double last_scroll = 0;
  std::string view = "list";

  void load();
  void save() const;
};

}  // namespace yolodex
