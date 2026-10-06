/* SPDX-License-Identifier: Unlicense */

#include "print_layout.hpp"

namespace yolodex {

int print_slots(int cards_in_job)
{
  if (cards_in_job <= 1)
    return 1;
  return 4;
}

double print_slot_height(double page_h, double gap, int slots)
{
  const int n = slots < 1 ? 1 : slots;
  return (page_h - gap * (n - 1)) / static_cast<double>(n);
}

}  // namespace yolodex
