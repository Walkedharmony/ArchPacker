#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "arch_packer.h"

#pragma comment(lib, "comctl32.lib")

// Global variables
HWND g_hListView = NULL;
ArchPacker g_packer;
std::vector<FileEntry> g_entries;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void InitListView(HWND hWnd);
void PopulateListView();
void OpenArchive(HWND hWnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"ArchiveViewerClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    // Create window
    HWND hWnd = CreateWindowEx(
        0,
        L"ArchiveViewerClass",
        L"Archive Viewer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Create menu
        HMENU hMenu = CreateMenu();
        HMENU hFileMenu = CreatePopupMenu();
        AppendMenu(hFileMenu, MF_STRING, 1, L"&Open Archive");
        AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hFileMenu, MF_STRING, 2, L"E&xit");
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
        SetMenu(hWnd, hMenu);

        // Create status bar
        CreateWindowEx(
            0, STATUSCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hWnd, (HMENU)1, GetModuleHandle(NULL), NULL
        );

        // Create list view
        InitListView(hWnd);
        break;
    }
    case WM_SIZE: {
        // Resize controls
        HWND hStatus = GetDlgItem(hWnd, 1);
        SendMessage(hStatus, WM_SIZE, 0, 0);

        RECT rc;
        GetClientRect(hWnd, &rc);
        if (hStatus) {
            RECT rcStatus;
            GetWindowRect(hStatus, &rcStatus);
            rc.bottom -= rcStatus.bottom - rcStatus.top;
        }

        SetWindowPos(g_hListView, NULL,
            rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOZORDER);
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1: // Open Archive
            OpenArchive(hWnd);
            break;
        case 2: // Exit
            PostQuitMessage(0);
            break;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void InitListView(HWND hWnd) {
    // Create list view control
    g_hListView = CreateWindowEx(
        0,
        WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
        LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
        0, 0, 0, 0,
        hWnd,
        (HMENU)2,
        GetModuleHandle(NULL),
        NULL
    );

    // Set extended styles
    ListView_SetExtendedListViewStyle(
        g_hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    // Add columns
    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    struct {
        const WCHAR* text;
        int width;
    } columns[] = {
        { L"Filename", 300 },
        { L"Size (bytes)", 100 },
        { L"Compressed", 100 },
        { L"Timestamp", 150 }
    };

    for (int i = 0; i < 4; i++) {
        lvc.iSubItem = i;
        WCHAR buffer[32];
        wcscpy_s(buffer, columns[i].text);
        lvc.pszText = buffer;
        lvc.cx = columns[i].width;
        ListView_InsertColumn(g_hListView, i, &lvc);
    }
}

void PopulateListView() {
    // Clear existing items
    ListView_DeleteAllItems(g_hListView);

    // Add items
    for (size_t i = 0; i < g_entries.size(); i++) {
        const auto& entry = g_entries[i];

        // Convert timestamp to string
        SYSTEMTIME st;
        FileTimeToSystemTime((FILETIME*)&entry.timestamp, &st);
        WCHAR timeStr[64];
        GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, timeStr, 64);
        wcscat_s(timeStr, L" ");
        WCHAR temp[64];
        GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, temp, 64);
        wcscat_s(timeStr, temp);

        // Add item
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;

        // Convert filename from char to wchar_t
        WCHAR wfilename[256];
        MultiByteToWideChar(CP_ACP, 0, entry.filename, -1, wfilename, 256);

        lvi.pszText = wfilename;
        ListView_InsertItem(g_hListView, &lvi);

        // Set subitems
        WCHAR sizeStr[32];
        swprintf_s(sizeStr, L"%u", entry.size);
        ListView_SetItemText(g_hListView, (int)i, 1, sizeStr);

        // Perbaikan untuk kolom compressed
        WCHAR compressedText[16];
        wcscpy_s(compressedText, entry.compressionType ? L"Yes" : L"No");
        ListView_SetItemText(g_hListView, (int)i, 2, compressedText);

        ListView_SetItemText(g_hListView, (int)i, 3, timeStr);
    }
}

void OpenArchive(HWND hWnd) {
    // Create file open dialog
    OPENFILENAME ofn = { 0 };
    WCHAR szFile[260] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"Archive Files (*.arch)\0*.arch\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        // Read archive
        std::ifstream in(szFile, std::ios::binary);
        if (!in) {
            MessageBox(hWnd, L"Failed to open archive file", L"Error", MB_ICONERROR);
            return;
        }

        ArchHeader header;
        if (!g_packer.ReadHeader(in, header)) {
            MessageBox(hWnd, L"Invalid archive format", L"Error", MB_ICONERROR);
            return;
        }

        g_entries.clear();
        if (g_packer.ReadFileEntries(in, g_entries, header.indexOffset)) {
            // Update window title
            std::wstring title = L"Archive Viewer - ";
            title += szFile;
            SetWindowText(hWnd, title.c_str());

            // Update status bar
            HWND hStatus = GetDlgItem(hWnd, 1);
            WCHAR statusText[256];
            swprintf_s(statusText, L"%d files loaded", (int)g_entries.size());
            SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)statusText);

            // Populate list view
            PopulateListView();
        }
        else {
            MessageBox(hWnd, L"Failed to read archive contents", L"Error", MB_ICONERROR);
        }
    }
}