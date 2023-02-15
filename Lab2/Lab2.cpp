// Lab1.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Lab2.h"
#include "Renderer.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, Renderer*);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

struct MyWindowCreateParams
{
    Renderer* pRenderer;
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB2, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    Renderer* pRenderer = new Renderer();

    HWND hWnd = 0;

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow, pRenderer))
    {
        delete pRenderer;
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB2));

    MSG msg;

    // Main message loop:
    bool exit = false;
    while (!exit)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (msg.message == WM_QUIT)
                exit = true;
        }
        //OutputDebugString(_T("Render\n"));
        if (pRenderer->IsRunning())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(16667));
            if (!pRenderer->Render())
            {
                pRenderer->Term();
                PostQuitMessage(0);
            }
        }
    }
    pRenderer->Term();
    delete pRenderer;
    pRenderer = nullptr;
    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB2));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, Renderer* pRenderer)
{
    hInst = hInstance; // Store instance handle in our global variable

    struct MyWindowCreateParams* pWCreateParams = new struct MyWindowCreateParams;
    pWCreateParams->pRenderer = pRenderer;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, pWCreateParams);

    if (!hWnd)
    {
        delete pWCreateParams;
        return FALSE;
    }
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    LONG WindowWidth = 1280;
    LONG WindowHeight = 720;

    // Adjust window size
    {
        RECT rc;
        rc.left = 0;
        rc.right = WindowWidth; // 1280
        rc.top = 0;
        rc.bottom = WindowHeight; // 720

        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        MoveWindow(hWnd, 100, 100, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }


    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CREATE)
    {
        struct MyWindowCreateParams* pWCreateParams = (struct MyWindowCreateParams*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (ULONG_PTR)pWCreateParams->pRenderer);
        delete pWCreateParams;
    }
    switch (message)
    {
    case WM_CREATE:
        {
            Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (!pRenderer->Init(hWnd))
            {
                pRenderer->Term();
                PostQuitMessage(0);
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            Renderer* pRenderer = (Renderer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            if (!pRenderer->Resize(width, height))
            {
                pRenderer->Term();
                PostQuitMessage(0);
                break;
            }
        }   
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
