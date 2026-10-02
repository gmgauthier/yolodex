/* SPDX-License-Identifier: Unlicense */

#include "card_face.hpp"
#include "check.hpp"

#include <gtkmm.h>

int main(int argc, char** argv)
{
  /* CardFace is a widget: it needs a display. Skip (77) where there is none,
   * as in a package build. */
  if (!gtk_init_check(&argc, &argv))
    return 77;
  Gtk::Main::init_gtkmm_internals();

  yolodex::CardFace face;
  face.set_enabled(true);
  face.set_index("Old");
  face.set_body("body");
  face.take_restore_point();
  CHECK(!face.can_undo());
  CHECK(!face.can_restore());

  /* The user edits the body. */
  face.body_view().get_buffer()->set_text("edited");
  CHECK(face.can_undo());
  CHECK(face.can_restore());

  /* Card -> Index -> OK with the same index: nothing changes. */
  CHECK(!face.edit_index("Old"));
  CHECK(face.index() == "Old");
  CHECK(face.can_undo());
  CHECK(face.can_restore());

  /* A new index is an ordinary edit: Undo puts the old one back, and the
   * body edit before it is still undoable. */
  CHECK(face.edit_index("New"));
  CHECK(face.index() == "New");
  CHECK(face.can_undo());
  CHECK(face.can_restore());
  CHECK(face.undo());
  CHECK(face.index() == "Old");
  CHECK(face.body() == "edited");
  /* (A buffer set_text is a delete then an insert, so the body edit may take
   * more than one Undo step.) */
  while (face.undo()) {
  }
  CHECK(face.body() == "body");
  CHECK(face.index() == "Old");

  /* Restore still goes back to the text the card was opened with. */
  face.edit_index("Again");
  face.restore();
  CHECK(face.index() == "Old");
  CHECK(face.body() == "body");

  return suite_test::done("card_face");
}
