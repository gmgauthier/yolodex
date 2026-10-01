/* SPDX-License-Identifier: Unlicense */

#include "stack.hpp"
#include "check.hpp"

#include <cstdlib>
#include <string>
#include <unistd.h>

int main()
{
  yolodex::Stack stack;
  CHECK(!stack.is_open());
  CHECK(stack.add() == -1);
  CHECK(!stack.save());

  stack.create_new();
  CHECK(stack.is_open());
  CHECK(!stack.dirty());
  CHECK(stack.count() == 1);
  CHECK(stack.display_name() == "Untitled");
  CHECK(stack.selected() != nullptr);

  const int first = stack.selected_id();
  stack.commit("Zeta", "last");
  CHECK(stack.dirty());
  const int second = stack.add();
  CHECK(second > first);
  stack.commit("Alpha", "a & b < c");
  CHECK(stack.count() == 2);
  CHECK(stack.cards()[0].index == "Alpha");
  CHECK(stack.cards()[1].index == "Zeta");
  CHECK(stack.go_to_prefix("ze"));
  CHECK(stack.selected()->index == "Zeta");
  CHECK(!stack.go_to_prefix("nope"));

  const int copy = stack.duplicate_selected();
  CHECK(copy > 0);
  CHECK(stack.count() == 3);
  CHECK(stack.remove_selected());
  CHECK(stack.count() == 2);

  const std::string path =
      "/tmp/yolodex-test-" + std::to_string(static_cast<long long>(getpid())) + ".xml";
  CHECK(stack.save_as(path));
  CHECK(!stack.dirty());
  CHECK(stack.display_name().find("yolodex-test-") == 0);

  yolodex::Stack loaded;
  CHECK(loaded.open(path));
  CHECK(loaded.count() == 2);
  CHECK(loaded.cards()[0].index == "Alpha");
  CHECK(loaded.cards()[0].body == "a & b < c");
  CHECK(loaded.cards()[1].index == "Zeta");
  CHECK(loaded.cards()[1].body == "last");
  CHECK(!loaded.open("/etc/hostname"));
  CHECK(!loaded.error().empty());

  loaded.close();
  CHECK(!loaded.is_open());
  CHECK(loaded.empty());

  std::remove(path.c_str());
  return suite_test::done("stack");
}
