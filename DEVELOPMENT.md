# YOLO-dex development plan

A gtkmm-3 **index-card stack** for LCOS. The *window* is Windows 3.x / 95 Cardfile; the *payload* is a small XML file, not binary `.crd`.

Display name: **YOLO-dex**  
Binary / repo / package: `yolodex`  
License: The Unlicense (`UNLICENSE`)  
Repos: https://gitea.scriptorium/gmgauthier/yolodex (origin), https://github.com/gmgauthier/yolodex

Reference window: `brand/ui-reference.svg`

People records live in Ephemeris Contacts. YOLO-dex is notes on cards, not a rolodex.

## Status (2026-09-11)

**M6 in tree.** Packaging: `debian/`, `scripts/release.sh`, `INSTALL.md`. Tag `v0.1.0`.

## 1. Locked decisions

| Decision | Choice |
|---|---|
| Product | Original app. Chrome is Win 3.1 Cardfile. Format is ours, not Microsoft `.crd` |
| Name | YOLO-dex. Binary `yolodex`. APP_ID `org.gmgauthier.Yolodex` |
| Toolkit | C++17, gtkmm-3.0, GTK3 CSS, Meson |
| Look | One decorated window. Two views: **List** (index list + card face) and **Card** (stacked tabs + front card). No notebook of notebooks, no Markdown preview, no tags/folders |
| v1 format | One UTF-8 XML file, extension **`.yolodex`**. libxml2. Human-readable |
| Never as v1 | Binary `.crd` as the native format. Autodial. OLE/pictures. Sync. Markdown dialect. vCard fields |
| Later format | Optional **import** of classic `.crd` (MGC / RRG / DKO). Not a reason to become a `.crd` writer |
| Card | Index line + plain-text body. UTF-8. No 39-char / 440-char limits (those were the 16-bit object, not the look) |
| Sort | Always alphabetical by index, case-insensitive. Duplicate index lines allowed. No manual reorder in v1 |
| Open | File → Open replaces the current stack (one file, one window) |
| Network | None |
| Init | No systemd. Config `~/.config/yolodex/yolodex.ini` |
| Brand | Borrow LCOS beige / navy. Do **not** use Bryan’s seal. Mark is a small stack of index cards |
| License | The Unlicense |
| Versioning | Semantic (`MAJOR.MINOR.PATCH`). `meson.build` is the source of truth. Debian changelog and git tag `vX.Y.Z` match it. See **Process**. |

Why not Joplin / CherryTree / GNOME Notes: trees, Markdown, sync. This product is a stack of cards.

## 2. Window

### List view (ships first)

```
+------------------------------------------------------------------+
| File  Edit  View  Card  Search  Help                             |
+------------------------------------------------------------------+
| [Add] [Delete]   [Find] [Print]          [List] [Card]           |
+------------------------+-----------------------------------------+
| Apple pie              |  Index: Apple pie                       |
| Baker                  | ----------------------------------------|
| Contacts               |  Preheat the oven.                      |
| Zoo hours              |  375°F. Lattice the top.                |
+------------------------+-----------------------------------------+
| List — 4 cards                                                   |
+------------------------------------------------------------------+
```

Left: `Gtk::TreeView` of index lines (one column, ellipsize, no H-scroll — same lesson as Read-O-Matic Contents).
Right: the **card face** — index `Gtk::Entry` on a header strip, body `Gtk::TextView` below, framed as a beige card.

This is an upgrade of Win 3.1 List view (which showed titles only). You can see and edit the body without flipping views.

### Card view (M4, still v1)

Hide the left list. Draw a cascade of index tabs (Cairo) above the **same** card-face widget. Click a tab to select. PgUp / PgDn / mouse wheel walk the stack. Only as many tabs as fit.

### Menus (Win 3.1 shape)

```
File          Edit           View      Card         Search        Help
 New          Undo           ● List    Add          Go To…        About YOLO-dex
 Open…        Cut            ○ Card    Delete       Find…
 Save         Copy                     Duplicate    Find Next
 Save As…     Paste                    Index…
 ────────     ────────
 Print…       Restore
 Print All
 ────────
 Exit
```

No Autodial. No Merge in v1. No Picture / OLE.

`Card → Index…` opens a small dialog to edit the index line (authentic). The header Entry on the card face does the same job without the dialog.

## 3. File format

`people.yolodex` (M1):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<yolodex version="1">
  <card id="1">
    <index>Apple pie</index>
    <body>Preheat the oven.
375°F. Lattice the top.</body>
  </card>
</yolodex>
```

- `id` is a stable incrementing integer. Last-card restore keys on id, not the index text
- Body is plain text. Newlines are real line breaks inside `<body>`; `&`, `<`, and `>` are escaped. Custom writer (not libxml pretty-print) so indent does not leak into the body. Load with libxml2.
- Pretty-print samples so they are grep-able
- Not zip. Not JSON. Not a directory of notes

MIME: `application/x-yolodex`.

## 4. Architecture

```
yolodex
├── brand/                   icon-tile.svg, ui-reference.svg
├── data/
│   ├── yolodex.desktop
│   ├── samples/             git-only, export-ignore (M1)
│   └── skin/lcos/lcos.css
├── src/
│   ├── main.cpp
│   ├── application.{hpp,cpp}   Gtk::Application + flock + uniqueness
│   ├── paths.{hpp,cpp}         SOURCE_ROOT / DATADIR / YOLODEX_DATA
│   ├── main_window.{hpp,cpp}
│   ├── about_dialog.{hpp,cpp}
│   ├── card_face.{hpp,cpp}     index Entry + body TextView
│   ├── stack.{hpp,cpp}         M1: cards + XML load/save
│   ├── card_view.{hpp,cpp}     M4: Cairo tab cascade
│   ├── settings.{hpp,cpp}      M2: ini
│   └── find_dialog.{hpp,cpp}   M2: Search → Find
├── debian/                  M6
├── scripts/release.sh       M6
├── meson.build              0.1.0
├── README.md
└── DEVELOPMENT.md
```

Dependencies: **gtkmm-3.0** now; **libxml-2.0** from M1. No libarchive.

Uninstalled binary finds CSS via `SOURCE_ROOT`. Installed binary uses `DATADIR`. `YOLODEX_DATA` overrides both.

### `CardFace`

Reusable widget for both views. Bind a card (or nullptr to blank/disable). Index commit (Enter or focus-out) emits `signal_index_changed`. Body edits emit `signal_body_changed`.

CSS: beige card (`#F7F5EF`), index strip `#E8E4D8`, Clearlooks-proof selection `#3D6AA8` / white. Client chrome `#E6E6E1`.

### `Stack` (M1)

In-memory vector, **kept sorted** by index (`ustring.casefold()`). Re-sort when the index Entry commits. Do not re-sort on every keystroke. Dirty flag. Title: `YOLO-dex - filename.yolodex` with `*` when dirty.

### List hover / left pin (from Read-O-Matic Contents)

Clearlooks ignores treeview CSS. Same trick: `SELECTION_NONE` + cell-data hover/sticky `#C4C4BC`. `POLICY_NEVER` on H-scroll, ellipsize, xalign 0, do not `set_cursor` / `scroll_to_row` horizontally, snap hadjustment to lower (`keep_nav_left`, capture pointers).

## 5. Original limits we will not clone

Win 3.1 Cardfile: 39-character index, 11×40 body (440 chars), 64K segment (~1260 cards). YOLO-dex allows long UTF-8 index + unbounded body. Ellipsize the list and the Card-view tabs.

## 6. Print (M3)

`Gtk::PrintOperation` + Pango/Cairo.

- **Print…** — the front card, as one card-shaped rectangle
- **Print All** — ~4 cards per page, single column of little cards. Clip body; do not paginate one card across pages

## 7. Milestones

v1.0 = M0–M6. Feature set of 0.1.x.

### M0 — Window

Scaffold. Meson, `Application` flock, `MainWindow`, menus, empty List layout, toolbar, About, `lcos.css`, `brand/ui-reference.svg`, `.desktop`. Status: `No stack open.` File → New shows one blank card in memory (unsaved). Add/Delete/Find/Print/Card view are stubs.

### M1 — Stack file

`Stack` XML load/save. Add / Delete / Duplicate. Index + body bind. Alpha sort. Dirty / New / Open / Save / Save As. Confirm discard if dirty. Sample `data/samples/recipes.yolodex` (git-only, `export-ignore`).

### M2 — Search + restore

Find / Find Next (case-insensitive, index then body, wrap the stack). Go To prefix dialog. Prev/Next. Keys:

| Key | Action |
|---|---|
| Ctrl+N / O / S / Shift+S | New / Open / Save / Save As |
| Insert | Add card |
| Delete | Delete card (if focus is the list, not the body) |
| Ctrl+F / F3 | Find / Find Next |
| Ctrl+G | Go To |
| Ctrl+P | Print |
| F5 / Shift+F5 | List / Card |
| Alt+← / Alt+→ | Prev / Next card |

Restore last file + last card id from `~/.config/yolodex/yolodex.ini`. If cheap, also save body scroll on the selected card.

### M3 — Print + List polish

Print / Print All. Card-face chrome. Cut/Copy/Paste/Undo via the TextView buffer. `Card → Restore` = revert current card to last saved snapshot. Confirm Delete. (List hover/left-pin landed with M1.)

### M4 — Card view

`View → Card`: hide the list, show Cairo tab cascade + the same `CardFace`. Click tab to select. Wheel / PgUp / PgDn walk. Remember view in ini. List view remains the editing workhorse.

### M5 — Small extras still in v1

Status `List — n cards` / `Card — i of n` already shipped. Options → Appearance matches Read-O-Matic (family, size, weight from Pango + `~/.local/share/fonts`, three palettes). Card title/index is always bold. Persist in `[topic]` of the ini.

### M6 — Package (this slice)

`debian/` native 3.0 Meson dh, `scripts/release.sh` (tarball + `.deb` + AppImage). AppImage is the published fallback for distros that do not install `.deb`. `INSTALL.md`. LCOS VM screenshots in README. Tag `v0.1.0`. Push origin + github. Attach `.deb`, tarball, and AppImage.

## 8. Parked (not v1)

- Pictures / OLE on a card
- Autodial
- Merge two stacks
- Import classic `.crd`
- Markdown, checkboxes, tags, notebooks
- vCard / phone-book fields (people records are Ephemeris Contacts)
- Sync, cloud, multiple windows of the same file
- Manual drag-reorder
- “Library” of stacks

## 9. Traps

- Parsing Microsoft `.crd` as the native format (MGC/RRG/DKO + OLE)
- Becoming CherryTree (trees, rich text, attachments)
- Hard-cloning 39/440 character limits
- Custom title bar
- Overriding `GTK_THEME`; use `prefer_light_theme` only
- Horizontal scroll on the index list (Read-O-Matic Contents)
- Dangling refs in realize/map lambdas (capture pointers)
- Shipping AppImage as a release gate

## Process

Do not commit to `master`. Every change lands through a pull request.

### Branches

- `feature/<short-name>` — new user-visible work
- `fix/<short-name>` — bugs, packaging nits, regressions

Open a pull request into `master`. Merge only after review.

### Gates

A pull request must pass **lint** before merge. CI runs `./scripts/lint.sh` (no `--fix`). Locally:

- `./scripts/lint.sh --fix` — clang-format rewrites `src/`
- `./scripts/lint.sh` — SPDX headers, no tabs, clang-format `--dry-run --Werror`, cppcheck (`warning`) on `src/`
- `meson compile` with this tree’s `warning_level=2` is clean (no new warnings)

Do not pass `--fix` in CI. Do not merge a red PR.

**Tests** are required when they exist (`meson test -C build`). Until a test suite lands, the gate is lint plus a clean compile plus a manual pass of the change.

### Semantic versioning

Every **shipped** pull request — merged to `master` and tagged as a release — bumps the version. `meson.build` is the source of truth. Keep these in lockstep in the same PR:

- `meson.build` `version:`
- `debian/changelog` (new stanza)
- git tag `vMAJOR.MINOR.PATCH` after merge

Then `./scripts/release.sh` produces `.deb`, tarball, and AppImage.

| Bump | When |
|---|---|
| **PATCH** (`x.y.Z`) | Bug fix or packaging. No new user-facing feature. |
| **MINOR** (`x.Y.0`) | New backward-compatible feature. |
| **MAJOR** (`X.0.0`) | Breaking change: native file format, dropped config keys, removed UI users rely on. |

While the version is `0.y.z`, still bump MINOR and PATCH this way. Do not treat 0.x as a free-for-all. The Debian revision (`-1`, `-2`) is only for rebuilding the same upstream version with no source change.
