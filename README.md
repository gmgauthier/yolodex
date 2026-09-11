# YOLO-dex

![YOLO-dex List view](brand/screenshot-list.png)

An **index-card stack** for The Lunduke Computer Operating System (LCOS). The window is Windows 3.x / 95 Cardfile, not a Markdown notebook.

Binary: `yolodex`. Unlicense.

LCOS itself: [https://github.com/BryanLunduke/LCOS](https://github.com/BryanLunduke/LCOS)

## Status

**M5 in tree.** Appearance (family/size/weight, three palettes, fonts from `~/.local/share/fonts`). Card title stays bold. Packaging is M6.

| Doc | What |
|---|---|
| [DEVELOPMENT.md](DEVELOPMENT.md) | Locked decisions, architecture, milestones M0–M6 |

## Build

```
sudo apt install build-essential meson ninja-build pkg-config g++ libgtkmm-3.0-dev libxml2-dev libfontconfig1-dev
meson setup build
meson compile -C build
./build/yolodex
```

## License

[The Unlicense](https://unlicense.org). See [UNLICENSE](UNLICENSE).
