/* SPDX-License-Identifier: Unlicense */

#include "stack.hpp"
#include "check.hpp"

#include <cstdlib>
#include <fstream>
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

  {
    /* XML 1.0 illegal controls (VT, FF, other C0) must not make the saved
     * stack unreadable. Tab, LF, and CR survive. */
    yolodex::Stack s;
    s.create_new();
    s.commit(Glib::ustring("Ctl\x0B" "Index"), Glib::ustring("a\tb\nc\x0C" "d\x01" "e"));
    const int other = s.add();
    CHECK(other > 0);
    s.commit("Plain", "kept");
    const std::string p2 =
        "/tmp/yolodex-ctl-" + std::to_string(static_cast<long long>(getpid())) + ".yolodex";
    CHECK(s.save_as(p2));
    yolodex::Stack back;
    CHECK(back.open(p2));
    CHECK(back.count() == 2);
    CHECK(back.cards()[0].index == "CtlIndex");
    CHECK(back.cards()[0].body == "a\tb\ncde");
    CHECK(back.cards()[1].index == "Plain");
    CHECK(back.cards()[1].body == "kept");
    std::remove(p2.c_str());
  }

  {
    /* A stack already saved with a raw control byte still opens. */
    const std::string p3 =
        "/tmp/yolodex-raw-" + std::to_string(static_cast<long long>(getpid())) + ".yolodex";
    {
      std::ofstream out(p3, std::ios::binary);
      out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<yolodex version=\"1\">\n"
          << "  <card id=\"1\">\n    <index>Bad\x0Bone</index>\n    <body>x</body>\n  </card>\n"
          << "  <card id=\"2\">\n    <index>Good</index>\n    <body>y</body>\n  </card>\n"
          << "</yolodex>\n";
    }
    yolodex::Stack raw;
    CHECK(raw.open(p3));
    CHECK(raw.count() == 2);
    std::remove(p3.c_str());
  }

  return suite_test::done("stack");
}
