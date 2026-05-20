#!/usr/bin/env bash
# Builds Minecraft9xLauncher.exe with OpenWatcom 1.9 under wine.
# Run from the project root: ./build.sh

set -e
cd "$(dirname "$0")"

if ! command -v wine >/dev/null; then
    echo "wine not found on PATH" >&2
    exit 1
fi
if [ ! -x "WATCOM/binnt/wpp386.exe" ]; then
    echo "WATCOM tree not found at ./WATCOM (need WATCOM/binnt/wpp386.exe)" >&2
    exit 1
fi

# Wine maps / to Z: by default; build a Windows-style path to the WATCOM tree.
WATCOM_WIN="Z:$(pwd | sed 's#/#\\#g')\\WATCOM"
INCLUDES="$WATCOM_WIN\\h;$WATCOM_WIN\\h\\nt"

echo ">> cleaning previous artifacts"
rm -f launcher.obj launcher.err launcher.res \
      Minecraft9xLauncher.exe Minecraft9xLauncher.map \
      Minecraft9xLauncher.sym Minecraft9xLauncher.lk1

echo ">> compiling launcher.cpp"
wine WATCOM/binnt/wpp386.exe launcher.cpp \
    -i="$INCLUDES" \
    -w4 -e25 -zq -od -d2 -6r -bt=nt -fo=.obj -mf -xs -xr 2>&1 \
    | grep -vE '^[[:space:]]*$|fixme:' || true
[ -f launcher.obj ] || { echo "compile failed (no launcher.obj)"; exit 1; }

echo ">> linking"
echo "FIL launcher.obj" > Minecraft9xLauncher.lk1
wine WATCOM/binnt/wlink.exe \
    name Minecraft9xLauncher \
    d all sys nt_win \
    op m op maxe=25 op q op symf \
    @Minecraft9xLauncher.lk1 2>&1 \
    | grep -vE '^[[:space:]]*$|fixme:' || true
[ -f Minecraft9xLauncher.exe ] || { echo "link failed (no exe)"; exit 1; }

if [ -f launcher.rc ]; then
    echo ">> binding resources (icon)"
    wine WATCOM/binnt/wrc.exe -q -bt=nt -i="$INCLUDES" \
        launcher.rc Minecraft9xLauncher.exe 2>&1 \
        | grep -vE '^[[:space:]]*$|fixme:' || true
fi

echo ">> done"
ls -la Minecraft9xLauncher.exe
file Minecraft9xLauncher.exe
