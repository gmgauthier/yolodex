/* SPDX-License-Identifier: Unlicense */

#include "save_path.hpp"
#include "check.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

int main()
{
  using yolodex::save_needs_overwrite_prompt;
  using yolodex::with_stack_suffix;

  CHECK(with_stack_suffix("/tmp/recipes") == "/tmp/recipes.yolodex");
  CHECK(with_stack_suffix("/tmp/recipes.yolodex") == "/tmp/recipes.yolodex");
  /* Any case of the suffix is already a stack name. */
  CHECK(with_stack_suffix("/tmp/Name.YOLODEX") == "/tmp/Name.YOLODEX");
  CHECK(with_stack_suffix("/tmp/Name.YoloDex") == "/tmp/Name.YoloDex");
  CHECK(with_stack_suffix("/tmp/notes.txt") == "/tmp/notes.txt.yolodex");

  const std::string dir = "/tmp/yolodex-save-" + std::to_string(static_cast<long long>(getpid()));
  const std::string typed = dir + "-recipes";
  const std::string existing = typed + ".yolodex";
  std::remove(existing.c_str());

  /* Nothing at the suffixed name: no extra prompt. */
  CHECK(!save_needs_overwrite_prompt(typed, with_stack_suffix(typed)));

  /* The chooser confirmed "recipes", but "recipes.yolodex" is what would be
   * replaced: that needs its own confirmation. */
  std::ofstream(existing) << "x";
  CHECK(save_needs_overwrite_prompt(typed, with_stack_suffix(typed)));

  /* The chooser already confirmed the exact file. */
  CHECK(!save_needs_overwrite_prompt(existing, with_stack_suffix(existing)));

  std::remove(existing.c_str());
  return suite_test::done("save_path");
}
