/* SPDX-License-Identifier: Unlicense */

#include "find.hpp"
#include "check.hpp"

#include <vector>

namespace {

yolodex::Card card(int id, const char* index, const char* body)
{
  yolodex::Card c;
  c.id = id;
  c.index = index;
  c.body = body;
  return c;
}

void test_find_next_wraps_within_one_field()
{
  /* One card, body "alpha alpha": Find, Find Next, Find Next cycles 0, 6, 0. */
  const std::vector<yolodex::Card> cards = {card(1, "Only", "alpha alpha")};
  yolodex::FindHit hit;
  CHECK(yolodex::find_in_cards(cards, "alpha", 0, 0, 0, hit));
  CHECK(!hit.in_index);
  CHECK(hit.offset == 0);
  CHECK(yolodex::find_in_cards(cards, "alpha", hit.row, 1, hit.offset + hit.length, hit));
  CHECK(hit.offset == 6);
  CHECK(yolodex::find_in_cards(cards, "alpha", hit.row, 1, hit.offset + hit.length, hit));
  CHECK(hit.offset == 0);
  CHECK(!hit.in_index);
}

void test_find_next_moves_to_later_card()
{
  const std::vector<yolodex::Card> cards = {card(1, "A", "alpha"), card(2, "B", "x alpha")};
  yolodex::FindHit hit;
  CHECK(yolodex::find_in_cards(cards, "alpha", 0, 1, 5, hit));
  CHECK(hit.row == 1);
  CHECK(hit.offset == 2);
  /* And wraps back to the first card. */
  CHECK(yolodex::find_in_cards(cards, "alpha", 1, 1, hit.offset + hit.length, hit));
  CHECK(hit.row == 0);
  CHECK(hit.offset == 0);
}

void test_single_match_is_found_again()
{
  const std::vector<yolodex::Card> cards = {card(1, "Key", "nothing here")};
  yolodex::FindHit hit;
  CHECK(yolodex::find_in_cards(cards, "key", 0, 0, 0, hit));
  CHECK(hit.in_index);
  CHECK(yolodex::find_in_cards(cards, "key", 0, 0, hit.offset + hit.length, hit));
  CHECK(hit.in_index);
  CHECK(hit.offset == 0);
  CHECK(!yolodex::find_in_cards(cards, "absent", 0, 0, 0, hit));
}

}  // namespace

int main()
{
  test_find_next_wraps_within_one_field();
  test_find_next_moves_to_later_card();
  test_single_match_is_found_again();
  return suite_test::done("find");
}
