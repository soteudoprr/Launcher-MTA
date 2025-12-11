#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdio.h>
#include <string.h>

DWORD GetMTAPID() {
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, "Multi Theft Auto.exe") == 0) {
                DWORD pid = pe.th32ProcessID;
                CloseHandle(snap);
                return pid;
            }
        } while (Process32Next(snap, &pe));
    }

    CloseHandle(snap);
    return 0;
}

// ===========================
//  OVERLAY 100% TRANSPARENTE
// ===========================

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            HFONT font = CreateFontA(
                16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE,
                "Tahoma"
            );

            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            SetTextColor(hdc, RGB(255, 60, 60));
            SetBkMode(hdc, TRANSPARENT);

            RECT r;
            GetClientRect(hwnd, &r);

            r.left   = r.right - 200;
            r.top    = r.bottom - 30;

            DrawTextA(
                hdc,
                "SouteX Launcher",
                -1,
                &r,
                DT_RIGHT | DT_BOTTOM | DT_SINGLELINE
            );

            SelectObject(hdc, oldFont);
            DeleteObject(font);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, w, l);
}

HWND CriarOverlay() {
    const char *CLASS = "SOUTEX_OVERLAY";

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

    if (!RegisterClassA(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            return NULL;
        }
    }

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST |
        WS_EX_LAYERED |
        WS_EX_TRANSPARENT |
        WS_EX_TOOLWINDOW,
        CLASS,
        "SouteX Overlay",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (hwnd == NULL) {
        return NULL;
    }

    // Transparência com RGB(0,0,0) como cor chave + 255 de opacidade para o conteúdo
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY | LWA_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    return hwnd;
}

// ===========================

void AnimarOtimizacao() {
    const int total = 40;

    for (int p = 1; p <= 100; p++) {
        int filled = (p * total) / 100;

        printf("\rOtimizando [");
        for (int i = 0; i < filled; i++) printf("#");
        for (int i = filled; i < total; i++) printf("-");
        printf("] %d%%", p);

        fflush(stdout);
        Sleep(25);
    }
    printf("\n\n");
}

int main() {
    printf("\tLauncher Desenvolvido Por SouteX\n");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("--------------------------------------------------------------------------\n");
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    printf("\n[+] Start Game...\n");

    // Abrir MTA
    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        "C:\\Program Files (x86)\\MTA San Andreas 1.6\\Multi Theft Auto.exe",
        NULL, NULL, SW_SHOW
    );

    if ((INT_PTR)result <= 32) {
        printf("[!] Erro ao abrir o jogo. Codigo: %d\n", (int)(INT_PTR)result);
        printf("[!] Verifique se o caminho esta correto.\n");
        system("pause");
        return 1;
    }

    Sleep(2000);
    printf("\n");

    DWORD pid = 0;
    int tentativas = 0;
    while (pid == 0 && tentativas < 60) {
        pid = GetMTAPID();
        Sleep(500);
        tentativas++;
    }

    if (pid == 0) {
        printf("[!] Timeout: Jogo nao iniciou em 30 segundos.\n");
        system("pause");
        return 1;
    }

    printf("[+] Game Started\n");
    printf("[=] PID: %lu\n\n", pid);

    AnimarOtimizacao();

    // Esconder terminal completamente
    HWND console = GetConsoleWindow();
    if (console) {
        ShowWindow(console, SW_HIDE);
    }

    // Criar overlay transparente
    HWND overlay = CriarOverlay();
    if (!overlay) {
        printf("[!] Erro ao criar overlay\n");
    }

    // Loop até o jogo fechar
    while (1) {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProc) break;

        DWORD exitCode = 0;
        GetExitCodeProcess(hProc, &exitCode);
        CloseHandle(hProc);

        if (exitCode != STILL_ACTIVE) break;

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Sleep(100);
    }

    if (overlay) {
        DestroyWindow(overlay);
    }
    
    printf("\n[+] Jogo fechado. Encerrando launcher...\n");
    return 0;
}