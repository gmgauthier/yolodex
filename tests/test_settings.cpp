/* SPDX-License-Identifier: Unlicense */

#include "settings.hpp"
#include "check.hpp"

#include <glib.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

int main()
{
  const fs::path home =
      fs::temp_directory_path() /
      ("yolodex-settings-" + std::to_string(static_cast<long long>(getpid())));
  fs::create_directories(home);
  setenv("XDG_CONFIG_HOME", home.c_str(), 1);

  {
    /* No config yet: there is no saved position to restore. */
    yolodex::Settings s;
    s.load();
    CHECK(!s.has_position);
  }

  {
    /* A window left of / above the primary monitor round-trips. */
    yolodex::Settings s;
    s.window_x = -1200;
    s.window_y = -40;
    s.has_position = true;
    s.save();
    yolodex::Settings back;
    back.load();
    CHECK(back.has_position);
    CHECK(back.window_x == -1200);
    CHECK(back.window_y == -40);
  }

  {
    /* -1 is an ordinary coordinate, not "unset". */
    yolodex::Settings s;
    s.window_x = -1;
    s.window_y = 300;
    s.has_position = true;
    s.save();
    yolodex::Settings back;
    back.load();
    CHECK(back.has_position);
    CHECK(back.window_x == -1);
    CHECK(back.window_y == 300);
  }

  {
    /* Saving without a position leaves no stale coordinates behind. */
    yolodex::Settings s;
    s.save();
    yolodex::Settings back;
    back.load();
    CHECK(!back.has_position);
  }

  std::error_code ec;
  fs::remove_all(home, ec);
  return suite_test::done("settings");
}
