Software OpenGL bundles
=======================

Each subfolder here is one entry in the launcher's "OpenGL" dropdown.
At launch time, every file in the chosen subfolder is copied into the
JRE's bin directory next to java.exe (with java.exe.local dropped in
to defeat KnownDLLs), and an optional env.txt is read to set environment
variables for the JVM. The launcher tracks what it copied and reverses
the install on the next launch, so switching modes is clean.

What's bundled out of the box (newest to oldest):

  mesa-llvmpipe/                Mesa 26 llvmpipe (modern, JIT)
  mesa-softpipe/                Mesa 26 softpipe (modern, no JIT, slowest)
      pal1000/mesa-dist-win 26.0.6. Win7+/wine. OpenGL 4.6.
      Two copies of the same DLLs; GALLIUM_DRIVER picks the driver.

  mesa9x-win98-llvmpipe/        Mesa 25 llvmpipe, Win9x port
  mesa9x-win98-softpipe/        Mesa 25 softpipe, Win9x port (smaller)
      JHRobotics/mesa9x v23.1.9.138 release. Drop-in opengl32 designed
      to load on Windows 98/Me (and any newer Windows). Llvmpipe needs
      SSE-capable CPU; softpipe doesn't.

  mesa-20-fdossena/             Mesa 20.1.8 (Dossena classic)
      fdossena.com legacy build, Sept 2020. Single 28 MB opengl32.dll.
      A solid all-rounder that worked well across Win7/8.1/10.

  mesa-18-fdossena/             Mesa 18.0.4 (Dossena, even older)
      fdossena.com legacy build, May 2018. 24 MB opengl32.dll. Try this
      when newer builds glitch out on weird drivers.

  mesa9x-17-llvmpipe/           Mesa 17 llvmpipe, Win98/Me
  mesa9x-17-softpipe/           Mesa 17 softpipe (only ~10 MB)
      JHRobotics/mesa9x v17.3.9.3, May 2023. The softpipe variant is
      the smallest software OpenGL we ship - drop in if you suspect
      LLVM JIT is misbehaving on your guest.

Sources:
  https://github.com/pal1000/mesa-dist-win  (mesa-* directories)
  https://github.com/JHRobotics/mesa9x      (mesa9x-* directories)
  https://fdossena.com/?p=mesa/index.frag   (mesa-*-fdossena directories)

Adding your own
---------------

  resources/gl/<name>/
      opengl32.dll        (required)
      <any extra DLLs>    (also copied)
      env.txt             (optional, KEY=VALUE per line)
      name.txt            (optional, single line - shown in dropdown)

If env.txt is present its variables are exported to the JVM and reset
when the launch returns. If name.txt is present its first line is the
display name; otherwise the dropdown shows "Software: <foldername>".

How the install actually works
------------------------------

opengl32.dll is on Windows' KnownDLLs registry list (XP and later), so
the loader uses the system32 copy regardless of what's in the working
directory. To override it the launcher:

  1. Resolves java.exe to a full path via SearchPathA.
  2. Copies the chosen mode's files into that directory, backing up
     anything already there as <file>.mc9xbak.
  3. Drops zero-byte java.exe.local and javaw.exe.local files there.
     These are the documented switch that tells Windows to search the
     EXE's own folder before consulting KnownDLLs.
  4. Records what it touched in <launcher>\.gl_jre_install. The next
     launch reads that file, deletes our copies, and renames the
     backups back to their original names.

If your Java is in C:\Program Files\..., the launcher needs admin to
install. If 'java=' in settings.txt is just 'java', SearchPathA will
pick whatever the system PATH resolves to - point it at the real
JRE\bin\java.exe (full path) if the override is not taking effect.

Notes on Voodoo / 3dfx hardware
-------------------------------

3dfx Voodoo cards (Voodoo 1..5) on real Windows 98 already get OpenGL
through the 3dfx driver's installed ICD (3dfxvgl.dll). That's the
"Default (system OpenGL)" mode - no override needed. Voodoo OpenGL caps
out at ~OpenGL 1.2, which means it can render Minecraft alpha/beta and
maybe up through 1.5.2. For 1.6.4+ you want one of the software bundles.

Modern Glide-wrappers like dgVoodoo2 and nGlide are unrelated to this -
they translate Glide for old Glide-only games to modern Direct3D, and
they don't run on Win9x.
