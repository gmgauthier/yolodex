/* SPDX-License-Identifier: Unlicense */

#include "settings.hpp"

#include <glib.h>
#include <glibmm/fileutils.h>
#include <glibmm/keyfile.h>
#include <glibmm/miscutils.h>

namespace yolodex {
namespace {

std::string config_dir()
{
  return Glib::build_filename(Glib::get_user_config_dir(), "yolodex");
}

std::string config_path()
{
  return Glib::build_filename(config_dir(), "yolodex.ini");
}

int get_int(Glib::KeyFile& kf, const char* group, const char* key, int fallback)
{
  try {
    if (kf.has_key(group, key))
      return kf.get_integer(group, key);
  } catch (const Glib::Error&) {
  }
  return fallback;
}

double get_dbl(Glib::KeyFile& kf, const char* group, const char* key, double fallback)
{
  try {
    if (kf.has_key(group, key))
      return kf.get_double(group, key);
  } catch (const Glib::Error&) {
  }
  return fallback;
}

std::string get_str(Glib::KeyFile& kf, const char* group, const char* key)
{
  try {
    if (kf.has_key(group, key))
      return kf.get_string(group, key);
  } catch (const Glib::Error&) {
  }
  return {};
}

}  // namespace

void Settings::load()
{
  Glib::KeyFile kf;
  try {
    kf.load_from_file(config_path());
  } catch (const Glib::Error&) {
    return;
  }
  window_x = get_int(kf, "window", "x", window_x);
  window_y = get_int(kf, "window", "y", window_y);
  window_w = get_int(kf, "window", "width", window_w);
  window_h = get_int(kf, "window", "height", window_h);
  paned = get_int(kf, "window", "paned", paned);
  last_path = get_str(kf, "session", "path");
  last_id = get_int(kf, "session", "card_id", last_id);
  last_scroll = get_dbl(kf, "session", "scroll", last_scroll);
  const std::string v = get_str(kf, "session", "view");
  if (!v.empty())
    view = v;
}

void Settings::save() const
{
  g_mkdir_with_parents(config_dir().c_str(), 0700);
  Glib::KeyFile kf;
  kf.set_integer("window", "x", window_x);
  kf.set_integer("window", "y", window_y);
  kf.set_integer("window", "width", window_w);
  kf.set_integer("window", "height", window_h);
  kf.set_integer("window", "paned", paned);
  kf.set_string("session", "path", last_path);
  kf.set_integer("session", "card_id", last_id);
  kf.set_double("session", "scroll", last_scroll);
  kf.set_string("session", "view", view);
  try {
    kf.save_to_file(config_path());
  } catch (const Glib::Error&) {
  }
}

}  // namespace yolodex
