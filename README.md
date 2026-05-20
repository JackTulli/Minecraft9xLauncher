# Win9x Minecraft Launcher

A native Win32 Minecraft launcher for Windows 95 and later. The launcher itself is a single 32-bit C++ executable (~64 KB compiled with OpenWatcom 1.9) - it isn't a Java app, so it loads instantly and runs fine on a 486-era machine. You'll need to provide your own Minecraft JAR files.

Minecraft itself only runs on Windows 98 and above with KernelEx. Versions higher than 1.9 require significant effort to get Java 8 working on Windows 98 - 1.12.2 with Forge is primarily recommended for Windows 2000 and XP systems.

## Features

- **Per-version launch profiles** - drop a `launch.ini` (main class, JVM args, classpath, game args with `{username}`/`{uuid}`/`{ram_min}`/`{ram}` placeholders) into any folder under `resources\versions\<id>\` and it appears in the dropdown.
- **Pluggable OpenGL backends** - default system OpenGL, an LWJGL software fallback, Mesa 26 llvmpipe/softpipe, and Mesa9x for Win98/Me are all bundled under `resources\gl\`. Drop your own folder containing an `opengl32.dll` there and it appears in the dropdown automatically. The launcher mirrors the chosen DLLs into both the version dir and the JRE's `bin\` (with a `.local` marker to defeat KnownDLLs on XP+), and cleans up the previous mode's files before installing the new one.
- **32-bit Java auto-discovery** - on launch the program scans `JAVA_HOME`, the `JavaSoft\Java Runtime Environment` / `Java Development Kit` registry keys (both views including `WOW6432Node`), and `Program Files` / `Program Files (x86)` for `java.exe`. Each candidate's PE header is checked and 64-bit binaries are hidden - Minecraft can't load the bundled i386 LWJGL natives with a 64-bit JVM, so they're filtered out before reaching the dropdown.
- **Bundled Java installers** - drop 32-bit JRE installers (`.exe` or `.msi`) into `resources\jre\` and click "Install Java..." to run any of them via ShellExecute. The repo ships with three: 1.5.0_12, 1.6.10, and 8u51 (all i586).
- **Sound via Betacraft proxy** - the launcher always passes `-Dhttp.proxyHost=betacraft.uk -Dhttp.proxyPort=80` to the JVM, so beta/alpha versions can fetch the resource listing from Betacraft's mirror of the dead `s3.amazonaws.com/MinecraftResources/` bucket. Files cache locally under `resources\versions\<id>\resources\` on first launch. Use Java 6 or higher - Java 5's OpenAL stack can't play the .ogg files even with the proxy.
- **No-sound toggle** - sets `-Dorg.lwjgl.openal.libname=disabled` to skip OpenAL for systems without working audio drivers.
- **Editable Xms/Xmx** - min and max heap are independent inputs, validated and persisted.
- **Offline UUID** - uses Mojang's `v3 MD5("OfflinePlayer:" + name)` scheme so worlds are tied to the username consistently across launches.

## Supported versions

`1.12.2forge`, `1.8.9`, `1.7.10`, `1.6.4`, `1.5.2`, `1.4.7`, `1.2.5`, `1.0`, `beta1.7.3`, `betahacks`, `alpha1.1.2`.

Vanilla 1.12.2 is not currently listed - it requires a full `assets/` folder that isn't shipped here. 1.12.2 with Forge works because Forge ships its own resource loading path.

## Building

```bash
./build.sh
```

Drives OpenWatcom 1.9 (bundled under `./WATCOM`) via Wine to produce `Minecraft9xLauncher.exe`. On Windows, point a PowerShell session at `WATCOM\binnt\` and invoke `wpp386` / `wlink` / `wrc` with the same arguments.

## Notes

- 3dfx Voodoo hardware uses 3dfx's own ICD via the "Default" OpenGL mode. Voodoo OpenGL caps near 1.2, so only alpha/beta through 1.5.2 will render.
- The launcher remembers the last-used version, username, RAM settings, GL mode, and Java path in `settings.txt`.

Discord: jtofexstinction.

<!-- TODO: replace the screenshot below with a fresh one showing the new Java dropdown + Install button. Drag-drop a PNG into GitHub's editor and it'll generate a new user-attachments URL. -->

![image](https://github.com/user-attachments/assets/79a377fb-a174-4d74-b6ac-c6f2ced25b08)
