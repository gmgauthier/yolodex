# Installing YOLO-dex

Four ways to get a binary, in the order LCOS cares about:

| Artifact | Who it is for |
|---|---|
| **`.deb`** | LCOS, Devuan Excalibur, Debian Trixie. Preferred. |
| **Source tarball** | Distro packagers and `meson setup && ninja install`. |
| **AppImage** | Fallback for distros that do not install `.deb` files. gtkmm only. Published on the GitHub/Gitea release. |
| **Git build** | Developers. See below. |

Version comes from `meson.build` (currently `0.1.0`).

## Runtime needs

- GTK 3 / gtkmm-3.0
- libxml2
- fontconfig

On Debian / Devuan / LCOS:

```
sudo apt install libgtkmm-3.0-1t64 libxml2 fontconfig
```

(Package names on older Debian may be `libgtkmm-3.0-1v5`.)

## 1. Debian package (preferred)

From a release `.deb`:

```
sudo apt install ./dist/yolodex_0.1.0-1_amd64.deb
```

Or, from this tree:

```
./scripts/release.sh deb
sudo apt install ./dist/yolodex_0.1.0-1_amd64.deb
```

That installs:

- `/usr/bin/yolodex`
- `/usr/share/applications/yolodex.desktop`
- `/usr/share/icons/hicolor/scalable/apps/yolodex.svg`
- `/usr/share/yolodex/skin/lcos/lcos.css`
- `/usr/share/yolodex/brand/icon-tile.svg`

Launch from the menu or `yolodex`. Config is `~/.config/yolodex/yolodex.ini`.

Uninstall: `sudo apt remove yolodex`.

## 2. Source tarball

`meson dist` produces `build/meson-dist/yolodex-VERSION.tar.xz` (sample stacks under `data/samples/` are git-only, not in the tarball).

```
tar -xf yolodex-0.1.0.tar.xz
cd yolodex-0.1.0
sudo apt install build-essential meson ninja-build pkg-config \
  libgtkmm-3.0-dev libxml2-dev libfontconfig1-dev
meson setup build --prefix=/usr
meson compile -C build
sudo meson install -C build
```

`./scripts/release.sh tarball` runs `meson dist` for you.

## 3. AppImage (fallback)

LCOS 0.3 already runs AppImages. The image bundles gtkmm from the build host.

```
./scripts/release.sh appimage
```

Requires `linuxdeploy` on `$PATH` (see <https://github.com/linuxdeploy/linuxdeploy>). Output lands under `dist/`.

```
chmod +x YOLO-dex-*.AppImage yolodex-*.AppImage
./yolodex-*.AppImage
```

The AppImage runtime sets `APPDIR`; YOLO-dex looks for skin and brand under `$APPDIR/usr/share/yolodex`. Leave `APPDIR` unset for `.deb` and `meson install` builds.

## 4. Developer build (no install)

```
meson setup build
meson compile -C build
./build/yolodex
```

The binary finds CSS via `SOURCE_ROOT` in the build tree. `YOLODEX_DATA` overrides that.

## One command for every artifact

```
./scripts/release.sh all
```

Writes tarball, `.deb`, and AppImage (if `linuxdeploy` is there) under `dist/`. The GitHub/Gitea release includes the AppImage as the non-deb fallback.

## What this project will not ship

- A Markdown dialect or notebook tree
- A systemd unit
- Vendored Clearlooks / xfwm themes (Recommends the desktop theme)
- Sample stacks in the tarball (they stay git-only)
