/* SPDX-License-Identifier: Unlicense */

#include "save_path.hpp"

#include <sys/stat.h>

#include <cctype>

namespace yolodex {

std::string with_stack_suffix(const std::string& path)
{
  static const std::string suf = ".yolodex";
  if (path.size() >= suf.size()) {
    bool same = true;
    const size_t at = path.size() - suf.size();
    for (size_t i = 0; i < suf.size() && same; ++i)
      same = std::tolower(static_cast<unsigned char>(path[at + i])) == suf[i];
    if (same)
      return path;
  }
  return path + suf;
}

bool save_needs_overwrite_prompt(const std::string& chosen, const std::string& final_path)
{
  if (final_path == chosen)
    return false;
  struct stat st;
  return ::stat(final_path.c_str(), &st) == 0;
}

}  // namespace yolodex
