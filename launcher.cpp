#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LAUNCHER_VERSION "1.6.0"
#define MAX_CLASSPATH    16384
#define MAX_LAUNCH_CMD   32768
#define ID_BTN_PLAY          1
#define ID_CHK_NOSOUND       2
#define ID_BTN_INSTALL_JAVA  3
#define ID_MENU_INSTALL_BASE 0x4000
#define MAX_GL_MODES         16
#define MAX_JAVAS            16
#define MAX_JRE_INSTALLERS   16

static HBITMAP g_hBitmap = NULL;
static HFONT   g_hFont   = NULL;
static HWND    g_hwnd;
static HWND    g_hButtonPlay, g_hCheckNoSound;
static HWND    g_hDropdownVersion, g_hTextboxUsername, g_hTextboxRamMax, g_hTextboxRamMin;
static HWND    g_hDropdownGl;
static HWND    g_hDropdownJava, g_hButtonInstallJava;
static HWND    g_hLabelInfo, g_hLabelVersion, g_hLabelUsername;
static HWND    g_hLabelRamMax, g_hLabelRamMin, g_hLabelGl, g_hLabelJava, g_hLabelStatus;

static char g_cfgVersion[64]   = "beta1.7.3";
static char g_cfgUsername[32]  = "Player";
static char g_cfgRamMaxMb[8]   = "512";
static char g_cfgRamMinMb[8]   = "256";
static char g_cfgJava[260]     = "java";
static int  g_cfgNoSound       = 0;
static char g_cfgGlMode[64]    = "default";

typedef struct {
    char name[64];           /* shown in dropdown */
    char id[64];             /* persisted in settings */
    char dirPath[MAX_PATH];  /* resources\gl\<id>\ to mirror into version dir, or "" */
    int  setLwjglFallback;   /* 1 to append -Dorg.lwjgl...allowSoftwareOpenGL=true */
} GlMode;

static GlMode g_glModes[MAX_GL_MODES];
static int    g_glModeCount = 0;

typedef struct {
    char displayName[160];   /* shown in dropdown */
    char path[MAX_PATH];     /* full path to java.exe */
} JavaInstall;

static JavaInstall g_javas[MAX_JAVAS];
static int         g_javaCount = 0;

typedef struct {
    char name[64];           /* file name shown in menu */
    char path[MAX_PATH];     /* full path passed to ShellExecute */
} JreInstaller;

static JreInstaller g_jreInstallers[MAX_JRE_INSTALLERS];
static int          g_jreInstallerCount = 0;

static const char *kVersions[] = {
    "1.12.2forge", "1.8.9", "1.7.10", "1.6.4",
    "1.5.2", "1.4.7", "1.2.5", "1.0", "beta1.7.3", "betahacks", "alpha1.1.2"
};
static const int kVersionCount = sizeof(kVersions) / sizeof(kVersions[0]);

static void LogError(const char *msg) {
    FILE *fp = fopen("log.txt", "a");
    if (fp) { fprintf(fp, "Error: %s\n", msg); fclose(fp); }
}

/* ---------- MD5 (RFC 1321) ---------- */

typedef struct { unsigned int s[4]; unsigned int count[2]; unsigned char buf[64]; } MD5_CTX;

static const unsigned int MD5_K[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};
static const unsigned int MD5_R[64] = {
    7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
    5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
    4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
    6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
};

#define ROTL32(x,n) (((x) << (n)) | ((x) >> (32 - (n))))

static void md5_transform(MD5_CTX *ctx, const unsigned char *block) {
    unsigned int M[16];
    int i;
    for (i = 0; i < 16; i++) {
        M[i] =  (unsigned int)block[i*4]
             | ((unsigned int)block[i*4+1] << 8)
             | ((unsigned int)block[i*4+2] << 16)
             | ((unsigned int)block[i*4+3] << 24);
    }
    unsigned int a = ctx->s[0], b = ctx->s[1], c = ctx->s[2], d = ctx->s[3];
    for (i = 0; i < 64; i++) {
        unsigned int f, g, t;
        if (i < 16)      { f = (b & c) | (~b & d); g = i; }
        else if (i < 32) { f = (d & b) | (~d & c); g = (5*i + 1) & 15; }
        else if (i < 48) { f = b ^ c ^ d;          g = (3*i + 5) & 15; }
        else             { f = c ^ (b | ~d);       g = (7*i) & 15; }
        t = d; d = c; c = b;
        b = b + ROTL32(a + f + MD5_K[i] + M[g], MD5_R[i]);
        a = t;
    }
    ctx->s[0] += a; ctx->s[1] += b; ctx->s[2] += c; ctx->s[3] += d;
}

static void md5_init(MD5_CTX *ctx) {
    ctx->s[0] = 0x67452301; ctx->s[1] = 0xefcdab89;
    ctx->s[2] = 0x98badcfe; ctx->s[3] = 0x10325476;
    ctx->count[0] = ctx->count[1] = 0;
}

static void md5_update(MD5_CTX *ctx, const unsigned char *data, unsigned int len) {
    unsigned int idx  = (ctx->count[0] >> 3) & 0x3F;
    unsigned int part = 64 - idx;
    unsigned int i    = 0;
    if ((ctx->count[0] += len << 3) < (len << 3)) ctx->count[1]++;
    ctx->count[1] += len >> 29;
    if (len >= part) {
        memcpy(&ctx->buf[idx], data, part);
        md5_transform(ctx, ctx->buf);
        for (i = part; i + 63 < len; i += 64) md5_transform(ctx, &data[i]);
        idx = 0;
    }
    if (i < len) memcpy(&ctx->buf[idx], &data[i], len - i);
}

static void md5_final(MD5_CTX *ctx, unsigned char digest[16]) {
    unsigned char bits[8];
    static const unsigned char pad[64] = { 0x80 };
    unsigned int idx, padlen;
    int i, j;
    for (i = 0; i < 8; i++)
        bits[i] = (unsigned char)(ctx->count[i < 4 ? 0 : 1] >> ((i & 3) * 8));
    idx    = (ctx->count[0] >> 3) & 0x3F;
    padlen = (idx < 56) ? (56 - idx) : (120 - idx);
    md5_update(ctx, pad, padlen);
    md5_update(ctx, bits, 8);
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            digest[i*4+j] = (unsigned char)(ctx->s[i] >> (j * 8));
}

/* Mojang-style offline UUID: v3 of MD5("OfflinePlayer:" + name). */
static void OfflineUUID(const char *username, char out[37]) {
    char nameBuf[128];
    MD5_CTX ctx;
    unsigned char d[16];
    sprintf(nameBuf, "OfflinePlayer:%s", username);
    md5_init(&ctx);
    md5_update(&ctx, (const unsigned char *)nameBuf, (unsigned int)strlen(nameBuf));
    md5_final(&ctx, d);
    d[6] = (unsigned char)((d[6] & 0x0F) | 0x30);
    d[8] = (unsigned char)((d[8] & 0x3F) | 0x80);
    sprintf(out, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        d[0],d[1],d[2],d[3], d[4],d[5], d[6],d[7], d[8],d[9],
        d[10],d[11],d[12],d[13],d[14],d[15]);
}

/* ---------- Helpers ---------- */

static void TrimRight(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1] == '\r' || s[n-1] == '\n' || s[n-1] == ' ' || s[n-1] == '\t'))
        s[--n] = 0;
}

static void CopyField(char *dst, size_t cap, const char *src) {
    if (cap == 0) return;
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = 0;
}

static int IsValidUsername(const char *u) {
    int n = (int)strlen(u);
    int i;
    if (n < 1 || n > 16) return 0;
    for (i = 0; i < n; i++) {
        char c = u[i];
        int ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                 (c >= '0' && c <= '9') || (c == '_');
        if (!ok) return 0;
    }
    return 1;
}

static int IsValidRam(const char *r) {
    int n = (int)strlen(r);
    int i, val = 0;
    if (n < 2 || n > 4) return 0;
    for (i = 0; i < n; i++) {
        if (r[i] < '0' || r[i] > '9') return 0;
        val = val * 10 + (r[i] - '0');
    }
    return val >= 64 && val <= 8192;
}

static int FindVersionIndex(const char *v) {
    int i;
    for (i = 0; i < kVersionCount; i++)
        if (strcmp(v, kVersions[i]) == 0) return i;
    return -1;
}

/* ---------- OpenGL mode discovery ---------- */

static void AddGlMode(const char *name, const char *id,
                      const char *dirPath, int setFallback) {
    GlMode *m;
    if (g_glModeCount >= MAX_GL_MODES) return;
    m = &g_glModes[g_glModeCount++];
    CopyField(m->name,    sizeof(m->name),    name);
    CopyField(m->id,      sizeof(m->id),      id);
    CopyField(m->dirPath, sizeof(m->dirPath), dirPath ? dirPath : "");
    m->setLwjglFallback = setFallback;
}

/* Read display-name override from <dir>\name.txt. Falls back to dir basename. */
static void ReadModeDisplayName(const char *dir, const char *fallback,
                                char *out, size_t cap) {
    char path[MAX_PATH];
    FILE *fp;
    sprintf(path, "%s\\name.txt", dir);
    fp = fopen(path, "r");
    if (fp) {
        if (fgets(out, (int)cap, fp)) {
            TrimRight(out);
            if (out[0]) { fclose(fp); return; }
        }
        fclose(fp);
    }
    _snprintf(out, cap, "Software: %s", fallback);
    out[cap - 1] = 0;
}

static void DiscoverGlModes(void) {
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char dirPath[MAX_PATH];
    char glProbe[MAX_PATH];
    char displayName[64];
    char id[64];

    g_glModeCount = 0;
    AddGlMode("Default (system OpenGL)",     "default",        NULL, 0);
    AddGlMode("Software fallback (LWJGL)",   "lwjgl-fallback", NULL, 1);

    h = FindFirstFileA("resources\\gl\\*", &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (fd.cFileName[0] == '.') continue;
        sprintf(dirPath, "resources\\gl\\%s", fd.cFileName);
        sprintf(glProbe, "%s\\opengl32.dll", dirPath);
        if (GetFileAttributesA(glProbe) == INVALID_FILE_ATTRIBUTES) continue;
        ReadModeDisplayName(dirPath, fd.cFileName, displayName, sizeof(displayName));
        _snprintf(id, sizeof(id), "gl-%s", fd.cFileName);
        id[sizeof(id) - 1] = 0;
        AddGlMode(displayName, id, dirPath, 1);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static int FindGlModeIndex(const char *id) {
    int i;
    for (i = 0; i < g_glModeCount; i++)
        if (strcmp(g_glModes[i].id, id) == 0) return i;
    return 0;  /* fall back to Default */
}

/* Forward declaration; SetStatus is defined further down. */
static void SetStatus(const char *msg);

/* ---------- Java discovery ---------- */

/* Returns 32 / 64 for an i386 / amd64 PE, or 0 if file missing or unknown. */
static int GetExeBitness(const char *path) {
    HANDLE h;
    DWORD got = 0;
    DWORD peOff = 0;
    unsigned char dos[64];
    unsigned char hdr[24];   /* PE\0\0 sig + IMAGE_FILE_HEADER */
    int bits = 0;

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
    /* Machine word is at hdr[4..5], little-endian. */
    {
        WORD machine = (WORD)(hdr[4] | (hdr[5] << 8));
        if (machine == 0x014C) bits = 32;
        else if (machine == 0x8664 || machine == 0xAA64) bits = 64;
    }
    return bits;
}

static void NormalizePath(char *s) {
    while (*s) { if (*s == '/') *s = '\\'; s++; }
}

static int JavaAlreadyKnown(const char *path) {
    int i;
    for (i = 0; i < g_javaCount; i++)
        if (_stricmp(g_javas[i].path, path) == 0) return 1;
    return 0;
}

/* Take a candidate java home (or java.exe path). Add to list if it's a
   32-bit java.exe and we haven't already seen it. */
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

    /* If it's a directory, append \bin\java.exe. */
    attrs = GetFileAttributesA(full);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        size_t n = strlen(full);
        while (n > 0 && full[n - 1] == '\\') full[--n] = 0;
        _snprintf(full + n, sizeof(full) - n, "\\bin\\java.exe");
        full[sizeof(full) - 1] = 0;
    }

    if (GetFileAttributesA(full) == INVALID_FILE_ATTRIBUTES) return;

    /* Resolve to canonical absolute path so dedup actually catches dupes. */
    r = GetFullPathNameA(full, sizeof(resolved), resolved, NULL);
    if (r == 0 || r >= sizeof(resolved)) CopyField(resolved, sizeof(resolved), full);

    if (JavaAlreadyKnown(resolved)) return;
    if (GetExeBitness(resolved) != 32) return;

    {
        JavaInstall *j = &g_javas[g_javaCount++];
        char home[MAX_PATH];
        char *bs;
        CopyField(home, sizeof(home), resolved);
        bs = strrchr(home, '\\');           /* strip java.exe */
        if (bs) *bs = 0;
        bs = strrchr(home, '\\');           /* strip bin */
        if (bs) *bs = 0;
        base = strrchr(home, '\\');
        base = base ? base + 1 : home;
        _snprintf(j->displayName, sizeof(j->displayName), "%s  (%s)", base, home);
        j->displayName[sizeof(j->displayName) - 1] = 0;
        CopyField(j->path, sizeof(j->path), resolved);
    }
}

/* Enumerate HKLM\<subkey>\<version>\JavaHome. */
static void ScanJavaRegistry(HKEY root, const char *subkey) {
    HKEY hKey, hSub;
    DWORD i;
    char ver[128];
    DWORD verLen;
    char home[MAX_PATH];
    DWORD homeLen;
    DWORD type;

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

/* Scan <programFiles>\Java\*\bin\java.exe and <programFiles>\BellSoft\*\bin\java.exe. */
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

    /* 1. Currently configured java=... in settings.txt, if valid + 32-bit. */
    if (g_cfgJava[0]) {
        char resolved[MAX_PATH];
        DWORD rr = SearchPathA(NULL, g_cfgJava, ".exe", sizeof(resolved), resolved, NULL);
        if (rr > 0 && rr < sizeof(resolved)) TryAddJava(resolved);
        else TryAddJava(g_cfgJava);
    }

    /* 2. JAVA_HOME. */
    n = GetEnvironmentVariableA("JAVA_HOME", env, sizeof(env));
    if (n > 0 && n < sizeof(env)) TryAddJava(env);

    /* 3. Registry (both views, JRE + JDK). */
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\Java Runtime Environment");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\Java Development Kit");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\JRE");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\JDK");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\JavaSoft\\Java Runtime Environment");
    ScanJavaRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\JavaSoft\\Java Development Kit");

    /* 4. Filesystem scan of Program Files trees. */
    n = GetEnvironmentVariableA("ProgramFiles(x86)", env, sizeof(env));
    if (n > 0 && n < sizeof(env)) ScanProgramFilesJava(env);
    n = GetEnvironmentVariableA("ProgramFiles", env, sizeof(env));
    if (n > 0 && n < sizeof(env)) ScanProgramFilesJava(env);
    n = GetEnvironmentVariableA("ProgramW6432", env, sizeof(env));
    if (n > 0 && n < sizeof(env)) ScanProgramFilesJava(env);
}

static int FindJavaIndex(const char *path) {
    int i;
    if (!path || !path[0]) return -1;
    for (i = 0; i < g_javaCount; i++)
        if (_stricmp(g_javas[i].path, path) == 0) return i;
    return -1;
}

/* ---------- JRE installer discovery (resources\jre\*.exe / *.msi) ---------- */

static void DiscoverJreInstallers(void) {
    WIN32_FIND_DATAA fd;
    HANDLE h;
    const char *ext;
    g_jreInstallerCount = 0;
    h = FindFirstFileA("resources\\jre\\*", &fd);
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
                      "resources\\jre\\%s", fd.cFileName);
            ji->path[sizeof(ji->path) - 1] = 0;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static void ShowInstallJavaMenu(void) {
    HMENU hMenu;
    RECT btn;
    int i;
    DiscoverJreInstallers();
    if (g_jreInstallerCount == 0) {
        MessageBoxA(g_hwnd,
            "No installers found in resources\\jre\\.\n\n"
            "Drop a 32-bit Java installer (.exe or .msi) there and try again.",
            "Install Java", MB_ICONINFORMATION | MB_OK);
        return;
    }
    hMenu = CreatePopupMenu();
    if (!hMenu) return;
    for (i = 0; i < g_jreInstallerCount; i++)
        AppendMenuA(hMenu, MF_STRING,
                    (UINT_PTR)(ID_MENU_INSTALL_BASE + i),
                    g_jreInstallers[i].name);
    GetWindowRect(g_hButtonInstallJava, &btn);
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN,
                   btn.left, btn.bottom, 0, g_hwnd, NULL);
    DestroyMenu(hMenu);
}

static void RunJreInstaller(int idx) {
    JreInstaller *ji;
    HINSTANCE r;
    if (idx < 0 || idx >= g_jreInstallerCount) return;
    ji = &g_jreInstallers[idx];
    r = ShellExecuteA(g_hwnd, "open", ji->path, NULL, NULL, SW_SHOWNORMAL);
    if ((DWORD_PTR)r <= 32) {
        char err[256];
        sprintf(err, "Failed to launch %s (ShellExecute error %lu).",
                ji->name, (DWORD)(DWORD_PTR)r);
        MessageBoxA(g_hwnd, err, "Install Java", MB_ICONEXCLAMATION | MB_OK);
        LogError(err);
        return;
    }
    SetStatus("Installer launched. When it finishes, restart the launcher to detect the new Java.");
}

/* ---------- GL mode install / env-var save & restore ---------- */

#define MAX_SAVED_ENV 8
typedef struct { char name[64]; char prev[1024]; int hadPrev; } SavedEnv;
static SavedEnv g_savedEnv[MAX_SAVED_ENV];
static int      g_savedEnvCount = 0;

static void EnvSetWithSave(const char *name, const char *value) {
    SavedEnv *s;
    if (g_savedEnvCount >= MAX_SAVED_ENV) return;
    s = &g_savedEnv[g_savedEnvCount++];
    CopyField(s->name, sizeof(s->name), name);
    s->prev[0] = 0;
    s->hadPrev = (GetEnvironmentVariableA(name, s->prev, sizeof(s->prev)) > 0);
    SetEnvironmentVariableA(name, value);
}

static void EnvRestoreAll(void) {
    int i;
    for (i = 0; i < g_savedEnvCount; i++) {
        SavedEnv *s = &g_savedEnv[i];
        SetEnvironmentVariableA(s->name, s->hadPrev ? s->prev : NULL);
    }
    g_savedEnvCount = 0;
}

/* Read env.txt (KEY=VALUE per line, # comments) from the mode dir and set vars. */
static void ApplyModeEnv(const char *modeDir) {
    char path[MAX_PATH];
    char line[1024];
    FILE *fp;
    sprintf(path, "%s\\env.txt", modeDir);
    fp = fopen(path, "r");
    if (!fp) return;
    while (fgets(line, sizeof(line), fp)) {
        char *eq;
        TrimRight(line);
        if (line[0] == 0 || line[0] == '#') continue;
        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        EnvSetWithSave(line, eq + 1);
    }
    fclose(fp);
}

#define JRE_MANIFEST_PATH ".gl_jre_install"
#define BAK_SUFFIX        ".mc9xbak"

/* Per-version manifest of files we previously copied into the version dir. */
static void GlManifestPath(const char *version, char *out) {
    sprintf(out, "resources\\versions\\%s\\.gl_manifest", version);
}

/* Resolve g_cfgJava to a full path to java.exe via PATH. */
static int ResolveJavaExe(char *out, size_t cap) {
    DWORD r = SearchPathA(NULL, g_cfgJava, ".exe", (DWORD)cap, out, NULL);
    return (r > 0 && r < cap) ? 1 : 0;
}

/* Read the JRE-install manifest and undo whatever we did last time:
   delete files we copied into the JRE bin, restore any backups. */
static void CleanPreviousJreInstall(void) {
    FILE *fp;
    char line[512];
    char jreDir[MAX_PATH] = "";
    char filePath[MAX_PATH], bakPath[MAX_PATH];

    fp = fopen(JRE_MANIFEST_PATH, "r");
    if (!fp) return;
    while (fgets(line, sizeof(line), fp)) {
        TrimRight(line);
        if (line[0] == 0) continue;
        if (strncmp(line, "javadir=", 8) == 0) {
            CopyField(jreDir, sizeof(jreDir), line + 8);
            continue;
        }
        if (jreDir[0] == 0) continue;
        sprintf(filePath, "%s\\%s", jreDir, line);
        DeleteFileA(filePath);
        sprintf(bakPath, "%s%s", filePath, BAK_SUFFIX);
        if (GetFileAttributesA(bakPath) != INVALID_FILE_ATTRIBUTES) {
            MoveFileA(bakPath, filePath);
        }
    }
    fclose(fp);
    DeleteFileA(JRE_MANIFEST_PATH);
}

/* Create <jreDir>\<exeName>.local if not already present. The .local
   mechanism overrides KnownDLLs so the loader looks in <jreDir> first. */
static void EnsureLocalMarker(const char *jreDir, const char *exeName,
                              FILE *manifest) {
    char path[MAX_PATH];
    HANDLE h;
    sprintf(path, "%s\\%s.local", jreDir, exeName);
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return;
    h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        if (manifest) fprintf(manifest, "%s.local\n", exeName);
    }
}

/* Mirror the mode dir into java.exe's directory (so opengl32.dll wins
   over System32 via the .local redirect we drop alongside it). */
static int InstallAtJreLocation(const char *modeDir) {
    char javaPath[MAX_PATH];
    char jreDir[MAX_PATH];
    char *bs;
    char src[MAX_PATH], dst[MAX_PATH], bak[MAX_PATH];
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    FILE *manifest;
    int copied = 0;
    DWORD attrs;

    if (!ResolveJavaExe(javaPath, sizeof(javaPath))) return 0;
    CopyField(jreDir, sizeof(jreDir), javaPath);
    bs = strrchr(jreDir, '\\');
    if (!bs) return 0;
    *bs = 0;

    manifest = fopen(JRE_MANIFEST_PATH, "w");
    if (manifest) fprintf(manifest, "javadir=%s\n", jreDir);

    sprintf(pattern, "%s\\*", modeDir);
    h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (fd.cFileName[0] == '.') continue;
            if (_stricmp(fd.cFileName, "env.txt")     == 0) continue;
            if (_stricmp(fd.cFileName, "name.txt")    == 0) continue;
            if (_stricmp(fd.cFileName, "license.txt") == 0) continue;
            if (_stricmp(fd.cFileName, "README.txt")  == 0) continue;

            sprintf(src, "%s\\%s", modeDir, fd.cFileName);
            sprintf(dst, "%s\\%s", jreDir, fd.cFileName);
            attrs = GetFileAttributesA(dst);
            if (attrs != INVALID_FILE_ATTRIBUTES) {
                sprintf(bak, "%s%s", dst, BAK_SUFFIX);
                DeleteFileA(bak);
                if (!MoveFileA(dst, bak)) continue;
            }
            if (CopyFileA(src, dst, FALSE)) {
                if (manifest) fprintf(manifest, "%s\n", fd.cFileName);
                copied++;
            } else if (attrs != INVALID_FILE_ATTRIBUTES) {
                MoveFileA(bak, dst);
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    if (copied > 0) {
        EnsureLocalMarker(jreDir, "java",  manifest);
        EnsureLocalMarker(jreDir, "javaw", manifest);
    }
    if (manifest) fclose(manifest);
    return copied;
}

/* Delete all files listed in the manifest, then delete the manifest itself. */
static void CleanPreviousGlInstall(const char *version) {
    char manifestPath[MAX_PATH];
    char filePath[MAX_PATH];
    char line[256];
    FILE *fp;
    GlManifestPath(version, manifestPath);
    fp = fopen(manifestPath, "r");
    if (!fp) return;
    while (fgets(line, sizeof(line), fp)) {
        TrimRight(line);
        if (line[0] == 0) continue;
        sprintf(filePath, "resources\\versions\\%s\\%s", version, line);
        DeleteFileA(filePath);
    }
    fclose(fp);
    DeleteFileA(manifestPath);
}

/* Copy every file from <modeDir>\* into resources\versions\<version>\, except
   helper files (env.txt, name.txt, README.txt). Writes a manifest of what was
   copied so the next mode switch can clean up. */
static int CopyGlModeFiles(const char *modeDir, const char *version) {
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char pattern[MAX_PATH];
    char src[MAX_PATH];
    char dst[MAX_PATH];
    char manifestPath[MAX_PATH];
    FILE *manifest;
    int copied = 0;
    sprintf(pattern, "%s\\*", modeDir);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;

    GlManifestPath(version, manifestPath);
    manifest = fopen(manifestPath, "w");
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (fd.cFileName[0] == '.') continue;
        if (_stricmp(fd.cFileName, "env.txt")    == 0) continue;
        if (_stricmp(fd.cFileName, "name.txt")   == 0) continue;
        if (_stricmp(fd.cFileName, "README.txt") == 0) continue;
        sprintf(src, "%s\\%s", modeDir, fd.cFileName);
        sprintf(dst, "resources\\versions\\%s\\%s", version, fd.cFileName);
        if (CopyFileA(src, dst, FALSE)) {
            if (manifest) fprintf(manifest, "%s\n", fd.cFileName);
            copied++;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    if (manifest) fclose(manifest);
    return copied;
}

/* ---------- settings.txt ---------- */

static void LoadSettings(void) {
    FILE *fp = fopen("settings.txt", "r");
    char line[512];
    if (!fp) return;
    while (fgets(line, sizeof(line), fp)) {
        char *eq;
        const char *key, *val;
        TrimRight(line);
        if (line[0] == 0 || line[0] == '#') continue;
        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        key = line;
        val = eq + 1;
        if      (strcmp(key, "version")    == 0) CopyField(g_cfgVersion,   sizeof(g_cfgVersion),   val);
        else if (strcmp(key, "username")   == 0) CopyField(g_cfgUsername,  sizeof(g_cfgUsername),  val);
        else if (strcmp(key, "ram_max_mb") == 0) CopyField(g_cfgRamMaxMb,  sizeof(g_cfgRamMaxMb),  val);
        else if (strcmp(key, "ram_min_mb") == 0) CopyField(g_cfgRamMinMb,  sizeof(g_cfgRamMinMb),  val);
        else if (strcmp(key, "ram_mb")     == 0) CopyField(g_cfgRamMaxMb,  sizeof(g_cfgRamMaxMb),  val); /* legacy key */
        else if (strcmp(key, "java")       == 0) CopyField(g_cfgJava,      sizeof(g_cfgJava),      val);
        else if (strcmp(key, "nosound")    == 0) g_cfgNoSound = (val[0] == '1' || val[0] == 't' || val[0] == 'T');
        else if (strcmp(key, "gl_mode")    == 0) CopyField(g_cfgGlMode,    sizeof(g_cfgGlMode),    val);
    }
    fclose(fp);
    if (!IsValidRam(g_cfgRamMaxMb)) CopyField(g_cfgRamMaxMb, sizeof(g_cfgRamMaxMb), "512");
    if (!IsValidRam(g_cfgRamMinMb)) CopyField(g_cfgRamMinMb, sizeof(g_cfgRamMinMb), "256");
}

static void SaveSettings(void) {
    FILE *fp = fopen("settings.txt", "w");
    if (!fp) { LogError("Failed to write settings.txt"); return; }
    fprintf(fp, "version=%s\n",    g_cfgVersion);
    fprintf(fp, "username=%s\n",   g_cfgUsername);
    fprintf(fp, "ram_max_mb=%s\n", g_cfgRamMaxMb);
    fprintf(fp, "ram_min_mb=%s\n", g_cfgRamMinMb);
    fprintf(fp, "nosound=%d\n",    g_cfgNoSound ? 1 : 0);
    fprintf(fp, "gl_mode=%s\n",    g_cfgGlMode);
    fprintf(fp, "java=%s\n",       g_cfgJava);
    fclose(fp);
}

/* ---------- launch.ini ---------- */

static int ReadLaunchIni(const char *version,
                         char *mainClass, size_t mainCap,
                         char *jvmArgs,   size_t jvmCap,
                         char *classpath, size_t cpCap,
                         char *gameArgs,  size_t gameCap) {
    char path[MAX_PATH];
    char line[8192];
    FILE *fp;
    sprintf(path, "resources\\versions\\%s\\launch.ini", version);
    fp = fopen(path, "r");
    if (!fp) return 0;
    mainClass[0] = jvmArgs[0] = classpath[0] = gameArgs[0] = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *eq;
        const char *key, *val;
        TrimRight(line);
        if (line[0] == 0 || line[0] == '#') continue;
        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        key = line;
        val = eq + 1;
        if      (strcmp(key, "main_class") == 0) CopyField(mainClass, mainCap, val);
        else if (strcmp(key, "jvm_args")   == 0) CopyField(jvmArgs,   jvmCap,  val);
        else if (strcmp(key, "classpath")  == 0) CopyField(classpath, cpCap,   val);
        else if (strcmp(key, "game_args")  == 0) CopyField(gameArgs,  gameCap, val);
    }
    fclose(fp);
    return mainClass[0] != 0 && classpath[0] != 0;
}

/* Substitute {username} {uuid} {xmx} {xms} placeholders. Unknown {tokens} are passed through. */
static void Substitute(char *out, size_t outCap, const char *in,
                       const char *username, const char *uuid) {
    size_t oi = 0;
    size_t i  = 0;
    while (in[i] && oi + 1 < outCap) {
        if (in[i] == '{') {
            const char *end = strchr(in + i + 1, '}');
            if (end) {
                size_t klen = (size_t)(end - (in + i + 1));
                const char *val = NULL;
                if      (klen == 8 && strncmp(in + i + 1, "username", 8) == 0) val = username;
                else if (klen == 4 && strncmp(in + i + 1, "uuid",     4) == 0) val = uuid;
                else if (klen == 3 && strncmp(in + i + 1, "ram",      3) == 0) val = g_cfgRamMaxMb;
                else if (klen == 7 && strncmp(in + i + 1, "ram_max", 7) == 0) val = g_cfgRamMaxMb;
                else if (klen == 7 && strncmp(in + i + 1, "ram_min", 7) == 0) val = g_cfgRamMinMb;
                if (val) {
                    size_t vl = strlen(val);
                    if (oi + vl >= outCap) vl = outCap - oi - 1;
                    memcpy(out + oi, val, vl);
                    oi += vl;
                    i = (size_t)(end - in) + 1;
                    continue;
                }
            }
        }
        out[oi++] = in[i++];
    }
    out[oi] = 0;
}

/* ---------- Launch ---------- */

static void LaunchVersion(const char *version, const char *username) {
    char mainClass[256] = "";
    char jvmArgs[4096]  = "";
    char *classpath = (char *)malloc(MAX_CLASSPATH);
    char gameArgs[2048] = "";
    char jvmS[4096];
    char gameS[2048];
    char *cpS = NULL;
    char *cmd = NULL;
    char workDir[MAX_PATH], fullWorkDir[MAX_PATH];
    char uuid[37];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    if (!classpath) { LogError("Out of memory (classpath)"); return; }

    if (!ReadLaunchIni(version, mainClass, sizeof(mainClass),
                       jvmArgs, sizeof(jvmArgs),
                       classpath, MAX_CLASSPATH,
                       gameArgs, sizeof(gameArgs))) {
        char err[256];
        sprintf(err, "Missing or invalid resources\\versions\\%s\\launch.ini", version);
        MessageBoxA(g_hwnd, err, "Error", MB_ICONEXCLAMATION | MB_OK);
        LogError(err);
        free(classpath);
        return;
    }

    OfflineUUID(username, uuid);

    cpS = (char *)malloc(MAX_CLASSPATH);
    cmd = (char *)malloc(MAX_LAUNCH_CMD);
    if (!cpS || !cmd) {
        LogError("Out of memory");
        free(classpath); free(cpS); free(cmd);
        return;
    }

    Substitute(jvmS,  sizeof(jvmS),  jvmArgs,   username, uuid);
    Substitute(cpS,   MAX_CLASSPATH, classpath, username, uuid);
    Substitute(gameS, sizeof(gameS), gameArgs,  username, uuid);

    if (g_cfgNoSound) {
        const char *flag = " -Dorg.lwjgl.openal.libname=disabled";
        if (strlen(jvmS) + strlen(flag) + 1 < sizeof(jvmS)) strcat(jvmS, flag);
    }

    /* Apply GL override:
         1. undo any prior install in this version dir AND in the JRE bin dir
         2. mirror the new mode's files into both
         3. set env vars; optionally add the LWJGL fallback flag
       The JRE-bin install is the load-bearing one - opengl32.dll is a
       KnownDLL and the loader otherwise ignores per-app copies. The
       .local marker we drop next to java.exe defeats KnownDLLs. */
    {
        GlMode *gl = &g_glModes[FindGlModeIndex(g_cfgGlMode)];
        CleanPreviousGlInstall(version);
        CleanPreviousJreInstall();
        if (gl->dirPath[0]) {
            int verCount = CopyGlModeFiles(gl->dirPath, version);
            int jreCount = InstallAtJreLocation(gl->dirPath);
            if (verCount == 0 && jreCount == 0) {
                char err[256];
                sprintf(err, "OpenGL mode '%s' has no files to install in %s.",
                        gl->name, gl->dirPath);
                MessageBoxA(g_hwnd, err, "OpenGL Mode", MB_ICONEXCLAMATION | MB_OK);
                LogError(err);
            } else if (jreCount == 0) {
                /* Found mode but couldn't reach JRE bin - usually permissions
                   or 'java' not resolvable to a full path. Warn the user since
                   the override likely will not take effect. */
                char err[384];
                sprintf(err,
                    "Installed '%s' to the version dir but could not reach\n"
                    "java.exe's directory. opengl32.dll is a KnownDLL on\n"
                    "Windows XP+, so without the install at java.exe's dir\n"
                    "this override may not take effect.\n\n"
                    "Make 'java=' in settings.txt point at JRE\\bin\\java.exe\n"
                    "(full path), or run the launcher as Administrator.",
                    gl->name);
                MessageBoxA(g_hwnd, err, "OpenGL Mode", MB_ICONINFORMATION | MB_OK);
                LogError(err);
            }
            ApplyModeEnv(gl->dirPath);
        }
        if (gl->setLwjglFallback) {
            const char *flag = " -Dorg.lwjgl.opengl.Display.allowSoftwareOpenGL=true";
            if (strlen(jvmS) + strlen(flag) + 1 < sizeof(jvmS)) strcat(jvmS, flag);
        }
    }

    GetCurrentDirectoryA(sizeof(workDir), workDir);
    sprintf(fullWorkDir, "%s\\resources\\versions\\%s", workDir, version);

    _snprintf(cmd, MAX_LAUNCH_CMD, "\"%s\" %s -cp \"%s\" %s %s",
              g_cfgJava, jvmS, cpS, mainClass, gameS);
    cmd[MAX_LAUNCH_CMD - 1] = 0;

    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, fullWorkDir, &si, &pi)) {
        char errMsg[256];
        sprintf(errMsg, "Failed to launch Java (error %lu). Check 'java' path in settings.txt.",
                GetLastError());
        MessageBoxA(g_hwnd, errMsg, "Launch Failed", MB_ICONEXCLAMATION | MB_OK);
        LogError(errMsg);
    } else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    EnvRestoreAll();

    free(classpath); free(cpS); free(cmd);
}

static void SetStatus(const char *msg) {
    if (g_hLabelStatus) SetWindowTextA(g_hLabelStatus, msg);
}

static int Atoi(const char *s) {
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

static void OnPlayClicked(void) {
    char username[64];
    char ramMax[8], ramMin[8];
    char status[160];
    int idx;
    const char *version;

    GetWindowTextA(g_hTextboxUsername, username, sizeof(username));
    if (!IsValidUsername(username)) {
        MessageBoxA(g_hwnd,
            "Username must be 1-16 characters, letters/digits/underscore only.",
            "Invalid Username", MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    GetWindowTextA(g_hTextboxRamMax, ramMax, sizeof(ramMax));
    GetWindowTextA(g_hTextboxRamMin, ramMin, sizeof(ramMin));
    if (!IsValidRam(ramMax) || !IsValidRam(ramMin)) {
        MessageBoxA(g_hwnd,
            "Max and Min RAM must each be a number of megabytes between 64 and 8192.",
            "Invalid RAM", MB_ICONEXCLAMATION | MB_OK);
        return;
    }
    if (Atoi(ramMin) > Atoi(ramMax)) {
        MessageBoxA(g_hwnd,
            "Min RAM cannot be larger than Max RAM.",
            "Invalid RAM", MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    idx = (int)SendMessageA(g_hDropdownVersion, CB_GETCURSEL, 0, 0);
    if (idx < 0 || idx >= kVersionCount) return;
    version = kVersions[idx];

    g_cfgNoSound = (SendMessageA(g_hCheckNoSound, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;

    {
        int glIdx = (int)SendMessageA(g_hDropdownGl, CB_GETCURSEL, 0, 0);
        if (glIdx < 0 || glIdx >= g_glModeCount) glIdx = 0;
        CopyField(g_cfgGlMode, sizeof(g_cfgGlMode), g_glModes[glIdx].id);
    }

    {
        int jIdx = (int)SendMessageA(g_hDropdownJava, CB_GETCURSEL, 0, 0);
        if (jIdx < 0 || jIdx >= g_javaCount) {
            MessageBoxA(g_hwnd,
                "No 32-bit Java is selected. Click \"Install Java...\" to install "
                "one from resources\\jre\\, then restart the launcher.",
                "Java Required", MB_ICONEXCLAMATION | MB_OK);
            return;
        }
        CopyField(g_cfgJava, sizeof(g_cfgJava), g_javas[jIdx].path);
    }

    CopyField(g_cfgUsername,  sizeof(g_cfgUsername),  username);
    CopyField(g_cfgVersion,   sizeof(g_cfgVersion),   version);
    CopyField(g_cfgRamMaxMb,  sizeof(g_cfgRamMaxMb),  ramMax);
    CopyField(g_cfgRamMinMb,  sizeof(g_cfgRamMinMb),  ramMin);
    SaveSettings();

    sprintf(status, "Launching %s as %s (%s-%s MB%s, GL: %s)...",
            version, username, ramMin, ramMax,
            g_cfgNoSound ? ", no sound" : "",
            g_glModes[FindGlModeIndex(g_cfgGlMode)].name);
    SetStatus(status);

    LaunchVersion(version, username);

    sprintf(status, "Launched %s. Watch for the Minecraft window.", version);
    SetStatus(status);
}

/* ---------- Window proc ---------- */

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            if (g_hBitmap) DeleteObject(g_hBitmap);
            if (g_hFont)   DeleteObject(g_hFont);
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            if (id == ID_BTN_PLAY) {
                OnPlayClicked();
            } else if (id == ID_BTN_INSTALL_JAVA) {
                ShowInstallJavaMenu();
            } else if (id >= ID_MENU_INSTALL_BASE
                    && id < ID_MENU_INSTALL_BASE + MAX_JRE_INSTALLERS) {
                RunJreInstaller(id - ID_MENU_INSTALL_BASE);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (g_hBitmap) {
                HDC hdcImage = CreateCompatibleDC(hdc);
                HBITMAP hOld = (HBITMAP)SelectObject(hdcImage, g_hBitmap);
                BITMAP bm;
                GetObject(g_hBitmap, sizeof(bm), &bm);
                BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcImage, 0, 0, SRCCOPY);
                SelectObject(hdcImage, hOld);
                DeleteDC(hdcImage);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

/* ---------- Release notes ---------- */

static void LoadReleaseNotes(char *out, size_t cap) {
    FILE *fp = fopen("release_notes.txt", "r");
    size_t n;
    if (!fp) {
        _snprintf(out, cap, "Win9x Minecraft Launcher v%s", LAUNCHER_VERSION);
        out[cap - 1] = 0;
        return;
    }
    n = fread(out, 1, cap - 1, fp);
    out[n] = 0;
    fclose(fp);
}

/* ---------- Entry point ---------- */

/* Apply the system GUI font to every child of the main window. */
static BOOL CALLBACK ApplyFontProc(HWND hChild, LPARAM lParam) {
    SendMessageA(hChild, WM_SETFONT, (WPARAM)(HFONT)lParam, MAKELPARAM(TRUE, 0));
    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc;
    char title[128];
    char notes[2048];
    int defIdx, i;
    MSG msg;

    (void)hPrevInstance;
    (void)lpCmdLine;

    LoadSettings();

    g_hBitmap = (HBITMAP)LoadImageA(NULL, "resources\\background.bmp",
                                    IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!g_hBitmap) {
        MessageBoxA(NULL, "Failed to load background image!", "Error", MB_ICONEXCLAMATION | MB_OK);
        LogError("Failed to load background image");
        return 0;
    }

    g_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "Win9xMcLauncher";
    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        LogError("Window Registration Failed");
        return 0;
    }

    _snprintf(title, sizeof(title), "Win9x Minecraft Launcher v%s", LAUNCHER_VERSION);
    title[sizeof(title) - 1] = 0;

    g_hwnd = CreateWindowA("Win9xMcLauncher", title,
        (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX)) | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 470,
        NULL, NULL, hInstance, NULL);
    if (g_hwnd == NULL) {
        MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        LogError("Window Creation Failed");
        return 0;
    }

    DiscoverGlModes();

    LoadReleaseNotes(notes, sizeof(notes));
    g_hLabelInfo = CreateWindowA("STATIC", notes,
        WS_VISIBLE | WS_CHILD,
        20, 70, 500, 140, g_hwnd, NULL, hInstance, NULL);

    /* Row 1: Version + Username */
    g_hLabelVersion = CreateWindowA("STATIC", "Version:",
        WS_VISIBLE | WS_CHILD,
        15, 228, 55, 18, g_hwnd, NULL, hInstance, NULL);

    g_hDropdownVersion = CreateWindowA("COMBOBOX", "",
        CBS_DROPDOWNLIST | WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
        75, 225, 130, 200, g_hwnd, NULL, hInstance, NULL);
    for (i = 0; i < kVersionCount; i++)
        SendMessageA(g_hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)kVersions[i]);

    defIdx = FindVersionIndex(g_cfgVersion);
    if (defIdx < 0) defIdx = FindVersionIndex("beta1.7.3");
    if (defIdx < 0) defIdx = 0;
    SendMessageA(g_hDropdownVersion, CB_SETCURSEL, (WPARAM)defIdx, 0);

    g_hLabelUsername = CreateWindowA("STATIC", "Username:",
        WS_VISIBLE | WS_CHILD,
        220, 228, 70, 18, g_hwnd, NULL, hInstance, NULL);

    g_hTextboxUsername = CreateWindowA("EDIT", g_cfgUsername,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        295, 225, 130, 22, g_hwnd, NULL, hInstance, NULL);
    SendMessageA(g_hTextboxUsername, EM_SETLIMITTEXT, 16, 0);

    /* Row 2: Max RAM, Min RAM, No-sound checkbox */
    g_hLabelRamMax = CreateWindowA("STATIC", "Max (MB):",
        WS_VISIBLE | WS_CHILD,
        15, 258, 65, 18, g_hwnd, NULL, hInstance, NULL);

    g_hTextboxRamMax = CreateWindowA("EDIT", g_cfgRamMaxMb,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
        85, 255, 55, 22, g_hwnd, NULL, hInstance, NULL);
    SendMessageA(g_hTextboxRamMax, EM_SETLIMITTEXT, 4, 0);

    g_hLabelRamMin = CreateWindowA("STATIC", "Min (MB):",
        WS_VISIBLE | WS_CHILD,
        155, 258, 65, 18, g_hwnd, NULL, hInstance, NULL);

    g_hTextboxRamMin = CreateWindowA("EDIT", g_cfgRamMinMb,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
        225, 255, 55, 22, g_hwnd, NULL, hInstance, NULL);
    SendMessageA(g_hTextboxRamMin, EM_SETLIMITTEXT, 4, 0);

    g_hCheckNoSound = CreateWindowA("BUTTON", "No sound",
        BS_AUTOCHECKBOX | WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        300, 256, 110, 22, g_hwnd, (HMENU)ID_CHK_NOSOUND, hInstance, NULL);
    SendMessageA(g_hCheckNoSound, BM_SETCHECK,
        g_cfgNoSound ? BST_CHECKED : BST_UNCHECKED, 0);

    /* Row 3: OpenGL mode + Play */
    g_hLabelGl = CreateWindowA("STATIC", "OpenGL:",
        WS_VISIBLE | WS_CHILD,
        15, 291, 55, 18, g_hwnd, NULL, hInstance, NULL);

    g_hDropdownGl = CreateWindowA("COMBOBOX", "",
        CBS_DROPDOWNLIST | WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
        75, 288, 240, 200, g_hwnd, NULL, hInstance, NULL);
    for (i = 0; i < g_glModeCount; i++)
        SendMessageA(g_hDropdownGl, CB_ADDSTRING, 0, (LPARAM)g_glModes[i].name);
    SendMessageA(g_hDropdownGl, CB_SETCURSEL,
        (WPARAM)FindGlModeIndex(g_cfgGlMode), 0);

    g_hButtonPlay = CreateWindowA("BUTTON", "Play",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        425, 285, 100, 28, g_hwnd, (HMENU)ID_BTN_PLAY, hInstance, NULL);

    /* Row 4: Java install + Install button.
       Only 32-bit JVMs work: this launcher targets Win9x/2000/XP, and even
       on modern Windows the bundled LWJGL 2.x natives are i386. We scan
       JAVA_HOME, the JavaSoft registry keys, and Program Files for
       java.exe, filter out anything that isn't IMAGE_FILE_MACHINE_I386,
       and list what remains. */
    DiscoverJavas();

    g_hLabelJava = CreateWindowA("STATIC", "Java (32-bit):",
        WS_VISIBLE | WS_CHILD,
        15, 323, 80, 18, g_hwnd, NULL, hInstance, NULL);

    g_hDropdownJava = CreateWindowA("COMBOBOX", "",
        CBS_DROPDOWNLIST | WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
        100, 320, 320, 220, g_hwnd, NULL, hInstance, NULL);
    for (i = 0; i < g_javaCount; i++)
        SendMessageA(g_hDropdownJava, CB_ADDSTRING, 0, (LPARAM)g_javas[i].displayName);
    {
        int jIdx = FindJavaIndex(g_cfgJava);
        if (jIdx < 0 && g_javaCount > 0) jIdx = 0;
        if (jIdx >= 0) SendMessageA(g_hDropdownJava, CB_SETCURSEL, (WPARAM)jIdx, 0);
    }

    g_hButtonInstallJava = CreateWindowA("BUTTON", "Install Java...",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        425, 318, 100, 24, g_hwnd, (HMENU)ID_BTN_INSTALL_JAVA, hInstance, NULL);

    /* Status line */
    g_hLabelStatus = CreateWindowA("STATIC", "Ready.",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        15, 360, 510, 50, g_hwnd, NULL, hInstance, NULL);

    if (g_javaCount == 0) {
        SetWindowTextA(g_hLabelStatus,
            "No 32-bit Java detected. Click \"Install Java...\" to install one, "
            "then restart the launcher. Minecraft will not launch without a "
            "32-bit JRE.");
    }

    EnumChildWindows(g_hwnd, ApplyFontProc, (LPARAM)g_hFont);

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
    SetFocus(g_hTextboxUsername);

    memset(&msg, 0, sizeof(msg));
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    SaveSettings();
    return (int)msg.wParam;
}
