/* SPDX-License-Identifier: Unlicense */

#include "stack.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <glib.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <algorithm>
#include <climits>
#include <set>
#include <sstream>

namespace yolodex {
namespace {

/* XML 1.0 Char production. Anything else (VT, FF, other C0 controls,
 * U+FFFE/U+FFFF) cannot be written even as a character reference, and one
 * such byte makes the whole file unreadable. */
bool xml_char_ok(gunichar ch)
{
  return ch == 0x9 || ch == 0xA || ch == 0xD || (ch >= 0x20 && ch <= 0xD7FF) ||
         (ch >= 0xE000 && ch <= 0xFFFD) || (ch >= 0x10000 && ch <= 0x10FFFF);
}

std::string xml_escape(const Glib::ustring& in)
{
  std::string out;
  out.reserve(in.bytes() + 8);
  const std::string& raw = in.raw();
  const char* p = raw.c_str();
  const char* end = p + raw.size();
  while (p < end) {
    const gunichar ch = g_utf8_get_char_validated(p, end - p);
    if (ch == static_cast<gunichar>(-1) || ch == static_cast<gunichar>(-2)) {
      ++p; /* invalid byte: drop it */
      continue;
    }
    const char* next = g_utf8_next_char(p);
    if (!xml_char_ok(ch)) {
      p = next;
      continue;
    }
    switch (ch) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      default:
        out.append(p, next);
        break;
    }
    p = next;
  }
  return out;
}

std::string node_name(xmlNode* n)
{
  if (!n || !n->name)
    return {};
  return reinterpret_cast<const char*>(n->name);
}

std::string node_prop(xmlNode* n, const char* key)
{
  if (!n)
    return {};
  xmlChar* v = xmlGetProp(n, BAD_CAST key);
  if (!v)
    return {};
  std::string out(reinterpret_cast<const char*>(v));
  xmlFree(v);
  return out;
}

Glib::ustring node_text(xmlNode* n)
{
  if (!n)
    return {};
  xmlChar* v = xmlNodeGetContent(n);
  if (!v)
    return {};
  Glib::ustring out(reinterpret_cast<const char*>(v));
  xmlFree(v);
  return out;
}

xmlNode* find_child(xmlNode* parent, const char* name)
{
  for (xmlNode* n = parent ? parent->children : nullptr; n; n = n->next) {
    if (n->type == XML_ELEMENT_NODE && node_name(n) == name)
      return n;
  }
  return nullptr;
}

bool index_less(const Card& a, const Card& b)
{
  const Glib::ustring fa = a.index.casefold();
  const Glib::ustring fb = b.index.casefold();
  if (fa < fb)
    return true;
  if (fb < fa)
    return false;
  return a.id < b.id;
}

}  // namespace

std::string Stack::display_name() const
{
  if (path_.empty())
    return "Untitled";
  return Glib::path_get_basename(path_);
}

int Stack::selected_id() const
{
  const Card* c = selected();
  return c ? c->id : -1;
}

Card* Stack::selected()
{
  if (selected_row_ < 0 || selected_row_ >= count())
    return nullptr;
  return &cards_[static_cast<size_t>(selected_row_)];
}

const Card* Stack::selected() const
{
  if (selected_row_ < 0 || selected_row_ >= count())
    return nullptr;
  return &cards_[static_cast<size_t>(selected_row_)];
}

void Stack::close()
{
  cards_.clear();
  path_.clear();
  error_.clear();
  selected_row_ = -1;
  next_id_ = 1;
  open_ = false;
  dirty_ = false;
}

void Stack::create_new()
{
  close();
  Card c;
  c.id = 1;
  cards_.push_back(c);
  next_id_ = 2;
  selected_row_ = 0;
  open_ = true;
  dirty_ = false;
}

void Stack::sort_cards()
{
  const int id = selected_id();
  std::sort(cards_.begin(), cards_.end(), index_less);
  selected_row_ = row_of_id(id);
}

int Stack::row_of_id(int id) const
{
  if (id < 1)
    return cards_.empty() ? -1 : 0;
  for (int i = 0; i < count(); ++i) {
    if (cards_[static_cast<size_t>(i)].id == id)
      return i;
  }
  return cards_.empty() ? -1 : 0;
}

bool Stack::id_in_use(int id) const
{
  return std::any_of(cards_.begin(), cards_.end(), [id](const Card& c) { return c.id == id; });
}

int Stack::take_fresh_id()
{
  int id = next_id_ < 1 ? 1 : next_id_;
  while (id_in_use(id))
    id = id == INT_MAX ? 1 : id + 1;
  next_id_ = id == INT_MAX ? 1 : id + 1;
  return id;
}

bool Stack::select_id(int id)
{
  const int row = row_of_id(id);
  if (row < 0)
    return false;
  selected_row_ = row;
  return true;
}

bool Stack::select_row(int row)
{
  if (row < 0 || row >= count())
    return false;
  selected_row_ = row;
  return true;
}

bool Stack::go_to_prefix(const Glib::ustring& prefix)
{
  const Glib::ustring p = prefix.casefold();
  if (p.empty() || cards_.empty())
    return false;
  for (int i = 0; i < count(); ++i) {
    const Glib::ustring idx = cards_[static_cast<size_t>(i)].index.casefold();
    if (idx.size() >= p.size() && idx.compare(0, p.size(), p) == 0) {
      selected_row_ = i;
      return true;
    }
  }
  return false;
}

void Stack::commit(const Glib::ustring& index, const Glib::ustring& body)
{
  Card* c = selected();
  if (!c)
    return;
  if (c->index == index && c->body == body)
    return;
  const int id = c->id;
  const bool reindex = c->index != index;
  c->index = index;
  c->body = body;
  dirty_ = true;
  if (reindex)
    sort_cards();
  select_id(id);
}

int Stack::add()
{
  if (!open_)
    return -1;
  Card c;
  c.id = take_fresh_id();
  cards_.push_back(c);
  dirty_ = true;
  sort_cards();
  select_id(c.id);
  return c.id;
}

int Stack::duplicate_selected()
{
  const Card* src = selected();
  if (!src)
    return -1;
  Card c = *src;
  c.id = take_fresh_id();
  cards_.push_back(c);
  dirty_ = true;
  sort_cards();
  select_id(c.id);
  return c.id;
}

bool Stack::remove_selected()
{
  if (selected_row_ < 0 || selected_row_ >= count())
    return false;
  const int row = selected_row_;
  cards_.erase(cards_.begin() + row);
  dirty_ = true;
  if (cards_.empty()) {
    selected_row_ = -1;
    return true;
  }
  selected_row_ = std::min(row, count() - 1);
  return true;
}

bool Stack::write_file(const std::string& path) const
{
  std::ostringstream os;
  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  os << "<yolodex version=\"1\">\n";
  for (const Card& c : cards_) {
    os << "  <card id=\"" << c.id << "\">\n";
    os << "    <index>" << xml_escape(c.index) << "</index>\n";
    os << "    <body>" << xml_escape(c.body) << "</body>\n";
    os << "  </card>\n";
  }
  os << "</yolodex>\n";
  const std::string data = os.str();
  try {
    Glib::file_set_contents(path, data);
  } catch (const Glib::Error& e) {
    return false;
  }
  return true;
}

bool Stack::save()
{
  error_.clear();
  if (path_.empty()) {
    error_ = "No file name.";
    return false;
  }
  if (!write_file(path_)) {
    error_ = "Could not write " + path_ + ".";
    return false;
  }
  dirty_ = false;
  return true;
}

bool Stack::save_as(const std::string& path)
{
  error_.clear();
  if (path.empty()) {
    error_ = "No file name.";
    return false;
  }
  if (!write_file(path)) {
    error_ = "Could not write " + path + ".";
    return false;
  }
  path_ = path;
  dirty_ = false;
  open_ = true;
  return true;
}

bool Stack::open(const std::string& path)
{
  error_.clear();
  xmlDoc* doc = xmlReadFile(path.c_str(), nullptr, XML_PARSE_NONET | XML_PARSE_NOBLANKS);
  if (!doc) {
    /* Older builds wrote illegal control characters into card text. Recover
     * those stacks instead of locking every card away. */
    doc = xmlReadFile(path.c_str(), nullptr,
                      XML_PARSE_NONET | XML_PARSE_NOBLANKS | XML_PARSE_RECOVER | XML_PARSE_NOERROR |
                          XML_PARSE_NOWARNING);
  }
  if (!doc) {
    error_ = "Not a YOLO-dex stack.";
    return false;
  }

  xmlNode* root = xmlDocGetRootElement(doc);
  if (!root || node_name(root) != "yolodex") {
    xmlFreeDoc(doc);
    error_ = "Not a YOLO-dex stack.";
    return false;
  }

  std::vector<Card> loaded;
  for (xmlNode* n = root->children; n; n = n->next) {
    if (n->type != XML_ELEMENT_NODE || node_name(n) != "card")
      continue;
    Card c;
    const std::string id_s = node_prop(n, "id");
    if (!id_s.empty())
      c.id = static_cast<int>(g_ascii_strtoll(id_s.c_str(), nullptr, 10));
    if (xmlNode* idx = find_child(n, "index"))
      c.index = node_text(idx);
    if (xmlNode* body = find_child(n, "body"))
      c.body = node_text(body);
    loaded.push_back(std::move(c));
  }
  xmlFreeDoc(doc);

  /* Every card gets its own id: edits, selection, and the card list all find
   * a card by id. An id unique in the file is kept; a missing, non-positive,
   * or repeated id gets a new one past the largest id in the file. */
  int max_id = 0;
  for (const Card& c : loaded)
    max_id = std::max(max_id, c.id);
  std::set<int> seen;
  auto next_free = [&]() {
    if (max_id < INT_MAX)
      return ++max_id;
    int id = 1;
    while (seen.count(id) ||
           std::any_of(loaded.begin(), loaded.end(), [id](const Card& c) { return c.id == id; }))
      ++id;
    return id;
  };
  for (Card& c : loaded) {
    if (c.id < 1 || seen.count(c.id))
      c.id = next_free();
    seen.insert(c.id);
  }
  std::sort(loaded.begin(), loaded.end(), index_less);

  cards_ = std::move(loaded);
  path_ = path;
  next_id_ = max_id < INT_MAX ? max_id + 1 : 1;
  selected_row_ = cards_.empty() ? -1 : 0;
  open_ = true;
  dirty_ = false;
  return true;
}

}  // namespace yolodex
