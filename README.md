# YOLO-dex

**Vended by Grok Build**

![YOLO-dex List view on LCOS](brand/screenshot-list.png)

An **index-card stack** for The Lunduke Computer Operating System (LCOS). The window is Windows 3.x / 95 Cardfile, not a Markdown notebook.

Binary: `yolodex`. Unlicense.

LCOS itself: [https://github.com/BryanLunduke/LCOS](https://github.com/BryanLunduke/LCOS)

![Card view](brand/screenshot-cards.png)

![Find](brand/screenshot-find.png)

![Appearance](brand/screenshot-options.png)

![About](brand/screenshot-about.png)

## Status

**v0.1.0 (M0–M6).** Cardfile-shaped index-card stack: List and Card views, Find, print, Appearance, `.deb` + source tarball. See [INSTALL.md](INSTALL.md).

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

Install: [INSTALL.md](INSTALL.md).

## License

[The Unlicense](https://unlicense.org). See [UNLICENSE](UNLICENSE).
