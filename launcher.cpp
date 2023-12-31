#include <windows.h>
#include <stdio.h>

HBITMAP g_hBitmap = NULL;
HWND hButton173, hButton10, hButton125, hButton147, hButton152, hButton112, hButtonBetaHacks;
HWND hTextbox, hLabel;

void ReadInfoFile() {
    // As hInfoTextbox is not used, you might want to replace it with the appropriate handling
}

void CreateBatFile(const char *version, const char *additionalText) {
    FILE *fp = fopen("run.bat", "w");
    if (fp) {
        fprintf(fp, "cd resources\n");
        fprintf(fp, "cd versions\n");
        fprintf(fp, "cd %s\n", version);
        fprintf(fp, "java -Xms1M -Xmx999M -Djava.library.path=natives/ -cp \"minecraft.jar;lwjgl.jar;lwjgl_util.jar\" net.minecraft.client.Minecraft ");
        fprintf(fp, "%s\n", additionalText);
        fclose(fp);
        system("run.bat");
        remove("run.bat");
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
            {
                char szText[100];
                GetWindowText(hTextbox, szText, sizeof(szText));
                if ((HWND)lParam == hButton173) {
                    CreateBatFile("beta1.7.3", szText);
                } else if ((HWND)lParam == hButton10) {
                    CreateBatFile("1.0", szText);
                } else if ((HWND)lParam == hButton125) {
                    CreateBatFile("1.2.5", szText);
                } else if ((HWND)lParam == hButton147) {
                    CreateBatFile("1.4.7", szText);
                } else if ((HWND)lParam == hButton152) {
                    CreateBatFile("1.5.2", szText);
                } else if ((HWND)lParam == hButton112) {
                    CreateBatFile("alpha1.1.2", szText);
                } else if ((HWND)lParam == hButtonBetaHacks) {
                    CreateBatFile("betahacks", szText);
                }
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
        return 0;
    }
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MyWindowClass";
    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    HWND hwnd = CreateWindowA(
        "MyWindowClass",
        "Win9x Minecraft Launcher A1.02",
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
        return 0;
    }
    hButton173 = CreateWindow("BUTTON", "Play B1.7.3", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 70, 100, 30, hwnd, NULL, hInstance, NULL);
    hButton10 = CreateWindow("BUTTON", "Play 1.0", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 110, 100, 30, hwnd, NULL, hInstance, NULL);
    hButton125 = CreateWindow("BUTTON", "Play 1.2.5", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 150, 100, 30, hwnd, NULL, hInstance, NULL);
    hButton147 = CreateWindow("BUTTON", "Play 1.4.7", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 190, 100, 30, hwnd, NULL, hInstance, NULL);
    hButton152 = CreateWindow("BUTTON", "Play 1.5.2", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 230, 100, 30, hwnd, NULL, hInstance, NULL);
    hButton112 = CreateWindow("BUTTON", "Play A1.1.2", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 270, 100, 30, hwnd, NULL, hInstance, NULL);
    hButtonBetaHacks = CreateWindow("BUTTON", "Play B-Hacks", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 310, 100, 30, hwnd, NULL, hInstance, NULL);
    hTextbox = CreateWindow("EDIT", "Type username here", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER, 120, 70, 400, 30, hwnd, NULL, hInstance, NULL);
    hLabel = CreateWindow("STATIC", "Win9x Mincraft Launcher Release: A1.02                               This is very early and of course very simple software. There are currently only 7 versions of minecraft included. You can of course replace minecraft.jar with any jar you want. before playing you must install java and kernelx. I reccomend Java 5 update 1 for best performence on windows 98. This launcher is not intended to be used on modern versions of windows. sound will not work there are far better options such as Betacraft.That said when ready simply type your user name into the top box, and play your selected version", WS_VISIBLE | WS_CHILD | SS_LEFT, 120, 110, 400, 230, hwnd, NULL, hInstance, NULL);
    ReadInfoFile();
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
