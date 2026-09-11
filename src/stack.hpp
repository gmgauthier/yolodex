/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <glibmm/ustring.h>

#include <string>
#include <vector>

namespace yolodex {

struct Card {
  int id = 0;
  Glib::ustring index;
  Glib::ustring body;
};

class Stack {
 public:
  bool is_open() const { return open_; }
  bool dirty() const { return dirty_; }
  bool empty() const { return cards_.empty(); }
  const std::string& path() const { return path_; }
  const std::string& error() const { return error_; }

  std::string display_name() const;
  int count() const { return static_cast<int>(cards_.size()); }
  int selected_row() const { return selected_row_; }
  int selected_id() const;

  const std::vector<Card>& cards() const { return cards_; }
  Card* selected();
  const Card* selected() const;

  void close();
  void create_new();
  bool open(const std::string& path);
  bool save();
  bool save_as(const std::string& path);

  int add();
  int duplicate_selected();
  bool remove_selected();
  bool select_id(int id);
  bool select_row(int row);
  bool go_to_prefix(const Glib::ustring& prefix);
  void commit(const Glib::ustring& index, const Glib::ustring& body);

 private:
  void sort_cards();
  int row_of_id(int id) const;
  bool write_file(const std::string& path) const;

  std::vector<Card> cards_;
  std::string path_;
  std::string error_;
  int selected_row_ = -1;
  int next_id_ = 1;
  bool open_ = false;
  bool dirty_ = false;
};

}  // namespace yolodex
