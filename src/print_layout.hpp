/* SPDX-License-Identifier: Unlicense */

#pragma once

namespace yolodex {

/* Equal slots on a page. One card in the job uses the whole page. More than
 * one still uses four. */
int print_slots(int cards_in_job);

/* Height of one slot. slots below 1 is treated as 1. */
double print_slot_height(double page_h, double gap, int slots);

}  // namespace yolodex
