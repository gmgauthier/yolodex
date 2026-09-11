/* SPDX-License-Identifier: Unlicense */

#include "stack.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <glib.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <algorithm>
#include <sstream>

namespace yolodex {
namespace {

std::string xml_escape(const Glib::ustring& in)
{
  std::string out;
  out.reserve(in.bytes() + 8);
  for (const char c : in.raw()) {
    switch (c) {
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
        out += c;
        break;
    }
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
  c.id = next_id_++;
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
  c.id = next_id_++;
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
  int max_id = 0;
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

  for (Card& c : loaded) {
    if (c.id < 1)
      c.id = max_id + 1;
    if (c.id > max_id)
      max_id = c.id;
  }
  std::sort(loaded.begin(), loaded.end(), index_less);

  cards_ = std::move(loaded);
  path_ = path;
  next_id_ = max_id + 1;
  if (next_id_ < 1)
    next_id_ = 1;
  selected_row_ = cards_.empty() ? -1 : 0;
  open_ = true;
  dirty_ = false;
  return true;
}

}  // namespace yolodex
