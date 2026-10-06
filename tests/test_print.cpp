/* SPDX-License-Identifier: Unlicense */

#include "print_layout.hpp"
#include "check.hpp"

int main()
{
  CHECK(yolodex::print_slots(0) == 1);
  CHECK(yolodex::print_slots(1) == 1);
  CHECK(yolodex::print_slots(2) == 4);
  CHECK(yolodex::print_slots(4) == 4);
  CHECK(yolodex::print_slots(5) == 4);

  const double page = 800;
  const double gap = 10;
  const double full = yolodex::print_slot_height(page, gap, yolodex::print_slots(1));
  CHECK(full == page);

  const double quarter = yolodex::print_slot_height(page, gap, yolodex::print_slots(5));
  CHECK(quarter == (page - gap * 3) / 4);
  CHECK(quarter < full);

  return suite_test::done("print");
}
