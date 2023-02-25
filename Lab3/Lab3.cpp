// Lab1.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Lab3.h"
#include "Renderer.h"

#define MAX_LOADSTRING 100

struct MyWindowData
{
    std::unique_ptr<Renderer> pRenderer;
    std::unique_ptr<Scene> pScene;

    MyWindowData() : pRenderer(std::make_unique<Renderer>()), pScene(std::make_unique<Scene>()) { };
};

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, MyWindowData*);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB3, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    MyWindowData* pMyWindowData = new struct MyWindowData();

    HWND hWnd = 0;

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow, pMyWindowData))
    {
        delete pMyWindowData;
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB3));

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
        if (pMyWindowData->pRenderer->IsRunning())
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds(unsigned long((1./60)*1e9)));
            pMyWindowData->pScene->Update(1. / 60);
            if (!pMyWindowData->pRenderer->Render(pMyWindowData->pScene.get()))
            {
                pMyWindowData->pRenderer->Term();
                PostQuitMessage(0);
            }
        }
    }
    pMyWindowData->pRenderer->Term();
    delete pMyWindowData;
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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB3));
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, MyWindowData* pMyWindowData)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, pMyWindowData);

    if (!hWnd)
    {
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
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (ULONG_PTR)pMyWindowData);
    }
    switch (message)
    {
    case WM_CREATE:
    {
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (!pMyWindowData->pRenderer->Init(hWnd))
        {
            pMyWindowData->pRenderer->Term();
            PostQuitMessage(0);
            break;
        }
        break;
    }
    case WM_SIZE:
    {
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        if (!pMyWindowData->pRenderer->Resize(width, height))
        {
            pMyWindowData->pRenderer->Term();
            PostQuitMessage(0);
            break;
        }
        break;
    }
    case WM_KEYDOWN:
    {
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        pMyWindowData->pScene->OnKeyDown(wParam, lParam);
        break;
    }
    case WM_KEYUP:
    {
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        pMyWindowData->pScene->OnKeyUp(wParam, lParam);
        break;
    }
    case WM_MOUSEMOVE:
    {
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        pMyWindowData->pScene->OnMouseMove(wParam, lParam);
        break;
    }
    case WM_LBUTTONDOWN:
    {
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        pMyWindowData->pScene->OnLMouseDown(wParam, lParam);
        break;
    }
    case WM_LBUTTONUP:
    {
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        pMyWindowData->pScene->OnLMouseUp(wParam, lParam);
        break;
    }
    case WM_MOUSEWHEEL:
    {
        struct MyWindowData* pMyWindowData = (struct MyWindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        pMyWindowData->pScene->OnMouseWheel(wParam, lParam);
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
