# Bug backlog

Reviewed 2026-10-01 against the 0.1.2 sources.

`meson test` runs `tests/test_stack.cpp` (`stack`), `tests/test_save_path.cpp` (`save_path`), and `tests/test_find.cpp` (`find`). `stack` checks create, sort, prefix jump, remove, XML escape of `&` and `<`, and save/open round trip, including card text with XML-illegal control characters, and that duplicate or missing ids in a file become unique ids. `save_path` checks that Save As adds `.yolodex` only when no letter case of it is there, and that a suffixed name that already exists needs its own overwrite confirmation. `find` checks that Find Next wraps through the whole stack and back to the front of the field it resumed in. Open defects below are not locked by a test until they are fixed. Since v0.1.6 the loader keeps every id that is unique in the file and gives a missing, non-positive, or repeated id a new one, so a later edit cannot land on another card.

## Open

### Find applies casefold offsets to the original text

- Severity: incorrect
- Confidence: high
- Where: `src/main_window.cpp:85`, `src/main_window.cpp:955`, `src/card_face.cpp:118`
- Trigger: Search for `ss` in a body `Straße`. `ß`, `ﬁ`, and `İ` all grow under `casefold()`.
- Outcome: `u_find` returns an offset into the casefolded string. The hit length is `needle.casefold().size()`. `show_find_hit` applies both to the original buffer with `get_iter_at_offset`. For `Straße` / `ss` the highlight covers `ße`, not `ß`. Resume stays in casefold space, so a later real match can be skipped or land past the end of the buffer. ASCII queries are unaffected.

### Card → Index → OK clears undo even when the index did not change

- Severity: incorrect
- Confidence: high
- Where: `src/main_window.cpp:807`, `src/main_window.cpp:575`, `src/card_face.cpp:189`
- Trigger: Edit the body so Undo and Restore are available. Card → Index, press OK, including OK with the index unchanged. Cancel does not do this.
- Outcome: `refresh` calls `bind_face`, which calls `take_restore_point`. That captures the current text as the restore baseline and clears the undo stack. Edit → Undo does nothing. Edit → Restore reports that the card is already restored. The pre-dialog body cannot be put back.

### A window position left of or above the origin is saved and then ignored

- Severity: incorrect
- Confidence: high
- Where: `src/main_window.cpp:171`, `src/main_window.cpp:817`
- Trigger: Place the window on a monitor that sits left of or above the primary origin and quit. `-1, -1` is also the unset sentinel.
- Outcome: `persist` stores `get_position` as-is. The next launch moves only when both coordinates are `>= 0`. One negative axis drops both, and the window comes back at the default placement.

## Closed

None.

## Closed

### Find Next never rescans the front of the field it resumed in

- Severity: incorrect
- Confidence: high
- Where: `src/main_window.cpp:972`, `src/main_window.cpp:981`
- Trigger: One card whose body is `alpha alpha`. Find "alpha", then Find Next, then Find Next again.
- Outcome: Resume starts at `last_hit_offset_ + last_hit_length_`. Only the first slot of the scan uses that offset. The loop does not visit that same field again from offset 0, so the earlier "alpha" is skipped. Status becomes "Not found." `last_hit_*` is left unchanged (`src/main_window.cpp:1001`), so every later Find Next resumes at the same exhausted offset until the query changes. A match on a later card is still found.
- Fixed in v0.1.7: The search (now `src/find.cpp`) scans one more slot after wrapping, so the field it resumed in is searched again from its front. `alpha alpha` cycles 0, 6, 0, and a single match is found again.

### Duplicate card ids send later edits to the other card

- Severity: data-loss
- Confidence: high
- Where: `src/stack.cpp:339`, `src/stack.cpp:151`, `src/stack.cpp:208`
- Trigger: Open a stack whose first card has a missing or non-positive id and a later card is `id="1"`. Or two cards that already share an id. Arrow to the second of those cards and edit it.
- Outcome: A missing id is assigned `max_id + 1` in file order, so it can land on an id that a later card already has. `row_of_id` returns the first match. `commit` writes the text onto the card you edited, then `select_id` moves the selection to the other card and does not rebind the face. The face still shows the card you were editing. The next keystroke, Undo, or Save commits that text onto the other card. If `max_id + 1` is not positive, `next_id_` is forced back to 1 (`src/stack.cpp:350`), so Add creates another card with id 1.
- Fixed in v0.1.6: Open keeps each id that is unique in the file and gives a missing, non-positive, or repeated id a new one past the largest id. Edits and selection stay on the card being edited. Add and Duplicate skip ids in use and never wrap to an id that exists, including after an id of 2147483647.

### A control character in a card makes the saved stack refuse to open

- Severity: data-loss
- Confidence: high
- Where: `src/stack.cpp:18`, `src/stack.cpp:309`
- Trigger: An index or body contains an XML 1.0 illegal control, such as U+000B. Tab, LF, and CR are legal. VT, FF, and the other C0 controls are not.
- Outcome: `xml_escape` passes the byte through. `save` / `save_as` still return true. `open` uses `xmlReadFile` without `XML_PARSE_RECOVER`, so the next open fails with "Not a YOLO-dex stack." Every card in that file is unreachable.
- Fixed in v0.1.5: Characters XML 1.0 cannot hold (VT, FF, the other C0 controls, U+FFFE/U+FFFF) are left out when a stack is written; tab, LF, and CR stay. A stack an older build already wrote with such a byte opens through libxml2's recover mode, so its cards are reachable again.

### Save As appends `.yolodex` after the overwrite check

- Severity: data-loss
- Confidence: high
- Where: `src/main_window.cpp:619`, `src/main_window.cpp:678`
- Trigger: Save As, type `recipes` while `recipes.yolodex` already exists. Or pick a name that ends in `.YOLODEX`.
- Outcome: The chooser confirms overwrite of the name the user typed. `ensure_suffix` then appends `.yolodex` unless the path already ends in that exact lowercase suffix. `recipes` becomes `recipes.yolodex` and replaces that file with no confirmation. `Name.YOLODEX` becomes `Name.YOLODEX.yolodex`.
- Fixed in v0.1.4: `.yolodex` in any letter case counts as the suffix. When appending the suffix lands on a different file that already exists, Save As asks before replacing it; Cancel leaves the file alone.
