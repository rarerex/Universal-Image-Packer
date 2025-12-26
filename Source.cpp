#include <windows.h>

#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <shlwapi.h>

using namespace Gdiplus;

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")

//tab 1 button inputs
#define IDM_OPEN_IMAGE     40001
#define IDM_RED_CHANNEL    40002
#define IDM_GREEN_CHANNEL  40003
#define IDM_BLUE_CHANNEL   40004
#define IDM_ALPHA_CHANNEL  40005
#define IDM_DOWNLOAD       40006
#define IDM_RESTORE_COLOR  40007

//tab 2 button inputs
#define IDM_AO_INPUT       50001
#define IDM_ROUGH_INPUT    50002
#define IDM_METAL_INPUT    50003
#define IDM_BUILD_AORM     50004
#define IDM_CLEAR_AORM     50005
#define IDM_DOWNLOAD_AORM  50006

//tab 3 button inputs
#define IDM_UR_INPUT        60001
#define IDM_UG_INPUT        60002
#define IDM_UB_INPUT        60003
#define IDM_UA_INPUT        60004
#define IDM_BUILD_UNITY     60005
#define IDM_CLEAR_UNITY     60006
#define IDM_DOWNLOAD_UNITY  60007
#define IDM_INVERT_SMOOTHNESS 60008

HINSTANCE hInst;
ULONG_PTR gdiplusToken;

//tab headers
HWND hTab;
HWND hTabPage1;
HWND hTabPage2;
HWND hTabPage3;
HWND hTabPage4;

//tab 1 buttons
HWND hButtonOpen;
HWND hButtonRestore;
HWND hButtonRed;
HWND hButtonGreen;
HWND hButtonBlue;
HWND hButtonAlpha;
HWND hButtonDownload;

//tab 2 buttons
HWND hButtonAO;
HWND hButtonRough;
HWND hButtonMetal;
HWND hButtonBuildAORM;
HWND hButtonClear;
HWND hButtonDownloadAORM;
//tab 2 labels
HWND hLabelAO;
HWND hLabelRough;
HWND hLabelMetal;

//tab 1 state
Image* gImage = nullptr;
Bitmap* gChannelImage = nullptr;
WCHAR gCurrentFile[MAX_PATH] = L"";
int gLastChannel = -1;

//tab 3 buttons
HWND hButtonUR;
HWND hButtonUG;
HWND hButtonUB;
HWND hButtonUA;
HWND hButtonBuildUnity;
HWND hButtonClearUnity;
HWND hButtonDownloadUnity;
HWND hButtonInvertSmoothness;
//tab 3 labels
HWND hLabelUR;
HWND hLabelUG;
HWND hLabelUB;
HWND hLabelUA;
HWND hLabelInvertSmoothness;

//aorm bitmaps
Bitmap* gAO = nullptr;
Bitmap* gRough = nullptr;
Bitmap* gMetal = nullptr;
Bitmap* gAORM = nullptr;

//unity bitmaps
Bitmap* gUnityR = nullptr;
Bitmap* gUnityG = nullptr;
Bitmap* gUnityB = nullptr;
Bitmap* gUnityA = nullptr;
Bitmap* gUnityMask = nullptr;
bool gInvertSmoothness = false;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TabPageProc(HWND, UINT, WPARAM, LPARAM);

//helpers
void ShowChannelButtons(BOOL show)
{
    ShowWindow(hButtonRestore, show ? SW_SHOW : SW_HIDE);
    ShowWindow(hButtonRed, show ? SW_SHOW : SW_HIDE);
    ShowWindow(hButtonGreen, show ? SW_SHOW : SW_HIDE);
    ShowWindow(hButtonBlue, show ? SW_SHOW : SW_HIDE);
    ShowWindow(hButtonAlpha, show ? SW_SHOW : SW_HIDE);
    ShowWindow(hButtonDownload, show ? SW_SHOW : SW_HIDE);
}

void LoadImageFromPath(const WCHAR* path)
{
    delete gImage;
    delete gChannelImage;
    gChannelImage = nullptr;
    gLastChannel = -1;

    gImage = Image::FromFile(path);
    if (!gImage || gImage->GetLastStatus() != Ok)
    {
        delete gImage;
        gImage = nullptr;
        return;
    }

    wcscpy_s(gCurrentFile, path);
    ShowChannelButtons(TRUE);
    InvalidateRect(hTabPage1, nullptr, TRUE);
}

Bitmap* LoadBitmapFile(HWND owner, HWND label)
{
    OPENFILENAMEW ofn = {};
    WCHAR file[MAX_PATH] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Images\0*.png;*.jpg;*.jpeg;*.bmp\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameW(&ofn)) return nullptr;

    Bitmap* bmp = Bitmap::FromFile(file);
    if (!bmp || bmp->GetLastStatus() != Ok)
    {
        delete bmp;
        return nullptr;
    }

    if (label)
    {
        WCHAR fname[MAX_PATH];
        PathStripPathW(file);
        wcscpy_s(fname, file);
        SetWindowTextW(label, fname);
    }
    return bmp;
}

//channel extraction
void ApplyChannel(int channel)
{
    if (!gImage) return;

    gLastChannel = channel;
    delete gChannelImage;

    Bitmap* src = (Bitmap*)gImage;
    int w = src->GetWidth();
    int h = src->GetHeight();

    gChannelImage = new Bitmap(w, h, PixelFormat32bppARGB);

    BitmapData s, d;
    Rect r(0, 0, w, h);

    src->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &s);
    gChannelImage->LockBits(&r, ImageLockModeWrite, PixelFormat32bppARGB, &d);

    BYTE* ps = (BYTE*)s.Scan0;
    BYTE* pd = (BYTE*)d.Scan0;

    for (int y = 0; y < h; y++)
    {
        BYTE* rs = ps + y * s.Stride;
        BYTE* rd = pd + y * d.Stride;

        for (int x = 0; x < w; x++)
        {
            BYTE b = rs[x * 4 + 0];
            BYTE g = rs[x * 4 + 1];
            BYTE rC = rs[x * 4 + 2];
            BYTE a = rs[x * 4 + 3];

            rd[x * 4 + 0] = (channel == 2) ? b : 0;
            rd[x * 4 + 1] = (channel == 1) ? g : 0;
            rd[x * 4 + 2] = (channel == 0) ? rC : 0;
            rd[x * 4 + 3] = a;
        }
    }

    src->UnlockBits(&s);
    gChannelImage->UnlockBits(&d);

    InvalidateRect(hTabPage1, nullptr, TRUE);
}

//validation helpers
void ShowWarning(HWND owner, const wchar_t* text)
{
    MessageBoxW(
        owner,
        text,
        L"Warning",
        MB_OK | MB_ICONWARNING
    );
}

bool SameResolution(Bitmap* a, Bitmap* b)
{
    if (!a || !b) return true;
    return a->GetWidth() == b->GetWidth() &&
        a->GetHeight() == b->GetHeight();
}

//image helpers
void OpenImage(HWND owner)
{
    OPENFILENAMEW ofn = {};
    WCHAR file[MAX_PATH] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Images\0*.png;*.jpg;*.jpeg;*.bmp\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
        LoadImageFromPath(file);
}

void SaveImage(HWND owner)
{
    Bitmap* bmp = nullptr;
    WCHAR suffix[32] = L"_original";
    WCHAR base[MAX_PATH] = L"";

    // Tab 1: Extract Channels
    if (IsWindowVisible(hTabPage1) && gImage)
    {
        bmp = gChannelImage ? gChannelImage : (Bitmap*)gImage;

        switch (gLastChannel)
        {
        case 0: wcscpy_s(suffix, L"_red"); break;
        case 1: wcscpy_s(suffix, L"_green"); break;
        case 2: wcscpy_s(suffix, L"_blue"); break;
        case 3: wcscpy_s(suffix, L"_alpha"); break;
        default: wcscpy_s(suffix, L"_original"); break;
        }

        wcscpy_s(base, gCurrentFile);
        PathRemoveExtensionW(base);
    }
    // Tab 2: Pack AORM
    else if (IsWindowVisible(hTabPage2) && gAORM)
    {
        bmp = gAORM;
        wcscpy_s(suffix, L"_aorm");
        wcscpy_s(base, L"AORM_map");
    }
    // Tab 3: Pack Unity Map
    else if (IsWindowVisible(hTabPage3) && gUnityMask)
    {
        bmp = gUnityMask;
        wcscpy_s(suffix, L"_unitymap");
        wcscpy_s(base, L"UnityMask");
    }
    else
    {
        return; // Nothing to save
    }

    WCHAR out[MAX_PATH];
    wsprintfW(out, L"%s%s.png", base, suffix);

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"PNG Files\0*.png\0";
    ofn.lpstrFile = out;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn)) return;

    CLSID clsid;
    if (FAILED(CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &clsid))) return;

    bmp->Save(out, &clsid, nullptr);
}

//build aorm
void BuildAORM()
{
    if (!gAO && !gRough && !gMetal) return;

    //check for inconsistent image resolutions
    Bitmap* ref =
        gAO ? gAO :
        gRough ? gRough :
        gMetal;

    if (!SameResolution(ref, gAO) ||
        !SameResolution(ref, gRough) ||
        !SameResolution(ref, gMetal))
    {
        ShowWarning(
            hTabPage2,
            L"All images must be the same resolution"
        );
        return;
    }

    int w = gAO ? gAO->GetWidth() : (gRough ? gRough->GetWidth() : gMetal->GetWidth());
    int h = gAO ? gAO->GetHeight() : (gRough ? gRough->GetHeight() : gMetal->GetHeight());

    delete gAORM;
    gAORM = new Bitmap(w, h, PixelFormat32bppARGB);

    BitmapData d, aoD, roughD, metalD;
    Rect r(0, 0, w, h);

    gAORM->LockBits(&r, ImageLockModeWrite, PixelFormat32bppARGB, &d);
    if (gAO) gAO->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &aoD);
    if (gRough) gRough->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &roughD);
    if (gMetal) gMetal->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &metalD);

    for (int y = 0; y < h; y++)
    {
        BYTE* pd = (BYTE*)d.Scan0 + y * d.Stride;
        BYTE* pa = gAO ? (BYTE*)aoD.Scan0 + y * aoD.Stride : nullptr;
        BYTE* pr = gRough ? (BYTE*)roughD.Scan0 + y * roughD.Stride : nullptr;
        BYTE* pm = gMetal ? (BYTE*)metalD.Scan0 + y * metalD.Stride : nullptr;

        for (int x = 0; x < w; x++)
        {
            pd[x * 4 + 0] = pm ? pm[x * 4 + 0] : 0;
            pd[x * 4 + 1] = pr ? pr[x * 4 + 0] : 0;
            pd[x * 4 + 2] = pa ? pa[x * 4 + 0] : 0;
            pd[x * 4 + 3] = 255;
        }
    }

    gAORM->UnlockBits(&d);
    if (gAO) gAO->UnlockBits(&aoD);
    if (gRough) gRough->UnlockBits(&roughD);
    if (gMetal) gMetal->UnlockBits(&metalD);

    InvalidateRect(hTabPage2, nullptr, TRUE);
}

//clear aorm
void ClearAORM()
{
    delete gAO; gAO = nullptr;
    delete gRough; gRough = nullptr;
    delete gMetal; gMetal = nullptr;
    delete gAORM; gAORM = nullptr;

    SetWindowTextW(hLabelAO, L"");
    SetWindowTextW(hLabelRough, L"");
    SetWindowTextW(hLabelMetal, L"");

    InvalidateRect(hTabPage2, nullptr, TRUE);
}

//unity roughness invert label
void UpdateInvertSmoothnessLabel()
{
    if (!hLabelInvertSmoothness) return;

    if (gInvertSmoothness)
        SetWindowTextW(hLabelInvertSmoothness, L"Inverted");
    else
        SetWindowTextW(hLabelInvertSmoothness, L"Default");
}

//build unity
void BuildUnityMask()
{
    if (!gUnityR && !gUnityG && !gUnityB && !gUnityA) return;

    int w = gUnityR ? gUnityR->GetWidth() : gUnityG ? gUnityG->GetWidth() : gUnityB ? gUnityB->GetWidth() :gUnityA->GetWidth();
    int h = gUnityR ? gUnityR->GetHeight() :gUnityG ? gUnityG->GetHeight() : gUnityB ? gUnityB->GetHeight() : gUnityA->GetHeight();

    delete gUnityMask;
    gUnityMask = new Bitmap(w, h, PixelFormat32bppARGB);

    BitmapData d, rD, gD, bD, aD;
    Rect rc(0, 0, w, h);

    //check for inconsistent image resolutions
    Bitmap* ref =
        gUnityR ? gUnityR :
        gUnityG ? gUnityG :
        gUnityB ? gUnityB :
        gUnityA;

    if (!SameResolution(ref, gUnityR) ||
        !SameResolution(ref, gUnityG) ||
        !SameResolution(ref, gUnityB) ||
        !SameResolution(ref, gUnityA))
    {
        ShowWarning(
            hTabPage3,
            L"All images must be the same resolution"
        );
        return;
    }

    gUnityMask->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &d);
    if (gUnityR) gUnityR->LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &rD);
    if (gUnityG) gUnityG->LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &gD);
    if (gUnityB) gUnityB->LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &bD);
    if (gUnityA) gUnityA->LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &aD);

    for (int y = 0; y < h; y++)
    {
        BYTE* pd = (BYTE*)d.Scan0 + y * d.Stride;
        BYTE* pr = gUnityR ? (BYTE*)rD.Scan0 + y * rD.Stride : nullptr;
        BYTE* pg = gUnityG ? (BYTE*)gD.Scan0 + y * gD.Stride : nullptr;
        BYTE* pb = gUnityB ? (BYTE*)bD.Scan0 + y * bD.Stride : nullptr;
        BYTE* pa = gUnityA ? (BYTE*)aD.Scan0 + y * aD.Stride : nullptr;

        for (int x = 0; x < w; x++)
        {
            pd[x * 4 + 2] = pr ? pr[x * 4] : 0;
            pd[x * 4 + 1] = pg ? pg[x * 4] : 0;
            pd[x * 4 + 0] = pb ? pb[x * 4] : 0;
            BYTE alpha = pa ? pa[x * 4] : 255;
            if (gInvertSmoothness) alpha = 255 - alpha;
            pd[x * 4 + 3] = alpha;
        }
    }

    gUnityMask->UnlockBits(&d);
    if (gUnityR) gUnityR->UnlockBits(&rD);
    if (gUnityG) gUnityG->UnlockBits(&gD);
    if (gUnityB) gUnityB->UnlockBits(&bD);
    if (gUnityA) gUnityA->UnlockBits(&aD);

    InvalidateRect(hTabPage3, nullptr, TRUE);
}

//clear unity
void ClearUnity()
{
    delete gUnityR; gUnityR = nullptr;
    delete gUnityG; gUnityG = nullptr;
    delete gUnityB; gUnityB = nullptr;
    delete gUnityA; gUnityA = nullptr;
    delete gUnityMask; gUnityMask = nullptr;

    gInvertSmoothness = false;
    UpdateInvertSmoothnessLabel();

    SetWindowTextW(hLabelUR, L"");
    SetWindowTextW(hLabelUG, L"");
    SetWindowTextW(hLabelUB, L"");
    SetWindowTextW(hLabelUA, L"");

    InvalidateRect(hTabPage3, nullptr, TRUE);
}

//painting
LRESULT CALLBACK TabPageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {

    case WM_COMMAND:
        SendMessage(GetParent(GetParent(hWnd)), msg, wParam, lParam);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        Graphics g(hdc);

        if (hWnd == hTabPage1)
        {
            if (gImage || gChannelImage)
            {
                Bitmap* img = gChannelImage ? gChannelImage : (Bitmap*)gImage;
                int left = 180, m = 20;
                int aw = rc.right - left - m * 2;
                int ah = rc.bottom - m * 2;
                float sx = (float)aw / img->GetWidth();
                float sy = (float)ah / img->GetHeight();
                float s = sx < sy ? sx : sy;
                int dw = (int)(img->GetWidth() * s);
                int dh = (int)(img->GetHeight() * s);
                g.DrawImage(img, left + (aw - dw) / 2, (ah - dh) / 2, dw, dh);
            }
        }

        if (hWnd == hTabPage2)
        {
            if (gAORM)
            {
                int left = 180, m = 20;
                int aw = rc.right - left - m * 2;
                int ah = rc.bottom - m * 2;
                float sx = (float)aw / gAORM->GetWidth();
                float sy = (float)ah / gAORM->GetHeight();
                float s = sx < sy ? sx : sy;
                int dw = (int)(gAORM->GetWidth() * s);
                int dh = (int)(gAORM->GetHeight() * s);
                g.DrawImage(gAORM, left + (aw - dw) / 2, (ah - dh) / 2, dw, dh);
            }
        }

        if (hWnd == hTabPage3)
        {
            if (gUnityMask)
            {
                int left = 180, m = 20;
                int aw = rc.right - left - m * 2;
                int ah = rc.bottom - m * 2;
                float sx = (float)aw / gUnityMask->GetWidth();
                float sy = (float)ah / gUnityMask->GetHeight();
                float s = sx < sy ? sx : sy;
                int dw = (int)(gUnityMask->GetWidth() * s);
                int dh = (int)(gUnityMask->GetHeight() * s);
                g.DrawImage(gUnityMask,
                    left + (aw - dw) / 2,
                    (ah - dh) / 2,
                    dw, dh);
            }
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

//main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_OPEN_IMAGE: OpenImage(hWnd); break;
        case IDM_RESTORE_COLOR:
            delete gChannelImage;
            gChannelImage = nullptr;
            gLastChannel = -1;
            InvalidateRect(hTabPage1, nullptr, TRUE);
            break;

        //tab 1 button router
        case IDM_RED_CHANNEL: ApplyChannel(0); break;
        case IDM_GREEN_CHANNEL: ApplyChannel(1); break;
        case IDM_BLUE_CHANNEL: ApplyChannel(2); break;
        case IDM_ALPHA_CHANNEL: ApplyChannel(3); break;
        case IDM_DOWNLOAD: SaveImage(hWnd); break;

        //tab 2 button router
        case IDM_AO_INPUT: delete gAO; gAO = LoadBitmapFile(hWnd, hLabelAO); break;
        case IDM_ROUGH_INPUT: delete gRough; gRough = LoadBitmapFile(hWnd, hLabelRough); break;
        case IDM_METAL_INPUT: delete gMetal; gMetal = LoadBitmapFile(hWnd, hLabelMetal); break;
        case IDM_BUILD_AORM: BuildAORM(); break;
        case IDM_CLEAR_AORM: ClearAORM(); break;
        case IDM_DOWNLOAD_AORM: SaveImage(hWnd); break;

        //tab 3 button router
        case IDM_UR_INPUT: delete gUnityR; gUnityR = LoadBitmapFile(hWnd, hLabelUR); break;
        case IDM_UG_INPUT: delete gUnityG; gUnityG = LoadBitmapFile(hWnd, hLabelUG); break;
        case IDM_UB_INPUT: delete gUnityB; gUnityB = LoadBitmapFile(hWnd, hLabelUB); break;
        case IDM_UA_INPUT: delete gUnityA; gUnityA = LoadBitmapFile(hWnd, hLabelUA); break;
        case IDM_BUILD_UNITY: BuildUnityMask(); break;
        case IDM_CLEAR_UNITY: ClearUnity(); break;
        case IDM_DOWNLOAD_UNITY: SaveImage(hWnd); break;
        case IDM_INVERT_SMOOTHNESS: gInvertSmoothness = !gInvertSmoothness; UpdateInvertSmoothnessLabel(); BuildUnityMask(); break;
        }
        break;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->hwndFrom == hTab &&
            ((LPNMHDR)lParam)->code == TCN_SELCHANGE)
        {
            int i = TabCtrl_GetCurSel(hTab);

            ShowWindow(hTabPage1, i == 0 ? SW_SHOW : SW_HIDE);
            ShowWindow(hTabPage2, i == 1 ? SW_SHOW : SW_HIDE);
            ShowWindow(hTabPage3, i == 2 ? SW_SHOW : SW_HIDE);
            ShowWindow(hTabPage4, i == 3 ? SW_SHOW : SW_HIDE);

            InvalidateRect(hTabPage1, nullptr, TRUE);
            InvalidateRect(hTabPage2, nullptr, TRUE);
            InvalidateRect(hTabPage3, nullptr, TRUE);
            InvalidateRect(hTabPage4, nullptr, TRUE);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

//entry
int WINAPI wWinMain(HINSTANCE h, HINSTANCE, LPWSTR, int n)
{
    hInst = h;

    GdiplusStartupInput gdi;
    GdiplusStartup(&gdiplusToken, &gdi, nullptr);

    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_TAB_CLASSES};
    InitCommonControlsEx(&ic);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = h;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MainWnd";
    RegisterClassExW(&wc);

    wc.lpfnWndProc = TabPageProc;
    wc.lpszClassName = L"TabPage";
    RegisterClassExW(&wc);

    HWND wnd = CreateWindowW(L"MainWnd", L"Universal Image Packer", WS_OVERLAPPEDWINDOW,
        100, 100, 900, 600, nullptr, nullptr, h, nullptr);

    hTab = CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE,
        0, 0, 900, 600, wnd, nullptr, h, nullptr);

    RECT rcWnd;
    GetWindowRect(wnd, &rcWnd);

    int wndW = rcWnd.right - rcWnd.left;
    int wndH = rcWnd.bottom - rcWnd.top;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    SetWindowPos(
        wnd,
        nullptr,
        (screenW - wndW) / 2,
        (screenH - wndH) / 2,
        0,
        0,
        SWP_NOZORDER | SWP_NOSIZE
    );

    TCITEMW ti = { TCIF_TEXT };
    ti.pszText = (LPWSTR)L"Extract Channels";
    TabCtrl_InsertItem(hTab, 0, &ti);

    ti.pszText = (LPWSTR)L"Pack AORM";
    TabCtrl_InsertItem(hTab, 1, &ti);

    ti.pszText = (LPWSTR)L"Pack Unity Map";
    TabCtrl_InsertItem(hTab, 2, &ti);

    ti.pszText = (LPWSTR)L"Help";
    TabCtrl_InsertItem(hTab, 3, &ti);

    RECT rc;
    GetClientRect(hTab, &rc);
    TabCtrl_AdjustRect(hTab, FALSE, &rc);

    hTabPage1 = CreateWindowW(L"TabPage", nullptr, WS_CHILD | WS_VISIBLE,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hTab, nullptr, h, nullptr);

    hTabPage2 = CreateWindowW(L"TabPage", nullptr, WS_CHILD,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hTab, nullptr, h, nullptr);

    hTabPage3 = CreateWindowW(L"TabPage", nullptr, WS_CHILD,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hTab, nullptr, h, nullptr);

    hTabPage4 = CreateWindowW(L"TabPage", nullptr, WS_CHILD,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hTab, nullptr, h, nullptr);

    int x = 10, y = 20, w = 150, hB = 30, s = 60;

    //tab 1 buttons
    hButtonOpen = CreateWindowW(L"BUTTON", L"Open Image", WS_CHILD | WS_VISIBLE, x, y, w, hB, hTabPage1, (HMENU)IDM_OPEN_IMAGE, h, nullptr);
    hButtonRestore = CreateWindowW(L"BUTTON", L"Restore Color", WS_CHILD, x, y + s, w, hB, hTabPage1, (HMENU)IDM_RESTORE_COLOR, h, nullptr);
    hButtonRed = CreateWindowW(L"BUTTON", L"Red Channel", WS_CHILD, x, y + s * 2, w, hB, hTabPage1, (HMENU)IDM_RED_CHANNEL, h, nullptr);
    hButtonGreen = CreateWindowW(L"BUTTON", L"Green Channel", WS_CHILD, x, y + s * 3, w, hB, hTabPage1, (HMENU)IDM_GREEN_CHANNEL, h, nullptr);
    hButtonBlue = CreateWindowW(L"BUTTON", L"Blue Channel", WS_CHILD, x, y + s * 4, w, hB, hTabPage1, (HMENU)IDM_BLUE_CHANNEL, h, nullptr);
    hButtonAlpha = CreateWindowW(L"BUTTON", L"Alpha Channel", WS_CHILD, x, y + s * 5, w, hB, hTabPage1, (HMENU)IDM_ALPHA_CHANNEL, h, nullptr);
    hButtonDownload = CreateWindowW(L"BUTTON", L"Download", WS_CHILD, x, y + s * 6, w, hB, hTabPage1, (HMENU)IDM_DOWNLOAD, h, nullptr);
    ShowChannelButtons(FALSE);

    //tab 2 buttons + labels + tooltips
    hButtonAO = CreateWindowW(L"BUTTON", L"Load AO", WS_CHILD | WS_VISIBLE, x, y, w, hB, hTabPage2, (HMENU)IDM_AO_INPUT, h, nullptr);
    hLabelAO = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y + hB + 5, w, 20, hTabPage2, nullptr, h, nullptr);
    hButtonRough = CreateWindowW(L"BUTTON", L"Load Roughness", WS_CHILD | WS_VISIBLE, x, y + s, w, hB, hTabPage2, (HMENU)IDM_ROUGH_INPUT, h, nullptr);
    hLabelRough = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y + s + hB + 5, w, 20, hTabPage2, nullptr, h, nullptr);
    hButtonMetal = CreateWindowW(L"BUTTON", L"Load Metalness", WS_CHILD | WS_VISIBLE, x, y + s * 2, w, hB, hTabPage2, (HMENU)IDM_METAL_INPUT, h, nullptr);
    hLabelMetal = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y + s * 2 + hB + 5, w, 20, hTabPage2, nullptr, h, nullptr);
    hButtonBuildAORM = CreateWindowW(L"BUTTON", L"Build AORM", WS_CHILD | WS_VISIBLE, x, y + s * 3, w, hB, hTabPage2, (HMENU)IDM_BUILD_AORM, h, nullptr);
    hButtonClear = CreateWindowW(L"BUTTON", L"Clear", WS_CHILD | WS_VISIBLE, x, y + s * 4, w, hB, hTabPage2, (HMENU)IDM_CLEAR_AORM, h, nullptr);
    hButtonDownloadAORM = CreateWindowW(L"BUTTON", L"Download", WS_CHILD | WS_VISIBLE, x, y + s * 5, w, hB, hTabPage2, (HMENU)IDM_DOWNLOAD_AORM, h, nullptr);

    //tab 3 buttons + labels
    hButtonUR = CreateWindowW(L"BUTTON", L"Load Metallic", WS_CHILD | WS_VISIBLE, x, y, w, hB, hTabPage3, (HMENU)IDM_UR_INPUT, h, nullptr);
    hLabelUR = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y + hB + 5, w, 20, hTabPage3, nullptr, h, nullptr);
    hButtonUG = CreateWindowW(L"BUTTON", L"Load AO", WS_CHILD | WS_VISIBLE, x, y + s, w, hB, hTabPage3, (HMENU)IDM_UG_INPUT, h, nullptr);
    hLabelUG = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y + s + hB + 5, w, 20, hTabPage3, nullptr, h, nullptr);
    hButtonUB = CreateWindowW(L"BUTTON", L"Load Detail", WS_CHILD | WS_VISIBLE, x, y + s * 2, w, hB, hTabPage3, (HMENU)IDM_UB_INPUT, h, nullptr);
    hLabelUB = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y + s * 2 + hB + 5, w, 20, hTabPage3, nullptr, h, nullptr);
    hButtonUA = CreateWindowW(L"BUTTON", L"Load Smoothness", WS_CHILD | WS_VISIBLE, x, y + s * 3, w, hB, hTabPage3, (HMENU)IDM_UA_INPUT, h, nullptr);
    hLabelUA = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y + s * 3 + hB + 5, w, 20, hTabPage3, nullptr, h, nullptr);
    hButtonBuildUnity = CreateWindowW(L"BUTTON", L"Build Map", WS_CHILD | WS_VISIBLE, x, y + s * 4, w, hB, hTabPage3, (HMENU)IDM_BUILD_UNITY, h, nullptr);
    hButtonClearUnity = CreateWindowW(L"BUTTON", L"Clear", WS_CHILD | WS_VISIBLE, x, y + s * 5, w, hB, hTabPage3, (HMENU)IDM_CLEAR_UNITY, h, nullptr);
    hButtonDownloadUnity = CreateWindowW(L"BUTTON", L"Download", WS_CHILD | WS_VISIBLE, x, y + s * 6, w, hB, hTabPage3, (HMENU)IDM_DOWNLOAD_UNITY, h, nullptr);
    hButtonInvertSmoothness = CreateWindowW(L"BUTTON", L"Invert Smoothness", WS_CHILD | WS_VISIBLE, x, y + s * 7, w, hB, hTabPage3, (HMENU)IDM_INVERT_SMOOTHNESS, h, nullptr);
    hLabelInvertSmoothness = CreateWindowW( L"STATIC", L"Default", WS_CHILD | WS_VISIBLE | SS_CENTER, x, y + s * 7 + hB + 5, w, 20, hTabPage3, nullptr, h, nullptr);

    //tab 4 help text
    HWND hHelpText = CreateWindowW(
        L"STATIC",
        L"Universal Image Packer (UIP)\n\n"
        L"Tab 1: Extract Channels\n"
        L"- Open an image\n"
        L"- Extract individual RGBA channels\n\n"

        L"Tab 2: Pack AORM\n"
        L"- Load AO, Roughness, Metalness\n"
        L"- Output packs to:\n"
        L"  Red = AO\n"
        L"  Green = Roughness\n"
        L"  Blue = Metalness\n\n"

        L"Tab 3: Pack Unity Map\n"
        L"- Metalness AO, Detail, Metalness\n"
        L"- Output packs to:\n"
        L"  Red = Metalness\n"
        L"  Green = AO\n"
        L"  Blue = Detail\n"
        L"  Alpha = Smoothness\n\n"

        L"- 'Invert Smoothness' inverts the black and white values\n"
        L"  for the roughness map, this is used when the map was not\n"
        L"  specifically designed for use in unity.\n\n"


        L"General:\n"
        L" - It is not required to upload all possible image inputs\n"
        L"  to build a map, but they all must be the same resolution.\n\n"

        L"  Created by rarerex\n"
        L"  https://github.com/rarerex/Universal-Image-Packer\n\n",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_EDITCONTROL,
        20,
        20,
        rc.right - rc.left - 40,
        rc.bottom - rc.top - 40,
        hTabPage4,
        nullptr,
        h,
        nullptr
    );

    //start
    ShowWindow(wnd, n);
    UpdateWindow(wnd);

    //exit
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    delete gImage;
    delete gChannelImage;
    delete gAO;
    delete gRough;
    delete gMetal;
    delete gAORM;
    delete gUnityR;
    delete gUnityG;
    delete gUnityB;
    delete gUnityA;
    delete gUnityMask;

    GdiplusShutdown(gdiplusToken);
    return 0;
}
