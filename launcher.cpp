#include <windows.h>
#include <stdio.h>

HBITMAP g_hBitmap = NULL;
HWND hButtonPlay, hDropdownVersion, hTextboxUsername, hLabelInfo;

void LogError(const char *message) {
    FILE *logFile = fopen("log.txt", "a");
    if (logFile) {
        fprintf(logFile, "Error: %s\n", message);
        fclose(logFile);
    }
}

void CreateBatFile(const char *version, const char *username) {
    FILE *fp = fopen("run.bat", "w");
    if (fp) {
        // For older versions (1.5.2 and earlier)
        if (strcmp(version, "beta1.7.3") == 0 || strcmp(version, "1.0") == 0 || strcmp(version, "1.2.5") == 0 ||
            strcmp(version, "1.4.7") == 0 || strcmp(version, "1.5.2") == 0 || strcmp(version, "alpha1.1.2") == 0 ||
            strcmp(version, "betahacks") == 0) {
            
            // Command for launching older versions
            fprintf(fp, "cd resources\\versions\\%s\n", version);
            fprintf(fp, "java -Xms128M -Xmx256M -Djava.library.path=natives/ -cp \"minecraft.jar;lwjgl.jar;lwjgl_util.jar\" net.minecraft.client.Minecraft ");
            fprintf(fp, "%s\n", username);

        } else if (strcmp(version, "1.8.9") == 0) {
            // Command for launching Minecraft 1.8.9
			fprintf(fp, "cd resources\\versions\\%s\n", version);
			fprintf(fp, "java -XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump -Xmx999m -XX:+UseConcMarkSweepGC -XX:+CMSIncrementalMode -XX:-UseAdaptiveSizePolicy -Xmn256m ");
			fprintf(fp, "-Djava.library.path=\"natives\" ");
			fprintf(fp, "-cp \"1.8.9.jar;..\\..\\libraries\\org\\ow2\\asm\\asm-debug-all\\5.2\\asm-debug-all-5.2.jar;..\\..\\libraries\\net\\minecraft\\launchwrapper\\1.12\\launchwrapper-1.12.jar;..\\..\\libraries\\org\\jline\\jline\\3.5.1\\jline-3.5.1.jar;..\\..\\libraries\\com\\typesafe\\akka\\akka-actor_2.11\\2.3.3\\akka-actor_2.11-2.3.3.jar;..\\..\\libraries\\com\\typesafe\\config\\1.2.1\\config-1.2.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-actors-migration_2.11\\1.1.0\\scala-actors-migration_2.11-1.1.0.jar;..\\..\\libraries\\org\\scala-lang\\scala-compiler\\2.11.1\\scala-compiler-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\plugins\\scala-continuations-library_2.11\\1.0.2_mc\\scala-continuations-library_2.11-1.0.2_mc.jar;..\\..\\libraries\\org\\scala-lang\\plugins\\scala-continuations-plugin_2.11.1\\1.0.2_mc\\scala-continuations-plugin_2.11.1-1.0.2_mc.jar;..\\..\\libraries\\org\\scala-lang\\scala-library\\2.11.1\\scala-library-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-parser-combinators_2.11\\1.0.1\\scala-parser-combinators_2.11-1.0.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-reflect\\2.11.1\\scala-reflect-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-swing_2.11\\1.0.1\\scala-swing_2.11-1.0.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-xml_2.11\\1.0.2\\scala-xml_2.11-1.0.2.jar;..\\..\\libraries\\lzma\\lzma\\0.0.1\\lzma-0.0.1.jar;..\\..\\libraries\\java3d\\vecmath\\1.5.2\\vecmath-1.5.2.jar;..\\..\\libraries\\net\\sf\\trove4j\\trove4j\\3.0.3\\trove4j-3.0.3.jar;..\\..\\libraries\\org\\apache\\maven\\maven-artifact\\3.5.3\\maven-artifact-3.5.3.jar;..\\..\\libraries\\net\\sf\\jopt-simple\\jopt-simple\\5.0.3\\jopt-simple-5.0.3.jar;..\\..\\libraries\\com\\mojang\\patchy\\1.3.9\\patchy-1.3.9.jar;..\\..\\libraries\\oshi-project\\oshi-core\\1.1\\oshi-core-1.1.jar;..\\..\\libraries\\net\\java\\dev\\jna\\jna\\4.4.0\\jna-4.4.0.jar;..\\..\\libraries\\net\\java\\dev\\jna\\platform\\3.4.0\\platform-3.4.0.jar;..\\..\\libraries\\com\\ibm\\icu\\icu4j-core-mojang\\51.2\\icu4j-core-mojang-51.2.jar;..\\..\\libraries\\net\\sf\\jopt-simple\\jopt-simple\\5.0.3\\jopt-simple-5.0.3.jar;..\\..\\libraries\\com\\paulscode\\codecjorbis\\20101023\\codecjorbis-20101023.jar;..\\..\\libraries\\com\\paulscode\\codecwav\\20101023\\codecwav-20101023.jar;..\\..\\libraries\\com\\paulscode\\libraryjavasound\\20101123\\libraryjavasound-20101123.jar;..\\..\\libraries\\com\\paulscode\\librarylwjglopenal\\20100824\\librarylwjglopenal-20100824.jar;..\\..\\libraries\\com\\paulscode\\soundsystem\\20120107\\soundsystem-20120107.jar;..\\..\\libraries\\io\\netty\\netty-all\\4.1.9.Final\\netty-all-4.1.9.Final.jar;..\\..\\libraries\\com\\google\\guava\\guava\\15.0\\guava-15.0.jar;..\\..\\libraries\\org\\apache\\commons\\commons-lang3\\3.5\\commons-lang3-3.5.jar;..\\..\\libraries\\commons-io\\commons-io\\2.5\\commons-io-2.5.jar;..\\..\\libraries\\commons-codec\\commons-codec\\1.10\\commons-codec-1.10.jar;..\\..\\libraries\\net\\java\\jinput\\jinput\\2.0.5\\jinput-2.0.5.jar;..\\..\\libraries\\net\\java\\jutils\\jutils\\1.0.0\\jutils-1.0.0.jar;..\\..\\libraries\\com\\google\\code\\gson\\gson\\2.8.0\\gson-2.8.0.jar;..\\..\\libraries\\com\\mojang\\authlib\\1.5.25\\authlib-1.5.25.jar;..\\..\\libraries\\com\\mojang\\realms\\1.10.22\\realms-1.10.22.jar;..\\..\\libraries\\org\\apache\\commons\\commons-compress\\1.8.1\\commons-compress-1.8.1.jar;..\\..\\libraries\\org\\apache\\httpcomponents\\httpclient\\4.3.3\\httpclient-4.3.3.jar;..\\..\\libraries\\commons-logging\\commons-logging\\1.1.3\\commons-logging-1.1.3.jar;..\\..\\libraries\\org\\apache\\httpcomponents\\httpcore\\4.3.2\\httpcore-4.3.2.jar;..\\..\\libraries\\it\\unimi\\dsi\\fastutil\\7.1.0\\fastutil-7.1.0.jar;..\\..\\libraries\\org\\apache\\logging\\log4j\\log4j-api\\2.8.1\\log4j-api-2.8.1.jar;..\\..\\libraries\\org\\apache\\logging\\log4j\\log4j-core\\2.8.1\\log4j-core-2.8.1.jar;..\\..\\libraries\\org\\lwjgl\\lwjgl\\lwjgl\\2.9.4-nightly-20150209\\lwjgl-2.9.4-nightly-20150209.jar;..\\..\\libraries\\org\\lwjgl\\lwjgl\\lwjgl_util\\2.9.4-nightly-20150209\\lwjgl_util-2.9.4-nightly-20150209.jar;..\\..\\libraries\\com\\mojang\\text2speech\\1.10.3\\text2speech-1.10.3.jar\" ");
			fprintf(fp, "net.minecraft.client.main.Main ");
			fprintf(fp, "--username %s --version 1.8.9 --gameDir . --assetsDir assets --accessToken 0 --userProperties {} --uuid %s --userType legacy --width 1280 --height 720\n", username, username);


        } else if (strcmp(version, "1.6.4") == 0 || strcmp(version, "1.7.10") == 0) {
            // Command for launching Minecraft 1.6.4 and 1.7.10
            fprintf(fp, "cd resources\\versions\\%s\n", version);
            fprintf(fp, "java -Xms512M -Xmx1G -Djava.library.path=\"natives\" ");
            fprintf(fp, "-cp \"1.6.4.jar;..\\..\\libraries\\jopt-simple-4.5.jar;..\\..\\libraries\\launchwrapper-1.5.jar;..\\..\\libraries\\asm-all-4.1.jar;..\\..\\libraries\\guava-14.0.jar;..\\..\\libraries\\commons-io-2.4.jar;..\\..\\libraries\\commons-lang3-3.1.jar;..\\..\\libraries\\netty-1.6.jar;..\\..\\libraries\\log4j-api-2.0-beta9.jar;..\\..\\libraries\\log4j-core-2.0-beta9.jar;..\\..\\libraries\\lwjgl-2.9.1.jar;..\\..\\libraries\\lwjgl_util-2.9.1.jar;..\\..\\libraries\\gson-2.2.4.jar;..\\..\\libraries\\argo-2.25_fixed.jar;..\\..\\libraries\\codecjorbis-20101023.jar;..\\..\\libraries\\codecwav-20101023.jar;..\\..\\libraries\\libraryjavasound-20101123.jar;..\\..\\libraries\\librarylwjglopenal-20100824.jar;..\\..\\libraries\\soundsystem-20120107.jar\" ");
            fprintf(fp, "net.minecraft.client.main.Main ");
            fprintf(fp, "--username %s --version 1.6.4 --gameDir . --assetsDir assets ", username);
            fprintf(fp, "--accessToken 0 --userProperties {} --uuid %s --userType legacy\n", username);




        } else  if (strcmp(version, "1.12.2forge") == 0) {
            // Command for launching Minecraft 1.12.2 with Forge, exactly as provided
			fprintf(fp, "cd resources\\versions\\%s\n", version);
            fprintf(fp, "java -XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump -Xmx999m -XX:+UseConcMarkSweepGC -XX:+CMSIncrementalMode -XX:-UseAdaptiveSizePolicy -Xmn256m ");
            fprintf(fp, "-Djava.library.path=\"natives\" ");
            fprintf(fp, "-cp \"1.12.2.jar;..\\..\\libraries\\net\\minecraftforge\\forge\\1.12.2-14.23.5.2855\\forge-1.12.2-14.23.5.2855.jar;..\\..\\libraries\\org\\ow2\\asm\\asm-debug-all\\5.2\\asm-debug-all-5.2.jar;..\\..\\libraries\\net\\minecraft\\launchwrapper\\1.12\\launchwrapper-1.12.jar;..\\..\\libraries\\org\\jline\\jline\\3.5.1\\jline-3.5.1.jar;..\\..\\libraries\\com\\typesafe\\akka\\akka-actor_2.11\\2.3.3\\akka-actor_2.11-2.3.3.jar;..\\..\\libraries\\com\\typesafe\\config\\1.2.1\\config-1.2.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-actors-migration_2.11\\1.1.0\\scala-actors-migration_2.11-1.1.0.jar;..\\..\\libraries\\org\\scala-lang\\scala-compiler\\2.11.1\\scala-compiler-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\plugins\\scala-continuations-library_2.11\\1.0.2_mc\\scala-continuations-library_2.11-1.0.2_mc.jar;..\\..\\libraries\\org\\scala-lang\\plugins\\scala-continuations-plugin_2.11.1\\1.0.2_mc\\scala-continuations-plugin_2.11.1-1.0.2_mc.jar;..\\..\\libraries\\org\\scala-lang\\scala-library\\2.11.1\\scala-library-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-parser-combinators_2.11\\1.0.1\\scala-parser-combinators_2.11-1.0.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-reflect\\2.11.1\\scala-reflect-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-swing_2.11\\1.0.1\\scala-swing_2.11-1.0.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-xml_2.11\\1.0.2\\scala-xml_2.11-1.0.2.jar;..\\..\\libraries\\lzma\\lzma\\0.0.1\\lzma-0.0.1.jar;..\\..\\libraries\\java3d\\vecmath\\1.5.2\\vecmath-1.5.2.jar;..\\..\\libraries\\net\\sf\\trove4j\\trove4j\\3.0.3\\trove4j-3.0.3.jar;..\\..\\libraries\\org\\apache\\maven\\maven-artifact\\3.5.3\\maven-artifact-3.5.3.jar;..\\..\\libraries\\net\\sf\\jopt-simple\\jopt-simple\\5.0.3\\jopt-simple-5.0.3.jar;..\\..\\libraries\\com\\mojang\\patchy\\1.3.9\\patchy-1.3.9.jar;..\\..\\libraries\\oshi-project\\oshi-core\\1.1\\oshi-core-1.1.jar;..\\..\\libraries\\net\\java\\dev\\jna\\jna\\4.4.0\\jna-4.4.0.jar;..\\..\\libraries\\net\\java\\dev\\jna\\platform\\3.4.0\\platform-3.4.0.jar;..\\..\\libraries\\com\\ibm\\icu\\icu4j-core-mojang\\51.2\\icu4j-core-mojang-51.2.jar;..\\..\\libraries\\net\\sf\\jopt-simple\\jopt-simple\\5.0.3\\jopt-simple-5.0.3.jar;..\\..\\libraries\\com\\paulscode\\codecjorbis\\20101023\\codecjorbis-20101023.jar;..\\..\\libraries\\com\\paulscode\\codecwav\\20101023\\codecwav-20101023.jar;..\\..\\libraries\\com\\paulscode\\libraryjavasound\\20101123\\libraryjavasound-20101123.jar;..\\..\\libraries\\com\\paulscode\\librarylwjglopenal\\20100824\\librarylwjglopenal-20100824.jar;..\\..\\libraries\\com\\paulscode\\soundsystem\\20120107\\soundsystem-20120107.jar;..\\..\\libraries\\io\\netty\\netty-all\\4.1.9.Final\\netty-all-4.1.9.Final.jar;..\\..\\libraries\\com\\google\\guava\\guava\\21.0\\guava-21.0.jar;..\\..\\libraries\\org\\apache\\commons\\commons-lang3\\3.5\\commons-lang3-3.5.jar;..\\..\\libraries\\commons-io\\commons-io\\2.5\\commons-io-2.5.jar;..\\..\\libraries\\commons-codec\\commons-codec\\1.10\\commons-codec-1.10.jar;..\\..\\libraries\\net\\java\\jinput\\jinput\\2.0.5\\jinput-2.0.5.jar;..\\..\\libraries\\net\\java\\jutils\\jutils\\1.0.0\\jutils-1.0.0.jar;..\\..\\libraries\\com\\google\\code\\gson\\gson\\2.8.0\\gson-2.8.0.jar;..\\..\\libraries\\com\\mojang\\authlib\\1.5.25\\authlib-1.5.25.jar;..\\..\\libraries\\com\\mojang\\realms\\1.10.22\\realms-1.10.22.jar;..\\..\\libraries\\org\\apache\\commons\\commons-compress\\1.8.1\\commons-compress-1.8.1.jar;..\\..\\libraries\\org\\apache\\httpcomponents\\httpclient\\4.3.3\\httpclient-4.3.3.jar;..\\..\\libraries\\commons-logging\\commons-logging\\1.1.3\\commons-logging-1.1.3.jar;..\\..\\libraries\\org\\apache\\httpcomponents\\httpcore\\4.3.2\\httpcore-4.3.2.jar;..\\..\\libraries\\it\\unimi\\dsi\\fastutil\\7.1.0\\fastutil-7.1.0.jar;..\\..\\libraries\\org\\apache\\logging\\log4j\\log4j-api\\2.8.1\\log4j-api-2.8.1.jar;..\\..\\libraries\\org\\apache\\logging\\log4j\\log4j-core\\2.8.1\\log4j-core-2.8.1.jar;..\\..\\libraries\\org\\lwjgl\\lwjgl\\lwjgl\\2.9.4-nightly-20150209\\lwjgl-2.9.4-nightly-20150209.jar;..\\..\\libraries\\org\\lwjgl\\lwjgl\\lwjgl_util\\2.9.4-nightly-20150209\\lwjgl_util-2.9.4-nightly-20150209.jar;..\\..\\libraries\\com\\mojang\\text2speech\\1.10.3\\text2speech-1.10.3.jar\" ");
            fprintf(fp, "net.minecraft.launchwrapper.Launch ");
            fprintf(fp, "--username %s --version 1.12.2 --gameDir . --assetsDir assets --accessToken 0 --userProperties {} --uuid %s --userType legacy --width 1280 --height 720 --tweakClass net.minecraftforge.fml.common.launcher.FMLTweaker\n", username, username);
        
		} else if (strcmp(version, "1.12.2") == 0) {
    // Command for launching Minecraft 1.12.2 without Forge
	fprintf(fp, "cd resources\\versions\\%s\n", version);
    fprintf(fp, "java -XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump -Xmx999m -XX:+UseConcMarkSweepGC -XX:+CMSIncrementalMode -XX:-UseAdaptiveSizePolicy -Xmn256m ");
    fprintf(fp, "-Djava.library.path=\"natives\" ");
    fprintf(fp, "-cp \"1.12.2.jar;..\\..\\libraries\\org\\ow2\\asm\\asm-debug-all\\5.2\\asm-debug-all-5.2.jar;..\\..\\libraries\\net\\minecraft\\launchwrapper\\1.12\\launchwrapper-1.12.jar;..\\..\\libraries\\org\\jline\\jline\\3.5.1\\jline-3.5.1.jar;..\\..\\libraries\\com\\typesafe\\akka\\akka-actor_2.11\\2.3.3\\akka-actor_2.11-2.3.3.jar;..\\..\\libraries\\com\\typesafe\\config\\1.2.1\\config-1.2.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-actors-migration_2.11\\1.1.0\\scala-actors-migration_2.11-1.1.0.jar;..\\..\\libraries\\org\\scala-lang\\scala-compiler\\2.11.1\\scala-compiler-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\plugins\\scala-continuations-library_2.11\\1.0.2_mc\\scala-continuations-library_2.11-1.0.2_mc.jar;..\\..\\libraries\\org\\scala-lang\\plugins\\scala-continuations-plugin_2.11.1\\1.0.2_mc\\scala-continuations-plugin_2.11.1-1.0.2_mc.jar;..\\..\\libraries\\org\\scala-lang\\scala-library\\2.11.1\\scala-library-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-parser-combinators_2.11\\1.0.1\\scala-parser-combinators_2.11-1.0.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-reflect\\2.11.1\\scala-reflect-2.11.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-swing_2.11\\1.0.1\\scala-swing_2.11-1.0.1.jar;..\\..\\libraries\\org\\scala-lang\\scala-xml_2.11\\1.0.2\\scala-xml_2.11-1.0.2.jar;..\\..\\libraries\\lzma\\lzma\\0.0.1\\lzma-0.0.1.jar;..\\..\\libraries\\java3d\\vecmath\\1.5.2\\vecmath-1.5.2.jar;..\\..\\libraries\\net\\sf\\trove4j\\trove4j\\3.0.3\\trove4j-3.0.3.jar;..\\..\\libraries\\org\\apache\\maven\\maven-artifact\\3.5.3\\maven-artifact-3.5.3.jar;..\\..\\libraries\\net\\sf\\jopt-simple\\jopt-simple\\5.0.3\\jopt-simple-5.0.3.jar;..\\..\\libraries\\com\\mojang\\patchy\\1.3.9\\patchy-1.3.9.jar;..\\..\\libraries\\oshi-project\\oshi-core\\1.1\\oshi-core-1.1.jar;..\\..\\libraries\\net\\java\\dev\\jna\\jna\\4.4.0\\jna-4.4.0.jar;..\\..\\libraries\\net\\java\\dev\\jna\\platform\\3.4.0\\platform-3.4.0.jar;..\\..\\libraries\\com\\ibm\\icu\\icu4j-core-mojang\\51.2\\icu4j-core-mojang-51.2.jar;..\\..\\libraries\\net\\sf\\jopt-simple\\jopt-simple\\5.0.3\\jopt-simple-5.0.3.jar;..\\..\\libraries\\com\\paulscode\\codecjorbis\\20101023\\codecjorbis-20101023.jar;..\\..\\libraries\\com\\paulscode\\codecwav\\20101023\\codecwav-20101023.jar;..\\..\\libraries\\com\\paulscode\\libraryjavasound\\20101123\\libraryjavasound-20101123.jar;..\\..\\libraries\\com\\paulscode\\librarylwjglopenal\\20100824\\librarylwjglopenal-20100824.jar;..\\..\\libraries\\com\\paulscode\\soundsystem\\20120107\\soundsystem-20120107.jar;..\\..\\libraries\\io\\netty\\netty-all\\4.1.9.Final\\netty-all-4.1.9.Final.jar;..\\..\\libraries\\com\\google\\guava\\guava\\21.0\\guava-21.0.jar;..\\..\\libraries\\org\\apache\\commons\\commons-lang3\\3.5\\commons-lang3-3.5.jar;..\\..\\libraries\\commons-io\\commons-io\\2.5\\commons-io-2.5.jar;..\\..\\libraries\\commons-codec\\commons-codec\\1.10\\commons-codec-1.10.jar;..\\..\\libraries\\net\\java\\jinput\\jinput\\2.0.5\\jinput-2.0.5.jar;..\\..\\libraries\\net\\java\\jutils\\jutils\\1.0.0\\jutils-1.0.0.jar;..\\..\\libraries\\com\\google\\code\\gson\\gson\\2.8.0\\gson-2.8.0.jar;..\\..\\libraries\\com\\mojang\\authlib\\1.5.25\\authlib-1.5.25.jar;..\\..\\libraries\\com\\mojang\\realms\\1.10.22\\realms-1.10.22.jar;..\\..\\libraries\\org\\apache\\commons\\commons-compress\\1.8.1\\commons-compress-1.8.1.jar;..\\..\\libraries\\org\\apache\\httpcomponents\\httpclient\\4.3.3\\httpclient-4.3.3.jar;..\\..\\libraries\\commons-logging\\commons-logging\\1.1.3\\commons-logging-1.1.3.jar;..\\..\\libraries\\org\\apache\\httpcomponents\\httpcore\\4.3.2\\httpcore-4.3.2.jar;..\\..\\libraries\\it\\unimi\\dsi\\fastutil\\7.1.0\\fastutil-7.1.0.jar;..\\..\\libraries\\org\\apache\\logging\\log4j\\log4j-api\\2.8.1\\log4j-api-2.8.1.jar;..\\..\\libraries\\org\\apache\\logging\\log4j\\log4j-core\\2.8.1\\log4j-core-2.8.1.jar;..\\..\\libraries\\org\\lwjgl\\lwjgl\\lwjgl\\2.9.4-nightly-20150209\\lwjgl-2.9.4-nightly-20150209.jar;..\\..\\libraries\\org\\lwjgl\\lwjgl\\lwjgl_util\\2.9.4-nightly-20150209\\lwjgl_util-2.9.4-nightly-20150209.jar;..\\..\\libraries\\com\\mojang\\text2speech\\1.10.3\\text2speech-1.10.3.jar\" ");
    fprintf(fp, "net.minecraft.client.main.Main ");
    fprintf(fp, "--username %s --version 1.12.2 --gameDir . --assetsDir assets --accessToken 0 --userProperties {} --uuid %s --userType legacy --width 1280 --height 720\n", username, username);
}
 else {
            // Default command for other versions
            fprintf(fp, "cd resources\\versions\\%s\n", version);
            fprintf(fp, "java -Xms128M -Xmx512m -Djava.library.path=\"natives\" -cp \"%s.jar;../../libraries/*\" net.minecraft.client.main.Main ", version);
            fprintf(fp, "--username %s --version %s --gameDir . --assetsDir assets ", username, version);
            fprintf(fp, "--accessToken 0 --userProperties {} --uuid %s --userType legacy\n", username, version, username);
        }

        fclose(fp);
        system("run.bat");
        remove("run.bat");
    } else {
        LogError("Failed to create run.bat");
    }
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            if (g_hBitmap)
                DeleteObject(g_hBitmap);
            PostQuitMessage(0);
            return 0;
        case WM_COMMAND:
            if ((HWND)lParam == hButtonPlay) {
                char version[100];
                char username[100];

                // Get the selected version from the dropdown
                int index = SendMessage(hDropdownVersion, CB_GETCURSEL, 0, 0);
                SendMessage(hDropdownVersion, CB_GETLBTEXT, index, (LPARAM)version);

                // Get the username from the textbox
                GetWindowText(hTextboxUsername, username, sizeof(username));

                // Create and execute the batch file
                CreateBatFile(version, username);
            }
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (g_hBitmap) {
                HDC hdcImage = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcImage, g_hBitmap);
                BITMAP bm;
                GetObject(g_hBitmap, sizeof(bm), &bm);
                BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcImage, 0, 0, SRCCOPY);
                SelectObject(hdcImage, hOldBitmap);
                DeleteDC(hdcImage);
            }
            EndPaint(hwnd, &ps);
        }
        return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hBitmap = (HBITMAP)LoadImageA(NULL, "resources\\background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!g_hBitmap) {
        MessageBoxA(NULL, "Failed to load background image!", "Error", MB_ICONEXCLAMATION | MB_OK);
        LogError("Failed to load background image");
        return 0;
    }
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MyWindowClass";
    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        LogError("Window Registration Failed");
        return 0;
    }
    HWND hwnd = CreateWindowA(
        "MyWindowClass",
        "Win9x Minecraft Launcher A:1.2.1",
        WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              540,
                              400,
                              NULL,
                              NULL,
                              hInstance,
                              NULL
    );
    if (hwnd == NULL) {
        MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        LogError("Window Creation Failed");
        return 0;
    }

    // Create UI elements
    hLabelInfo = CreateWindow("STATIC", "Version A:1.2.2 Release Notes In this update I have made the Minecraft 9x Launcher more user friendly with a redesigned interface that includes a dropdown menu for selecting versions a central Play button and a username field at the bottom right The libraries folder is now in the main resources directory simplifying file management This update fully supports Minecraft 1.6.4 to 1.12.2. If anyone wishes to help with the project feel free to contact me on discord jtofexstinction ENJOY!", WS_VISIBLE | WS_CHILD , 95, 100, 350, 200, hwnd, NULL, hInstance, NULL);

    hDropdownVersion = CreateWindow("COMBOBOX", "Version", CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL, 0, 345, 150, 200, hwnd, NULL, hInstance, NULL);
	SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"1.12.2forge");
	SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"1.8.9");
    SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"1.7.10");
	SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"1.6.4");
    SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"1.5.2");
    SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"1.4.7");
    SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"1.2.5");
    SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"1.0");
    SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"beta1.7.3");
	SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"betahacks");
    SendMessage(hDropdownVersion, CB_ADDSTRING, 0, (LPARAM)"alpha1.1.2");
    

    // Set default selection to beta1.7.3
    SendMessage(hDropdownVersion, CB_SETCURSEL, 8, 0);

    hButtonPlay = CreateWindow("BUTTON", "Play", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 220, 345, 100, 30, hwnd, NULL, hInstance, NULL);

    hTextboxUsername = CreateWindow("EDIT", "Type username here", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER, 390, 345, 150, 30, hwnd, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
