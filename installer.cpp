/* Minecraft9xLauncher installer.
   Single-window wizard, built with OpenWatcom 1.9 for the same Win9x target
   as the launcher. The payload lives next to this exe in a `data\` folder
   (assembled by pack.ps1); we mirror it into the user's chosen install path.

   Pages: Welcome, License, Install Path, Java Check, Install, Finish.

   Win9x compatibility constraints: ANSI APIs only, no SHCreateDirectoryEx,
   no SHGetFolderPath; use SHGetSpecialFolderPath. Common Controls v5 only
   (PROGRESS_CLASS), no themes. */

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INSTALLER_VERSION  "1.7.0"
#define APP_NAME           "Minecraft9xLauncher"
#define APP_EXE            "Minecraft9xLauncher.exe"
#define DEFAULT_INSTALL    "C:\\Minecraft9xLauncher"
#define DATA_SUBDIR        "data"

/* Window/layout */
#define WIN_W              536
#define WIN_H              415
#define CLIENT_W           520
#define CLIENT_H           380
#define SIDEBAR_W          164
#define HEADER_H           60
#define BUTTON_Y           345
#define BUTTON_W           80
#define BUTTON_H           24

/* Pages */
#define PAGE_WELCOME       0
#define PAGE_LICENSE       1
#define PAGE_PATH          2
#define PAGE_JAVA          3
#define PAGE_INSTALL       4
#define PAGE_FINISH        5
#define NUM_PAGES          6

/* Control IDs */
#define ID_BACK            100
#define ID_NEXT            101
#define ID_CANCEL          102
#define ID_LICENSE_ACCEPT  110
#define ID_PATH_EDIT       120
#define ID_PATH_BROWSE     121
#define ID_JAVA_LIST       130
#define ID_JAVA_REFRESH    131
#define ID_FINISH_LAUNCH   140
#define ID_FINISH_JAVA     141

/* Worker -> UI messages */
#define WM_INSTALL_PROGRESS (WM_APP + 1)
#define WM_INSTALL_FILE     (WM_APP + 2)
#define WM_INSTALL_DONE     (WM_APP + 3)
#define WM_INSTALL_FAILED   (WM_APP + 4)

#define MAX_JAVAS          16
#define MAX_JRE_INSTALLERS 16

/* ---------- globals ---------- */

static HINSTANCE g_hInst;
static HWND      g_hwnd;
static HFONT     g_hFont, g_hFontBold, g_hFontTitle;
static HBITMAP   g_hSidebar;
static HICON     g_hAppIcon;       /* 128x128 minecraft.ico for sidebar overlay */
static int       g_currentPage = PAGE_WELCOME;

/* Buttons (persistent across pages) */
static HWND g_hBack, g_hNext, g_hCancel;

/* Welcome page */
static HWND g_hWelTitle, g_hWelBody;

/* License page */
static HWND g_hLicHeader, g_hLicSubhead, g_hLicText, g_hLicAccept;

/* Path page */
static HWND g_hPathHeader, g_hPathSubhead, g_hPathLabel, g_hPathEdit;
static HWND g_hPathBrowse, g_hPathSpace;

/* Java page */
static HWND g_hJavaHeader, g_hJavaSubhead, g_hJavaStatus, g_hJavaList;
static HWND g_hJavaRefresh, g_hJavaNote;

/* Install page */
static HWND g_hInsHeader, g_hInsSubhead, g_hInsCurrent, g_hInsProgress;
static HWND g_hInsLog;

/* Finish page */
static HWND g_hFinTitle, g_hFinBody, g_hFinLaunch, g_hFinJava;

/* User state */
static char g_installPath[MAX_PATH] = DEFAULT_INSTALL;
static char g_exeDir[MAX_PATH];          /* directory containing installer.exe */
static char g_payloadDir[MAX_PATH];      /* g_exeDir + "\\data" (dev mode) */
static int  g_havePayloadDir  = 0;       /* 1 if data\ exists alongside exe */
static int  g_haveEmbeddedPayload = 0;   /* 1 if archive footer is appended to .exe */
static DWORD g_embeddedArchiveLow  = 0;  /* file offset of archive start, low 32 */
static DWORD g_embeddedArchiveHigh = 0;  /* file offset of archive start, high 32 */
static DWORD g_embeddedFileCount   = 0;
static int  g_licenseAccepted = 0;
static int  g_have32Java      = 0;       /* set after Java scan */
static char g_chosenJre[MAX_PATH] = ""; /* JRE installer to run at finish */

/* Worker thread */
static HANDLE g_hWorker      = NULL;
static volatile LONG g_cancel = 0;
static int  g_installResult  = 0;        /* 1 = ok, 0 = failed */
static char g_installError[256] = "";

/* Java/JRE inventories */
typedef struct {
    char displayName[160];
    char path[MAX_PATH];
} JavaInstall;
static JavaInstall g_javas[MAX_JAVAS];
static int         g_javaCount = 0;

typedef struct {
    char name[64];
    char path[MAX_PATH];
} JreInstaller;
static JreInstaller g_jreInstallers[MAX_JRE_INSTALLERS];
static int          g_jreInstallerCount = 0;

/* ---------- small utilities ---------- */

static void CopyField(char *dst, size_t cap, const char *src) {
    if (cap == 0) return;
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = 0;
}

static void NormalizePath(char *s) {
    while (*s) { if (*s == '/') *s = '\\'; s++; }
}

static int PathExists(const char *p) {
    return GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES;
}

/* CreateDirectory the whole path, ignoring "already exists". */
static int EnsureDirectory(const char *path) {
    char buf[MAX_PATH];
    char *p;
    CopyField(buf, sizeof(buf), path);
    NormalizePath(buf);
    /* skip leading "X:\" */
    p = buf;
    if (p[0] && p[1] == ':') p += 3;
    else if (p[0] == '\\' && p[1] == '\\') {
        /* UNC; skip \\server\share */
        int sl = 0;
        p += 2;
        while (*p && sl < 2) { if (*p == '\\') sl++; p++; }
    }
    while (*p) {
        if (*p == '\\') {
            *p = 0;
            if (!CreateDirectoryA(buf, NULL)) {
                DWORD e = GetLastError();
                if (e != ERROR_ALREADY_EXISTS) { *p = '\\'; return 0; }
            }
            *p = '\\';
        }
        p++;
    }
    if (buf[0] && !CreateDirectoryA(buf, NULL)) {
        DWORD e = GetLastError();
        if (e != ERROR_ALREADY_EXISTS) return 0;
    }
    return 1;
}

static void GetExeDir(char *out, size_t cap) {
    char *bs;
    GetModuleFileNameA(NULL, out, (DWORD)cap);
    bs = strrchr(out, '\\');
    if (bs) *bs = 0;
}

/* ---------- PE bitness + Java discovery (lifted from launcher.cpp) ---------- */

static int GetExeBitness(const char *path) {
    HANDLE h;
    DWORD got = 0, peOff = 0;
    unsigned char dos[64];
    unsigned char hdr[24];
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    if (!ReadFile(h, dos, sizeof(dos), &got, NULL) || got < sizeof(dos)) {
        CloseHandle(h); return 0;
    }
    if (dos[0] != 'M' || dos[1] != 'Z') { CloseHandle(h); return 0; }
    peOff =  (DWORD)dos[60]
          | ((DWORD)dos[61] << 8)
          | ((DWORD)dos[62] << 16)
          | ((DWORD)dos[63] << 24);
    if (SetFilePointer(h, peOff, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        CloseHandle(h); return 0;
    }
    if (!ReadFile(h, hdr, sizeof(hdr), &got, NULL) || got < sizeof(hdr)) {
        CloseHandle(h); return 0;
    }
    CloseHandle(h);
    if (hdr[0] != 'P' || hdr[1] != 'E' || hdr[2] != 0 || hdr[3] != 0) return 0;
    {
        WORD machine = (WORD)(hdr[4] | (hdr[5] << 8));
        if (machine == 0x014C) return 32;
        if (machine == 0x8664 || machine == 0xAA64) return 64;
    }
    return 0;
}

static int JavaAlreadyKnown(const char *path) {
    int i;
    for (i = 0; i < g_javaCount; i++)
        if (_stricmp(g_javas[i].path, path) == 0) return 1;
    return 0;
}

static void TryAddJava(const char *candidate) {
    char full[MAX_PATH];
    char resolved[MAX_PATH];
    DWORD attrs;
    DWORD r;
    const char *base;

    if (!candidate || !candidate[0]) return;
    if (g_javaCount >= MAX_JAVAS) return;

    CopyField(full, sizeof(full), candidate);
    NormalizePath(full);

    attrs = GetFileAttributesA(full);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        size_t n = strlen(full);
        while (n > 0 && full[n - 1] == '\\') full[--n] = 0;
        _snprintf(full + n, sizeof(full) - n, "\\bin\\java.exe");
        full[sizeof(full) - 1] = 0;
    }
    if (GetFileAttributesA(full) == INVALID_FILE_ATTRIBUTES) return;

    r = GetFullPathNameA(full, sizeof(resolved), resolved, NULL);
    if (r == 0 || r >= sizeof(resolved)) CopyField(resolved, sizeof(resolved), full);

    if (JavaAlreadyKnown(resolved)) return;
    if (GetExeBitness(resolved) != 32) return;

    {
        JavaInstall *j = &g_javas[g_javaCount++];
        char home[MAX_PATH];
        char *bs;
        CopyField(home, sizeof(home), resolved);
        bs = strrchr(home, '\\'); if (bs) *bs = 0;
        bs = strrchr(home, '\\'); if (bs) *bs = 0;
        base = strrchr(home, '\\');
        base = base ? base + 1 : home;
        _snprintf(j->displayName, sizeof(j->displayName), "%s  (%s)", base, home);
        j->displayName[sizeof(j->displayName) - 1] = 0;
        CopyField(j->path, sizeof(j->path), resolved);
    }
}

static void ScanJavaRegistry(HKEY root, const char *subkey) {
    HKEY hKey, hSub;
    DWORD i;
    char ver[128];
    DWORD verLen;
    char home[MAX_PATH];
    DWORD homeLen, type;
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return;
    for (i = 0; ; i++) {
        verLen = sizeof(ver);
        if (RegEnumKeyExA(hKey, i, ver, &verLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
        if (RegOpenKeyExA(hKey, ver, 0, KEY_READ, &hSub) != ERROR_SUCCESS) continue;
        homeLen = sizeof(home);
        if (RegQueryValueExA(hSub, "JavaHome", NULL, &type,
                             (LPBYTE)home, &homeLen) == ERROR_SUCCESS
            && (type == REG_SZ || type == REG_EXPAND_SZ)) {
            home[sizeof(home) - 1] = 0;
            TryAddJava(home);
        }
        RegCloseKey(hSub);
    }
    RegCloseKey(hKey);
}

static void ScanProgramFilesJava(const char *programFiles) {
    static const char *roots[] = { "Java", "BellSoft", "Eclipse Adoptium", "Zulu" };
    int r;
    char pattern[MAX_PATH];
    char candidate[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    if (!programFiles || !programFiles[0]) return;
    for (r = 0; r < (int)(sizeof(roots) / sizeof(roots[0])); r++) {
        _snprintf(pattern, sizeof(pattern), "%s\\%s\\*", programFiles, roots[r]);
        pattern[sizeof(pattern) - 1] = 0;
        h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (fd.cFileName[0] == '.') continue;
            _snprintf(candidate, sizeof(candidate),
                      "%s\\%s\\%s\\bin\\java.exe",
                      programFiles, roots[r], fd.cFileName);
            candidate[sizeof(candidate) - 1] = 0;
            TryAddJava(candidate);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
}

static void DiscoverJavas(void) {
    char env[MAX_PATH];
    DWORD n;
    g_javaCount = 0;

    n = GetEnvironmentVariableA("JAVA_HOME", env, sizeof(env));
    if (n > 0 && n < sizeof(env)) TryAddJava(env);

    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\Java Runtime Environment");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\Java Development Kit");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\JRE");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\JDK");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\JavaSoft\\Java Runtime Environment");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\JavaSoft\\Java Development Kit");

    n = GetEnvironmentVariableA("ProgramFiles(x86)", env, sizeof(env));
    if (n > 0 && n < sizeof(env)) ScanProgramFilesJava(env);
    n = GetEnvironmentVariableA("ProgramFiles", env, sizeof(env));
    if (n > 0 && n < sizeof(env)) ScanProgramFilesJava(env);
    n = GetEnvironmentVariableA("ProgramW6432", env, sizeof(env));
    if (n > 0 && n < sizeof(env)) ScanProgramFilesJava(env);

    g_have32Java = (g_javaCount > 0);
}

/* List bundled JRE installers.
   - Dev mode: enumerate <payloadDir>\resources\jre\*.{exe,msi}
   - Embedded mode: hardcoded list, paths point at where they'll be after
     install (so the Finish-page ShellExecute finds them). */
static const char *kBundledJreNames[] = {
    "jre-8u51-windows-i586.exe",
    "jre-1_6_10-windows-i586.exe",
    "jre-1_5_0_12-windows-i586-p.exe"
};

static void DiscoverBundledJres(void) {
    g_jreInstallerCount = 0;

    if (g_haveEmbeddedPayload && !g_havePayloadDir) {
        int i;
        int n = (int)(sizeof(kBundledJreNames) / sizeof(kBundledJreNames[0]));
        for (i = 0; i < n && g_jreInstallerCount < MAX_JRE_INSTALLERS; i++) {
            JreInstaller *ji = &g_jreInstallers[g_jreInstallerCount++];
            CopyField(ji->name, sizeof(ji->name), kBundledJreNames[i]);
            _snprintf(ji->path, sizeof(ji->path),
                      "%s\\resources\\jre\\%s",
                      g_installPath, kBundledJreNames[i]);
            ji->path[sizeof(ji->path) - 1] = 0;
        }
        return;
    }

    {
        WIN32_FIND_DATAA fd;
        HANDLE h;
        char pattern[MAX_PATH];
        const char *ext;
        _snprintf(pattern, sizeof(pattern), "%s\\resources\\jre\\*", g_payloadDir);
        pattern[sizeof(pattern) - 1] = 0;
        h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) return;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            ext = strrchr(fd.cFileName, '.');
            if (!ext) continue;
            if (_stricmp(ext, ".exe") != 0 && _stricmp(ext, ".msi") != 0) continue;
            if (g_jreInstallerCount >= MAX_JRE_INSTALLERS) break;
            {
                JreInstaller *ji = &g_jreInstallers[g_jreInstallerCount++];
                CopyField(ji->name, sizeof(ji->name), fd.cFileName);
                _snprintf(ji->path, sizeof(ji->path),
                          "%s\\resources\\jre\\%s", g_payloadDir, fd.cFileName);
                ji->path[sizeof(ji->path) - 1] = 0;
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
}

/* ---------- file count + recursive copy with progress ---------- */

static DWORD g_totalFiles = 0;
static DWORD g_doneFiles  = 0;

static void CountFiles(const char *dir) {
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char pat[MAX_PATH], child[MAX_PATH];
    _snprintf(pat, sizeof(pat), "%s\\*", dir);
    pat[sizeof(pat) - 1] = 0;
    h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.cFileName[0] == '.') continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            _snprintf(child, sizeof(child), "%s\\%s", dir, fd.cFileName);
            child[sizeof(child) - 1] = 0;
            CountFiles(child);
        } else {
            g_totalFiles++;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

/* Recursive copy. Posts WM_INSTALL_PROGRESS / WM_INSTALL_FILE messages to UI.
   Returns 0 on cancel/failure. */
static int CopyTree(const char *srcDir, const char *dstDir) {
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char pat[MAX_PATH], srcChild[MAX_PATH], dstChild[MAX_PATH];

    if (!EnsureDirectory(dstDir)) {
        _snprintf(g_installError, sizeof(g_installError),
                  "Could not create %s", dstDir);
        return 0;
    }

    _snprintf(pat, sizeof(pat), "%s\\*", srcDir);
    pat[sizeof(pat) - 1] = 0;
    h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return 1;
    do {
        if (g_cancel) { FindClose(h); return 0; }
        if (fd.cFileName[0] == '.') continue;
        _snprintf(srcChild, sizeof(srcChild), "%s\\%s", srcDir, fd.cFileName);
        srcChild[sizeof(srcChild) - 1] = 0;
        _snprintf(dstChild, sizeof(dstChild), "%s\\%s", dstDir, fd.cFileName);
        dstChild[sizeof(dstChild) - 1] = 0;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!CopyTree(srcChild, dstChild)) { FindClose(h); return 0; }
        } else {
            PostMessageA(g_hwnd, WM_INSTALL_FILE, 0, (LPARAM)_strdup(dstChild));
            if (!CopyFileA(srcChild, dstChild, FALSE)) {
                _snprintf(g_installError, sizeof(g_installError),
                          "CopyFile failed: %s -> %s (err %lu)",
                          srcChild, dstChild, GetLastError());
                FindClose(h);
                return 0;
            }
            g_doneFiles++;
            if (g_totalFiles > 0) {
                int pct = (int)((g_doneFiles * 100ULL) / g_totalFiles);
                PostMessageA(g_hwnd, WM_INSTALL_PROGRESS, (WPARAM)pct, 0);
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return 1;
}

/* ---------- embedded payload ----------

   Build-installer.ps1 appends an archive blob after the PE image. Layout:

     entry[0..N-1]:
        u16 pathLen        path uses forward slashes, relative to install dir
        char path[pathLen] (no NUL)
        u32 fileSize       (bytes)
        u8 contents[fileSize]
     footer (last 24 bytes of file):
        u32 fileCount
        u32 payloadSizeLow
        u32 payloadSizeHigh
        char magic[12]     "MC9XSFXEND\0\0"

   Footer offsets let us locate archiveStart by walking back from EOF.
   Win9x-safe: 64-bit offsets via SetFilePointer's high-dword arg. */

#define SFX_FOOTER_SIZE 24
#define SFX_MAGIC       "MC9XSFXEND\0\0"

static int Detect64BitSub(DWORD aLo, DWORD aHi, DWORD bLo, DWORD bHi,
                          DWORD *outLo, DWORD *outHi) {
    if (aLo >= bLo) { *outLo = aLo - bLo; *outHi = aHi - bHi; }
    else            { *outLo = aLo - bLo; *outHi = aHi - bHi - 1; }
    return 1;
}

static int DetectEmbeddedPayload(void) {
    char selfPath[MAX_PATH];
    HANDLE h;
    DWORD sizeLow, sizeHigh = 0;
    DWORD startLow, startHigh;
    LONG  seekHi;
    DWORD got;
    unsigned char footer[SFX_FOOTER_SIZE];
    DWORD fileCount, payloadLow, payloadHigh;

    GetModuleFileNameA(NULL, selfPath, sizeof(selfPath));
    h = CreateFileA(selfPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;

    sizeLow = GetFileSize(h, &sizeHigh);
    if (sizeLow == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        CloseHandle(h); return 0;
    }

    /* Seek to EOF - 24 (the footer). */
    Detect64BitSub(sizeLow, sizeHigh, SFX_FOOTER_SIZE, 0, &startLow, &startHigh);
    seekHi = (LONG)startHigh;
    if (SetFilePointer(h, (LONG)startLow, &seekHi, FILE_BEGIN) == INVALID_SET_FILE_POINTER
        && GetLastError() != NO_ERROR) {
        CloseHandle(h); return 0;
    }
    if (!ReadFile(h, footer, SFX_FOOTER_SIZE, &got, NULL) || got != SFX_FOOTER_SIZE) {
        CloseHandle(h); return 0;
    }
    CloseHandle(h);

    if (memcmp(footer + 12, SFX_MAGIC, 12) != 0) return 0;

    fileCount   = (DWORD)footer[0]  | ((DWORD)footer[1] << 8)
                | ((DWORD)footer[2] << 16) | ((DWORD)footer[3] << 24);
    payloadLow  = (DWORD)footer[4]  | ((DWORD)footer[5] << 8)
                | ((DWORD)footer[6] << 16) | ((DWORD)footer[7] << 24);
    payloadHigh = (DWORD)footer[8]  | ((DWORD)footer[9] << 8)
                | ((DWORD)footer[10] << 16) | ((DWORD)footer[11] << 24);

    /* archiveStart = (sizeLow,sizeHigh) - SFX_FOOTER_SIZE - payload */
    Detect64BitSub(sizeLow, sizeHigh, SFX_FOOTER_SIZE, 0, &startLow, &startHigh);
    Detect64BitSub(startLow, startHigh, payloadLow, payloadHigh,
                   &g_embeddedArchiveLow, &g_embeddedArchiveHigh);
    g_embeddedFileCount = fileCount;
    return 1;
}

static int ExtractFromSelf(const char *installPath) {
    char selfPath[MAX_PATH];
    HANDLE h;
    LONG seekHi;
    DWORD got;
    DWORD i;

    GetModuleFileNameA(NULL, selfPath, sizeof(selfPath));
    h = CreateFileA(selfPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        CopyField(g_installError, sizeof(g_installError),
                  "Failed to open installer image for self-extraction.");
        return 0;
    }

    seekHi = (LONG)g_embeddedArchiveHigh;
    if (SetFilePointer(h, (LONG)g_embeddedArchiveLow, &seekHi, FILE_BEGIN)
            == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        CopyField(g_installError, sizeof(g_installError),
                  "Failed to seek to embedded archive.");
        CloseHandle(h); return 0;
    }

    if (!EnsureDirectory(installPath)) {
        _snprintf(g_installError, sizeof(g_installError),
                  "Could not create install directory: %s", installPath);
        CloseHandle(h); return 0;
    }

    for (i = 0; i < g_embeddedFileCount; i++) {
        unsigned char header[2];
        WORD pathLen;
        char relPath[1024];
        unsigned char sizeBuf[4];
        DWORD fileSize;
        char destPath[MAX_PATH];
        char parent[MAX_PATH];
        HANDLE hOut;
        DWORD remaining;
        char buf[16384];
        int j;
        char *bs;

        if (g_cancel) { CloseHandle(h); return 0; }

        if (!ReadFile(h, header, 2, &got, NULL) || got != 2) {
            CopyField(g_installError, sizeof(g_installError),
                      "Archive read error (header).");
            CloseHandle(h); return 0;
        }
        pathLen = (WORD)(header[0] | (header[1] << 8));
        if (pathLen == 0 || pathLen >= sizeof(relPath)) {
            CopyField(g_installError, sizeof(g_installError),
                      "Archive corrupt (bad path length).");
            CloseHandle(h); return 0;
        }
        if (!ReadFile(h, relPath, pathLen, &got, NULL) || got != pathLen) {
            CopyField(g_installError, sizeof(g_installError),
                      "Archive read error (path).");
            CloseHandle(h); return 0;
        }
        relPath[pathLen] = 0;
        if (!ReadFile(h, sizeBuf, 4, &got, NULL) || got != 4) {
            CopyField(g_installError, sizeof(g_installError),
                      "Archive read error (size).");
            CloseHandle(h); return 0;
        }
        fileSize = (DWORD)sizeBuf[0]
                 | ((DWORD)sizeBuf[1] << 8)
                 | ((DWORD)sizeBuf[2] << 16)
                 | ((DWORD)sizeBuf[3] << 24);

        for (j = 0; relPath[j]; j++) if (relPath[j] == '/') relPath[j] = '\\';
        _snprintf(destPath, sizeof(destPath), "%s\\%s", installPath, relPath);
        destPath[sizeof(destPath) - 1] = 0;

        CopyField(parent, sizeof(parent), destPath);
        bs = strrchr(parent, '\\');
        if (bs) { *bs = 0; EnsureDirectory(parent); }

        PostMessageA(g_hwnd, WM_INSTALL_FILE, 0, (LPARAM)_strdup(destPath));

        hOut = CreateFileA(destPath, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hOut == INVALID_HANDLE_VALUE) {
            /* Can't write here; skip the file's bytes and continue. */
            seekHi = 0;
            SetFilePointer(h, (LONG)fileSize, &seekHi, FILE_CURRENT);
        } else {
            remaining = fileSize;
            while (remaining > 0) {
                DWORD toRead = (remaining < sizeof(buf)) ? remaining : (DWORD)sizeof(buf);
                DWORD written;
                if (!ReadFile(h, buf, toRead, &got, NULL) || got == 0) {
                    CloseHandle(hOut); CloseHandle(h);
                    CopyField(g_installError, sizeof(g_installError),
                              "Archive read error (body).");
                    return 0;
                }
                WriteFile(hOut, buf, got, &written, NULL);
                remaining -= got;
                if (g_cancel) { CloseHandle(hOut); CloseHandle(h); return 0; }
            }
            CloseHandle(hOut);
        }

        g_doneFiles++;
        if (g_embeddedFileCount > 0) {
            int pct = (int)((g_doneFiles * 100ULL) / g_embeddedFileCount);
            PostMessageA(g_hwnd, WM_INSTALL_PROGRESS, (WPARAM)pct, 0);
        }
    }
    CloseHandle(h);
    return 1;
}

static DWORD WINAPI InstallWorker(LPVOID arg) {
    int ok;
    (void)arg;
    g_totalFiles = 0;
    g_doneFiles  = 0;
    if (g_haveEmbeddedPayload) {
        ok = ExtractFromSelf(g_installPath);
    } else {
        CountFiles(g_payloadDir);
        ok = CopyTree(g_payloadDir, g_installPath);
    }
    g_installResult = ok;
    PostMessageA(g_hwnd, ok ? WM_INSTALL_DONE : WM_INSTALL_FAILED, 0, 0);
    return 0;
}

/* ---------- shortcut creation via IShellLink ---------- */

static int CreateShortcut(const char *targetExe, const char *workDir,
                          const char *iconPath, const char *lnkPath) {
    IShellLinkA *psl = NULL;
    IPersistFile *ppf = NULL;
    HRESULT hr;
    int ok = 0;
    WCHAR wlnk[MAX_PATH];

    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          IID_IShellLinkA, (void **)&psl);
    if (FAILED(hr) || !psl) return 0;
    psl->SetPath(targetExe);
    psl->SetWorkingDirectory(workDir);
    if (iconPath && iconPath[0]) psl->SetIconLocation(iconPath, 0);

    hr = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
    if (SUCCEEDED(hr) && ppf) {
        MultiByteToWideChar(CP_ACP, 0, lnkPath, -1, wlnk, MAX_PATH);
        if (SUCCEEDED(ppf->Save(wlnk, TRUE))) ok = 1;
        ppf->Release();
    }
    psl->Release();
    return ok;
}

static void CreateShortcuts(void) {
    char target[MAX_PATH], icon[MAX_PATH];
    char startMenuDir[MAX_PATH], lnk[MAX_PATH];
    _snprintf(target, sizeof(target), "%s\\%s", g_installPath, APP_EXE);
    target[sizeof(target) - 1] = 0;
    _snprintf(icon, sizeof(icon), "%s\\resources\\minecraft.ico", g_installPath);
    icon[sizeof(icon) - 1] = 0;

    if (SHGetSpecialFolderPathA(NULL, startMenuDir, CSIDL_PROGRAMS, FALSE)) {
        _snprintf(lnk, sizeof(lnk), "%s\\%s", startMenuDir, APP_NAME);
        lnk[sizeof(lnk) - 1] = 0;
        CreateDirectoryA(lnk, NULL);
        _snprintf(lnk, sizeof(lnk), "%s\\%s\\%s.lnk",
                  startMenuDir, APP_NAME, APP_NAME);
        lnk[sizeof(lnk) - 1] = 0;
        CreateShortcut(target, g_installPath, icon, lnk);
    }
    if (SHGetSpecialFolderPathA(NULL, startMenuDir, CSIDL_DESKTOPDIRECTORY, FALSE)) {
        _snprintf(lnk, sizeof(lnk), "%s\\%s.lnk", startMenuDir, APP_NAME);
        lnk[sizeof(lnk) - 1] = 0;
        CreateShortcut(target, g_installPath, icon, lnk);
    }
}

/* ---------- page rendering ---------- */

static void ShowHide(HWND h, int show) {
    if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
}

static void SetButtons(const char *back, const char *next, int backEnabled, int nextEnabled) {
    if (back) { SetWindowTextA(g_hBack, back); ShowHide(g_hBack, 1); }
    else      { ShowHide(g_hBack, 0); }
    if (next) { SetWindowTextA(g_hNext, next); ShowHide(g_hNext, 1); }
    else      { ShowHide(g_hNext, 0); }
    EnableWindow(g_hBack, backEnabled);
    EnableWindow(g_hNext, nextEnabled);
}

static void HideAllPageControls(void) {
    HWND all[] = {
        g_hWelTitle, g_hWelBody,
        g_hLicHeader, g_hLicSubhead, g_hLicText, g_hLicAccept,
        g_hPathHeader, g_hPathSubhead, g_hPathLabel, g_hPathEdit,
        g_hPathBrowse, g_hPathSpace,
        g_hJavaHeader, g_hJavaSubhead, g_hJavaStatus, g_hJavaList,
        g_hJavaRefresh, g_hJavaNote,
        g_hInsHeader, g_hInsSubhead, g_hInsCurrent, g_hInsProgress, g_hInsLog,
        g_hFinTitle, g_hFinBody, g_hFinLaunch, g_hFinJava
    };
    int i;
    for (i = 0; i < (int)(sizeof(all)/sizeof(all[0])); i++) ShowHide(all[i], 0);
}

static void RefreshJavaList(void);

static void EnterPage(int page) {
    HideAllPageControls();
    g_currentPage = page;
    switch (page) {
    case PAGE_WELCOME:
        ShowHide(g_hWelTitle, 1); ShowHide(g_hWelBody, 1);
        SetButtons("< &Back", "&Next >", 0, 1);
        break;
    case PAGE_LICENSE:
        ShowHide(g_hLicHeader, 1); ShowHide(g_hLicSubhead, 1);
        ShowHide(g_hLicText, 1);   ShowHide(g_hLicAccept, 1);
        SetButtons("< &Back", "&Next >", 1, g_licenseAccepted);
        break;
    case PAGE_PATH:
        ShowHide(g_hPathHeader, 1); ShowHide(g_hPathSubhead, 1);
        ShowHide(g_hPathLabel, 1);  ShowHide(g_hPathEdit, 1);
        ShowHide(g_hPathBrowse, 1); ShowHide(g_hPathSpace, 1);
        SetButtons("< &Back", "&Next >", 1, 1);
        break;
    case PAGE_JAVA:
        ShowHide(g_hJavaHeader, 1); ShowHide(g_hJavaSubhead, 1);
        ShowHide(g_hJavaStatus, 1); ShowHide(g_hJavaList, 1);
        ShowHide(g_hJavaRefresh, 1); ShowHide(g_hJavaNote, 1);
        SetButtons("< &Back", "&Install", 1, 1);
        RefreshJavaList();
        break;
    case PAGE_INSTALL:
        ShowHide(g_hInsHeader, 1); ShowHide(g_hInsSubhead, 1);
        ShowHide(g_hInsCurrent, 1); ShowHide(g_hInsProgress, 1);
        ShowHide(g_hInsLog, 1);
        SetButtons("< &Back", "&Next >", 0, 0);
        SetWindowTextA(g_hCancel, "&Cancel");
        /* start worker */
        SendMessageA(g_hInsProgress, PBM_SETPOS, 0, 0);
        SetWindowTextA(g_hInsCurrent, "Preparing...");
        SetWindowTextA(g_hInsLog, "");
        g_cancel = 0;
        g_hWorker = CreateThread(NULL, 0, InstallWorker, NULL, 0, NULL);
        break;
    case PAGE_FINISH:
        ShowHide(g_hFinTitle, 1); ShowHide(g_hFinBody, 1);
        ShowHide(g_hFinLaunch, 1);
        ShowHide(g_hFinJava, g_chosenJre[0] ? 1 : 0);
        SetButtons(NULL, "&Finish", 0, 1);
        SetWindowTextA(g_hCancel, "&Cancel");
        EnableWindow(g_hCancel, FALSE);
        break;
    }
    InvalidateRect(g_hwnd, NULL, TRUE);
}

static void RefreshJavaList(void) {
    int i;
    SendMessageA(g_hJavaList, LB_RESETCONTENT, 0, 0);
    DiscoverJavas();
    if (g_have32Java) {
        char st[160];
        _snprintf(st, sizeof(st),
                  "Found %d 32-bit Java install%s on this system.",
                  g_javaCount, g_javaCount == 1 ? "" : "s");
        SetWindowTextA(g_hJavaStatus, st);
        SetWindowTextA(g_hJavaNote,
            "Java is already installed - no action needed. Click Install to continue.");
        for (i = 0; i < g_javaCount; i++)
            SendMessageA(g_hJavaList, LB_ADDSTRING, 0, (LPARAM)g_javas[i].displayName);
        EnableWindow(g_hJavaList, FALSE);
        g_chosenJre[0] = 0;
    } else {
        SetWindowTextA(g_hJavaStatus,
            "No 32-bit Java detected. Pick one of the bundled installers to "
            "run after the launcher is installed:");
        SetWindowTextA(g_hJavaNote,
            "Tip: 8u51 works for all listed Minecraft versions and is the safe default.");
        EnableWindow(g_hJavaList, TRUE);
        DiscoverBundledJres();
        for (i = 0; i < g_jreInstallerCount; i++)
            SendMessageA(g_hJavaList, LB_ADDSTRING, 0, (LPARAM)g_jreInstallers[i].name);
        if (g_jreInstallerCount > 0) {
            /* Pre-select 8u51 if present, else first. */
            int sel = 0, j;
            for (j = 0; j < g_jreInstallerCount; j++) {
                if (strstr(g_jreInstallers[j].name, "8u")) { sel = j; break; }
            }
            SendMessageA(g_hJavaList, LB_SETCURSEL, (WPARAM)sel, 0);
            CopyField(g_chosenJre, sizeof(g_chosenJre), g_jreInstallers[sel].path);
        }
    }
}

/* ---------- WM_PAINT: background + header strip + sidebar ---------- */

static void PaintPage(HDC hdc) {
    RECT cli;
    HBRUSH wbr;
    GetClientRect(g_hwnd, &cli);
    wbr = (HBRUSH)GetStockObject(WHITE_BRUSH);
    FillRect(hdc, &cli, wbr);

    if (g_currentPage == PAGE_WELCOME || g_currentPage == PAGE_FINISH) {
        int sidebarH = BUTTON_Y - 5;

        /* Center-crop the source bitmap to the sidebar's aspect ratio so
           the artwork isn't squashed. background.bmp is 1000x1000 square;
           we crop a vertical strip whose width matches the sidebar's
           aspect (164x340 → ~482x1000 source rect) and StretchBlt that. */
        if (g_hSidebar) {
            HDC mdc = CreateCompatibleDC(hdc);
            HBITMAP old = (HBITMAP)SelectObject(mdc, g_hSidebar);
            BITMAP bm;
            int srcCropW, srcX;
            GetObject(g_hSidebar, sizeof(bm), &bm);
            srcCropW = (bm.bmHeight * SIDEBAR_W) / sidebarH;
            if (srcCropW > bm.bmWidth) srcCropW = bm.bmWidth;
            srcX = (bm.bmWidth - srcCropW) / 2;
            SetStretchBltMode(hdc, HALFTONE);
            SetBrushOrgEx(hdc, 0, 0, NULL);
            StretchBlt(hdc, 0, 0, SIDEBAR_W, sidebarH,
                       mdc, srcX, 0, srcCropW, bm.bmHeight, SRCCOPY);
            SelectObject(mdc, old);
            DeleteDC(mdc);
        } else {
            RECT side = { 0, 0, SIDEBAR_W, sidebarH };
            HBRUSH dark = CreateSolidBrush(RGB(28, 40, 24));
            FillRect(hdc, &side, dark);
            DeleteObject(dark);
        }

        /* Dark gradient-style band along the bottom of the sidebar so the
           "Minecraft9xLauncher" text reads against the artwork. */
        {
            int bandTop = sidebarH - 56;
            int y;
            for (y = bandTop; y < sidebarH; y++) {
                int alpha = ((y - bandTop) * 255) / 55;
                int v = (200 - alpha) / 5;  /* fade from translucent to dark */
                HPEN pen = CreatePen(PS_SOLID, 1, RGB(v, v + 4, v));
                HPEN oldPen = (HPEN)SelectObject(hdc, pen);
                MoveToEx(hdc, 0, y, NULL);
                LineTo(hdc, SIDEBAR_W, y);
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
            }
        }

        /* 128x128 minecraft icon centered horizontally, near the top. */
        if (g_hAppIcon) {
            int iconSize = 128;
            int ix = (SIDEBAR_W - iconSize) / 2;
            int iy = 28;
            DrawIconEx(hdc, ix, iy, g_hAppIcon, iconSize, iconSize,
                       0, NULL, DI_NORMAL);
        }

        /* Brand text over the dark band at the bottom. */
        {
            RECT t1 = { 0, sidebarH - 50, SIDEBAR_W, sidebarH - 30 };
            RECT t2 = { 0, sidebarH - 28, SIDEBAR_W, sidebarH - 10 };
            HFONT oldFont;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            oldFont = (HFONT)SelectObject(hdc, g_hFontTitle);
            DrawTextA(hdc, "Minecraft9x", -1, &t1, DT_CENTER | DT_SINGLELINE);
            SelectObject(hdc, g_hFontBold);
            SetTextColor(hdc, RGB(220, 220, 220));
            DrawTextA(hdc, "Launcher Setup", -1, &t2, DT_CENTER | DT_SINGLELINE);
            SelectObject(hdc, oldFont);
        }
    } else {
        /* draw header strip across the top */
        RECT hdr = { 0, 0, CLIENT_W, HEADER_H };
        FillRect(hdc, &hdr, wbr);
        /* bottom border line under header */
        {
            RECT line = { 0, HEADER_H, CLIENT_W, HEADER_H + 1 };
            HBRUSH gray = (HBRUSH)GetSysColorBrush(COLOR_3DSHADOW);
            FillRect(hdc, &line, gray);
        }
    }

    /* horizontal divider above the button row */
    {
        RECT line = { 0, BUTTON_Y - 12, CLIENT_W, BUTTON_Y - 11 };
        HBRUSH gray = (HBRUSH)GetSysColorBrush(COLOR_3DSHADOW);
        FillRect(hdc, &line, gray);
    }
}

/* ---------- WindowProc ---------- */

static BOOL BrowseForFolder(HWND owner, char *out, size_t cap) {
    BROWSEINFOA bi;
    LPITEMIDLIST pidl;
    char path[MAX_PATH];
    memset(&bi, 0, sizeof(bi));
    bi.hwndOwner = owner;
    bi.pszDisplayName = path;
    bi.lpszTitle = "Pick an install folder for " APP_NAME;
    bi.ulFlags   = BIF_RETURNONLYFSDIRS;
    pidl = SHBrowseForFolderA(&bi);
    if (!pidl) return FALSE;
    if (SHGetPathFromIDListA(pidl, path)) {
        CopyField(out, cap, path);
        return TRUE;
    }
    return FALSE;
}

static int ValidatePath(const char *p) {
    /* Minimal: must start with X:\ or \\ and be non-empty. */
    if (!p || !p[0]) return 0;
    if (p[1] == ':' && p[2] == '\\') return 1;
    if (p[0] == '\\' && p[1] == '\\') return 1;
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        if (g_currentPage == PAGE_INSTALL) {
            if (MessageBoxA(hwnd, "Cancel installation? Files copied so far will be left in place.",
                            "Cancel", MB_ICONQUESTION | MB_YESNO) != IDYES) return 0;
            g_cancel = 1;
            return 0;
        }
        if (g_currentPage != PAGE_FINISH) {
            if (MessageBoxA(hwnd, "Exit setup?", APP_NAME " Setup",
                            MB_ICONQUESTION | MB_YESNO) != IDYES) return 0;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (g_hSidebar)   DeleteObject(g_hSidebar);
        if (g_hAppIcon)   DestroyIcon(g_hAppIcon);
        if (g_hFont)      DeleteObject(g_hFont);
        if (g_hFontBold)  DeleteObject(g_hFontBold);
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintPage(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtl = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);
        if (hCtl == g_hWelTitle || hCtl == g_hFinTitle) {
            SetTextColor(hdc, RGB(0, 0, 0));
        }
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        WORD notif = HIWORD(wParam);

        if (id == ID_CANCEL) {
            SendMessageA(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        if (id == ID_BACK) {
            if (g_currentPage > 0 && g_currentPage != PAGE_INSTALL && g_currentPage != PAGE_FINISH)
                EnterPage(g_currentPage - 1);
            return 0;
        }
        if (id == ID_NEXT) {
            switch (g_currentPage) {
            case PAGE_WELCOME: EnterPage(PAGE_LICENSE); break;
            case PAGE_LICENSE:
                if (!g_licenseAccepted) {
                    MessageBoxA(hwnd, "You must accept the license to continue.",
                                APP_NAME " Setup", MB_ICONINFORMATION | MB_OK);
                    return 0;
                }
                EnterPage(PAGE_PATH); break;
            case PAGE_PATH:
                GetWindowTextA(g_hPathEdit, g_installPath, sizeof(g_installPath));
                if (!ValidatePath(g_installPath)) {
                    MessageBoxA(hwnd,
                        "Install path must be an absolute path like C:\\Minecraft9xLauncher.",
                        APP_NAME " Setup", MB_ICONEXCLAMATION | MB_OK);
                    return 0;
                }
                EnterPage(PAGE_JAVA); break;
            case PAGE_JAVA:
                if (!g_have32Java) {
                    int sel = (int)SendMessageA(g_hJavaList, LB_GETCURSEL, 0, 0);
                    if (sel >= 0 && sel < g_jreInstallerCount)
                        CopyField(g_chosenJre, sizeof(g_chosenJre),
                                  g_jreInstallers[sel].path);
                }
                EnterPage(PAGE_INSTALL); break;
            case PAGE_FINISH:
                /* finish button = OK */
                {
                    int launchNow = (SendMessageA(g_hFinLaunch, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    int runJava   = g_chosenJre[0] &&
                                    (SendMessageA(g_hFinJava, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (runJava) {
                        char target[MAX_PATH];
                        const char *base = strrchr(g_chosenJre, '\\');
                        base = base ? base + 1 : g_chosenJre;
                        _snprintf(target, sizeof(target),
                                  "%s\\resources\\jre\\%s", g_installPath, base);
                        target[sizeof(target) - 1] = 0;
                        ShellExecuteA(hwnd, "open", target, NULL, NULL, SW_SHOWNORMAL);
                    }
                    if (launchNow) {
                        char target[MAX_PATH];
                        _snprintf(target, sizeof(target),
                                  "%s\\%s", g_installPath, APP_EXE);
                        target[sizeof(target) - 1] = 0;
                        ShellExecuteA(hwnd, "open", target, NULL,
                                      g_installPath, SW_SHOWNORMAL);
                    }
                }
                DestroyWindow(hwnd);
                break;
            }
            return 0;
        }
        if (id == ID_LICENSE_ACCEPT && notif == BN_CLICKED) {
            g_licenseAccepted = (SendMessageA(g_hLicAccept, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(g_hNext, g_licenseAccepted);
            return 0;
        }
        if (id == ID_PATH_BROWSE) {
            char picked[MAX_PATH];
            if (BrowseForFolder(hwnd, picked, sizeof(picked))) {
                /* If user picked a folder, suffix our app name unless already there. */
                if (!strstr(picked, APP_NAME)) {
                    size_t n = strlen(picked);
                    if (n + 1 + strlen(APP_NAME) + 1 < sizeof(picked)) {
                        if (n > 0 && picked[n - 1] != '\\') strcat(picked, "\\");
                        strcat(picked, APP_NAME);
                    }
                }
                SetWindowTextA(g_hPathEdit, picked);
            }
            return 0;
        }
        if (id == ID_JAVA_REFRESH) {
            RefreshJavaList();
            return 0;
        }
        return 0;
    }

    case WM_INSTALL_PROGRESS: {
        int pct = (int)wParam;
        SendMessageA(g_hInsProgress, PBM_SETPOS, (WPARAM)pct, 0);
        return 0;
    }

    case WM_INSTALL_FILE: {
        char *path = (char *)lParam;
        if (path) {
            const char *base = strrchr(path, '\\');
            base = base ? base + 1 : path;
            SetWindowTextA(g_hInsCurrent, base);
            /* append last filename to log */
            {
                int len = GetWindowTextLengthA(g_hInsLog);
                SendMessageA(g_hInsLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
                SendMessageA(g_hInsLog, EM_REPLACESEL, 0, (LPARAM)path);
                SendMessageA(g_hInsLog, EM_REPLACESEL, 0, (LPARAM)"\r\n");
            }
            free(path);
        }
        return 0;
    }

    case WM_INSTALL_DONE: {
        if (g_hWorker) { CloseHandle(g_hWorker); g_hWorker = NULL; }
        SendMessageA(g_hInsProgress, PBM_SETPOS, 100, 0);
        SetWindowTextA(g_hInsCurrent, "Creating shortcuts...");
        CreateShortcuts();
        EnterPage(PAGE_FINISH);
        return 0;
    }

    case WM_INSTALL_FAILED: {
        if (g_hWorker) { CloseHandle(g_hWorker); g_hWorker = NULL; }
        MessageBoxA(hwnd,
            g_installError[0] ? g_installError : "Install failed.",
            APP_NAME " Setup", MB_ICONERROR | MB_OK);
        EnableWindow(g_hCancel, TRUE);
        SetButtons("< &Back", "&Retry", 1, 1);
        return 0;
    }

    default:
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
}

/* ---------- WinMain ---------- */

static HFONT MakeFont(int height, int bold) {
    LOGFONTA lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfHeight  = -height;
    lf.lfWeight  = bold ? FW_BOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    strcpy(lf.lfFaceName, "MS Shell Dlg");
    return CreateFontIndirectA(&lf);
}

/* Convert bare LF to CRLF so the EDIT control renders line breaks. */
static char *ConvertLfToCrLf(const char *src, DWORD srcLen, DWORD *outLen) {
    DWORD i, j, extra = 0;
    char *out;
    for (i = 0; i < srcLen; i++) {
        if (src[i] == '\n' && (i == 0 || src[i - 1] != '\r')) extra++;
    }
    out = (char *)malloc(srcLen + extra + 1);
    if (!out) return NULL;
    for (i = 0, j = 0; i < srcLen; i++) {
        if (src[i] == '\n' && (i == 0 || src[i - 1] != '\r')) out[j++] = '\r';
        out[j++] = src[i];
    }
    out[j] = 0;
    if (outLen) *outLen = j;
    return out;
}

static void LoadLicenseInto(HWND hEdit) {
    HRSRC  hRes;
    HGLOBAL hGlobal;
    const char *pData;
    DWORD  size;
    char  *converted;

    hRes = FindResourceA(g_hInst, "LICENSE", RT_RCDATA);
    if (!hRes) {
        SetWindowTextA(hEdit, "(LICENSE resource missing from installer)");
        return;
    }
    hGlobal = LoadResource(g_hInst, hRes);
    pData   = (const char *)LockResource(hGlobal);
    size    = SizeofResource(g_hInst, hRes);
    if (!pData || size == 0) {
        SetWindowTextA(hEdit, "(LICENSE resource empty)");
        return;
    }
    converted = ConvertLfToCrLf(pData, size, NULL);
    if (converted) {
        SetWindowTextA(hEdit, converted);
        free(converted);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR cmdLine, int nShow) {
    WNDCLASSA wc;
    MSG msg;
    INITCOMMONCONTROLSEX icc;
    char title[128];

    (void)hPrev; (void)cmdLine;
    g_hInst = hInstance;

    CoInitialize(NULL);

    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icc);

    GetExeDir(g_exeDir, sizeof(g_exeDir));
    _snprintf(g_payloadDir, sizeof(g_payloadDir), "%s\\%s", g_exeDir, DATA_SUBDIR);
    g_payloadDir[sizeof(g_payloadDir) - 1] = 0;

    /* Source-of-files priority:
       1. Embedded payload appended to this exe (production build)
       2. data\ folder next to the exe         (dev build / unpacked) */
    g_haveEmbeddedPayload = DetectEmbeddedPayload();
    g_havePayloadDir      = PathExists(g_payloadDir);

    if (!g_haveEmbeddedPayload && !g_havePayloadDir) {
        MessageBoxA(NULL,
            "This installer has no payload.\n\n"
            "Either it wasn't packed correctly during build, or it was "
            "run as a dev exe without a sibling 'data\\' folder.",
            APP_NAME " Setup", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    /* SIDEBAR bitmap and MAINICON live as PE resources, so the installer
       can show its own welcome page art before anything is extracted. */
    g_hSidebar = LoadBitmapA(hInstance, "SIDEBAR");
    g_hAppIcon = (HICON)LoadImageA(hInstance, "MAINICON", IMAGE_ICON,
                                   128, 128, LR_DEFAULTCOLOR);
    if (!g_hAppIcon) g_hAppIcon = LoadIconA(hInstance, "MAINICON");

    g_hFont      = MakeFont(13, 0);
    g_hFontBold  = MakeFont(13, 1);
    g_hFontTitle = MakeFont(20, 1);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIconA(hInstance, "MAINICON");
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = APP_NAME "Setup";
    RegisterClassA(&wc);

    _snprintf(title, sizeof(title), "%s Setup", APP_NAME);
    g_hwnd = CreateWindowExA(0, APP_NAME "Setup", title,
        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX) | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, WIN_W, WIN_H,
        NULL, NULL, hInstance, NULL);
    if (!g_hwnd) { MessageBoxA(NULL, "Window creation failed.", title, MB_OK); return 1; }

    /* ---- bottom buttons ---- */
    g_hBack = CreateWindowA("BUTTON", "< &Back",
        WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        CLIENT_W - 3 * (BUTTON_W + 8), BUTTON_Y, BUTTON_W, BUTTON_H,
        g_hwnd, (HMENU)ID_BACK, hInstance, NULL);
    g_hNext = CreateWindowA("BUTTON", "&Next >",
        WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        CLIENT_W - 2 * (BUTTON_W + 8), BUTTON_Y, BUTTON_W, BUTTON_H,
        g_hwnd, (HMENU)ID_NEXT, hInstance, NULL);
    g_hCancel = CreateWindowA("BUTTON", "&Cancel",
        WS_TABSTOP | WS_CHILD | WS_VISIBLE,
        CLIENT_W - 1 * (BUTTON_W + 8), BUTTON_Y, BUTTON_W, BUTTON_H,
        g_hwnd, (HMENU)ID_CANCEL, hInstance, NULL);

    /* ---- welcome page ---- */
    g_hWelTitle = CreateWindowA("STATIC",
        "Welcome to the\n" APP_NAME "\nSetup Wizard",
        WS_CHILD | SS_LEFT,
        SIDEBAR_W + 16, 20, CLIENT_W - SIDEBAR_W - 30, 80,
        g_hwnd, NULL, hInstance, NULL);
    g_hWelBody = CreateWindowA("STATIC",
        "This will install " APP_NAME " v" INSTALLER_VERSION " on your computer.\r\n\r\n"
        "The launcher is a native Win32 application that lets you run Minecraft "
        "(alpha through 1.12.2 + Forge) on Windows 95 and later. It works with "
        "32-bit Java only.\r\n\r\n"
        "Click Next to continue, or Cancel to exit.",
        WS_CHILD | SS_LEFT,
        SIDEBAR_W + 16, 120, CLIENT_W - SIDEBAR_W - 30, 180,
        g_hwnd, NULL, hInstance, NULL);

    /* ---- license page ---- */
    g_hLicHeader = CreateWindowA("STATIC", "License Agreement",
        WS_CHILD | SS_LEFT,
        18, 10, CLIENT_W - 36, 18, g_hwnd, NULL, hInstance, NULL);
    g_hLicSubhead = CreateWindowA("STATIC",
        "Please read the following license carefully.",
        WS_CHILD | SS_LEFT,
        30, 32, CLIENT_W - 60, 18, g_hwnd, NULL, hInstance, NULL);
    g_hLicText = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY,
        18, HEADER_H + 12, CLIENT_W - 36, 200,
        g_hwnd, NULL, hInstance, NULL);
    g_hLicAccept = CreateWindowA("BUTTON",
        "I &accept the terms of the license agreement",
        WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
        18, HEADER_H + 220, CLIENT_W - 36, 20,
        g_hwnd, (HMENU)ID_LICENSE_ACCEPT, hInstance, NULL);
    LoadLicenseInto(g_hLicText);

    /* ---- path page ---- */
    g_hPathHeader = CreateWindowA("STATIC", "Install Location",
        WS_CHILD | SS_LEFT,
        18, 10, CLIENT_W - 36, 18, g_hwnd, NULL, hInstance, NULL);
    g_hPathSubhead = CreateWindowA("STATIC",
        "Choose where to install " APP_NAME ".",
        WS_CHILD | SS_LEFT,
        30, 32, CLIENT_W - 60, 18, g_hwnd, NULL, hInstance, NULL);
    g_hPathLabel = CreateWindowA("STATIC",
        "Destination folder:",
        WS_CHILD | SS_LEFT,
        18, HEADER_H + 20, CLIENT_W - 36, 18, g_hwnd, NULL, hInstance, NULL);
    g_hPathEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", g_installPath,
        WS_CHILD | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
        18, HEADER_H + 42, CLIENT_W - 36 - 90, 22,
        g_hwnd, (HMENU)ID_PATH_EDIT, hInstance, NULL);
    g_hPathBrowse = CreateWindowA("BUTTON", "B&rowse...",
        WS_CHILD | WS_TABSTOP,
        CLIENT_W - 18 - 80, HEADER_H + 42, 80, 22,
        g_hwnd, (HMENU)ID_PATH_BROWSE, hInstance, NULL);
    g_hPathSpace = CreateWindowA("STATIC",
        "Recommended: a folder outside Program Files so the OpenGL mode "
        "switcher can write into the JRE folder without administrator rights.",
        WS_CHILD | SS_LEFT,
        18, HEADER_H + 80, CLIENT_W - 36, 60,
        g_hwnd, NULL, hInstance, NULL);

    /* ---- java page ---- */
    g_hJavaHeader = CreateWindowA("STATIC", "Java Runtime",
        WS_CHILD | SS_LEFT,
        18, 10, CLIENT_W - 36, 18, g_hwnd, NULL, hInstance, NULL);
    g_hJavaSubhead = CreateWindowA("STATIC",
        APP_NAME " requires a 32-bit Java Runtime to launch Minecraft.",
        WS_CHILD | SS_LEFT,
        30, 32, CLIENT_W - 60, 18, g_hwnd, NULL, hInstance, NULL);
    g_hJavaStatus = CreateWindowA("STATIC", "Scanning...",
        WS_CHILD | SS_LEFT,
        18, HEADER_H + 16, CLIENT_W - 36, 36,
        g_hwnd, NULL, hInstance, NULL);
    g_hJavaList = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
        18, HEADER_H + 56, CLIENT_W - 36 - 90, 130,
        g_hwnd, (HMENU)ID_JAVA_LIST, hInstance, NULL);
    g_hJavaRefresh = CreateWindowA("BUTTON", "Re&scan",
        WS_CHILD | WS_TABSTOP,
        CLIENT_W - 18 - 80, HEADER_H + 56, 80, 22,
        g_hwnd, (HMENU)ID_JAVA_REFRESH, hInstance, NULL);
    g_hJavaNote = CreateWindowA("STATIC", "",
        WS_CHILD | SS_LEFT,
        18, HEADER_H + 192, CLIENT_W - 36, 40,
        g_hwnd, NULL, hInstance, NULL);

    /* ---- install (progress) page ---- */
    g_hInsHeader = CreateWindowA("STATIC", "Installing",
        WS_CHILD | SS_LEFT,
        18, 10, CLIENT_W - 36, 18, g_hwnd, NULL, hInstance, NULL);
    g_hInsSubhead = CreateWindowA("STATIC",
        "Copying files. This may take a few minutes.",
        WS_CHILD | SS_LEFT,
        30, 32, CLIENT_W - 60, 18, g_hwnd, NULL, hInstance, NULL);
    g_hInsCurrent = CreateWindowA("STATIC", "",
        WS_CHILD | SS_LEFT | SS_ENDELLIPSIS,
        18, HEADER_H + 16, CLIENT_W - 36, 18,
        g_hwnd, NULL, hInstance, NULL);
    g_hInsProgress = CreateWindowA(PROGRESS_CLASS, "",
        WS_CHILD | PBS_SMOOTH,
        18, HEADER_H + 38, CLIENT_W - 36, 18,
        g_hwnd, NULL, hInstance, NULL);
    SendMessageA(g_hInsProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    g_hInsLog = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        18, HEADER_H + 70, CLIENT_W - 36, 160,
        g_hwnd, NULL, hInstance, NULL);

    /* ---- finish page ---- */
    g_hFinTitle = CreateWindowA("STATIC",
        "Installation Complete",
        WS_CHILD | SS_LEFT,
        SIDEBAR_W + 16, 20, CLIENT_W - SIDEBAR_W - 30, 30,
        g_hwnd, NULL, hInstance, NULL);
    g_hFinBody = CreateWindowA("STATIC",
        APP_NAME " has been installed.\r\n\r\n"
        "Click Finish to close this wizard. If you ticked a Java installer above, "
        "it will start now - finish that wizard before launching Minecraft.",
        WS_CHILD | SS_LEFT,
        SIDEBAR_W + 16, 70, CLIENT_W - SIDEBAR_W - 30, 120,
        g_hwnd, NULL, hInstance, NULL);
    g_hFinLaunch = CreateWindowA("BUTTON", "&Launch " APP_NAME " now",
        WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
        SIDEBAR_W + 16, 200, CLIENT_W - SIDEBAR_W - 30, 20,
        g_hwnd, (HMENU)ID_FINISH_LAUNCH, hInstance, NULL);
    SendMessageA(g_hFinLaunch, BM_SETCHECK, BST_CHECKED, 0);
    g_hFinJava = CreateWindowA("BUTTON", "&Run the selected Java installer",
        WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
        SIDEBAR_W + 16, 225, CLIENT_W - SIDEBAR_W - 30, 20,
        g_hwnd, (HMENU)ID_FINISH_JAVA, hInstance, NULL);
    SendMessageA(g_hFinJava, BM_SETCHECK, BST_CHECKED, 0);

    /* fonts */
    {
        HWND all[] = {
            g_hBack, g_hNext, g_hCancel,
            g_hWelBody,
            g_hLicHeader, g_hLicSubhead, g_hLicText, g_hLicAccept,
            g_hPathHeader, g_hPathSubhead, g_hPathLabel, g_hPathEdit,
            g_hPathBrowse, g_hPathSpace,
            g_hJavaHeader, g_hJavaSubhead, g_hJavaStatus, g_hJavaList,
            g_hJavaRefresh, g_hJavaNote,
            g_hInsHeader, g_hInsSubhead, g_hInsCurrent, g_hInsLog,
            g_hFinBody, g_hFinLaunch, g_hFinJava
        };
        int i;
        for (i = 0; i < (int)(sizeof(all)/sizeof(all[0])); i++)
            SendMessageA(all[i], WM_SETFONT, (WPARAM)g_hFont, 0);
        SendMessageA(g_hWelTitle, WM_SETFONT, (WPARAM)g_hFontTitle, 0);
        SendMessageA(g_hFinTitle, WM_SETFONT, (WPARAM)g_hFontTitle, 0);
        SendMessageA(g_hLicHeader,  WM_SETFONT, (WPARAM)g_hFontBold, 0);
        SendMessageA(g_hPathHeader, WM_SETFONT, (WPARAM)g_hFontBold, 0);
        SendMessageA(g_hJavaHeader, WM_SETFONT, (WPARAM)g_hFontBold, 0);
        SendMessageA(g_hInsHeader,  WM_SETFONT, (WPARAM)g_hFontBold, 0);
    }

    EnterPage(PAGE_WELCOME);
    ShowWindow(g_hwnd, nShow);
    UpdateWindow(g_hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CoUninitialize();
    return (int)msg.wParam;
}
