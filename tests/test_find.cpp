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

void test_offsets_are_in_the_original_text()
{
  int start = -1;
  int len = -1;
  /* "ß" folds to "ss": the hit is the one character "ß" at offset 4. */
  CHECK(yolodex::find_in_text("Straße", "ss", 0, start, len));
  CHECK(start == 4);
  CHECK(len == 1);
  CHECK(yolodex::find_in_text("Straße", "STRASSE", 0, start, len));
  CHECK(start == 0);
  CHECK(len == 6);
  /* A match after a growing character keeps its real offset. */
  CHECK(yolodex::find_in_text("ﬁle one", "one", 0, start, len));
  CHECK(start == 4);
  CHECK(len == 3);
  CHECK(yolodex::find_in_text("İx", "x", 0, start, len));
  CHECK(start == 1);
  CHECK(len == 1);
  /* Resume offsets are original offsets too. */
  CHECK(yolodex::find_in_text("ßa ss", "ss", 0, start, len));
  CHECK(start == 0);
  CHECK(len == 1);
  CHECK(yolodex::find_in_text("ßa ss", "ss", start + len, start, len));
  CHECK(start == 3);
  CHECK(len == 2);
  CHECK(!yolodex::find_in_text("ßa ss", "ss", start + len, start, len));
  /* ASCII is unchanged. */
  CHECK(yolodex::find_in_text("Hello World", "WORLD", 0, start, len));
  CHECK(start == 6);
  CHECK(len == 5);
}

void test_resume_after_growing_character_in_cards()
{
  /* Body "Straße strasse": Find "ss" hits "ß" (4), then "ss" in strasse (11), then wraps. */
  const std::vector<yolodex::Card> cards = {card(1, "K", "Straße strasse")};
  yolodex::FindHit hit;
  CHECK(yolodex::find_in_cards(cards, "ss", 0, 1, 0, hit));
  CHECK(hit.offset == 4);
  CHECK(hit.length == 1);
  CHECK(yolodex::find_in_cards(cards, "ss", 0, 1, hit.offset + hit.length, hit));
  CHECK(hit.offset == 11);
  CHECK(hit.length == 2);
  CHECK(yolodex::find_in_cards(cards, "ss", 0, 1, hit.offset + hit.length, hit));
  CHECK(hit.offset == 4);
}

}  // namespace

int main()
{
  test_find_next_wraps_within_one_field();
  test_find_next_moves_to_later_card();
  test_single_match_is_found_again();
  test_offsets_are_in_the_original_text();
  test_resume_after_growing_character_in_cards();
  return suite_test::done("find");
}
