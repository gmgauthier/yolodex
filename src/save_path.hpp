/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>

namespace yolodex {

/* The path Save As writes: unchanged if it already ends in ".yolodex" in any
 * letter case, otherwise with ".yolodex" appended. */
std::string with_stack_suffix(const std::string& path);

/* True when adding the suffix moved the save onto a different file that
 * already exists. The chooser only confirmed `chosen`, so `final_path` needs
 * its own overwrite confirmation. */
bool save_needs_overwrite_prompt(const std::string& chosen, const std::string& final_path);

}  // namespace yolodex
