# YOLO-dex

![YOLO-dex List view](brand/screenshot-list.png)

An **index-card stack** for The Lunduke Computer Operating System (LCOS). The window is Windows 3.x / 95 Cardfile, not a Markdown notebook.

Binary: `yolodex`. Unlicense.

LCOS itself: [https://github.com/BryanLunduke/LCOS](https://github.com/BryanLunduke/LCOS)

## Status

**M3 in tree.** Find/Go To/restore last stack, plus Print / Print All, Edit Cut/Copy/Paste/Undo, Card Restore, and confirm Delete. Card view comes later.

| Doc | What |
|---|---|
| [DEVELOPMENT.md](DEVELOPMENT.md) | Locked decisions, architecture, milestones M0–M6 |

## Build

```
sudo apt install build-essential meson ninja-build pkg-config g++ libgtkmm-3.0-dev libxml2-dev
meson setup build
meson compile -C build
./build/yolodex
```

## License

[The Unlicense](https://unlicense.org). See [UNLICENSE](UNLICENSE).
