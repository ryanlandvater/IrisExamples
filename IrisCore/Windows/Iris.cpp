/**
 * @file Iris.cpp
 * @author Ryan Landvater
 * @brief Entry Point for Iris Windows Example Implementation
 * @version 2024.0.1
 * @date 2023-08-29
 * 
 * @copyright Copyright (c) 2023-24
 * 
 * This is an example implementation of an Iris Viewer for the Windows
 * OS Platform. 
 *
 */

// Include Win32 App additional headers
// These will be automatically created by Visual Studio
#include "framework.h"
#include "Resource.h"

// Include standard headers
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

// Include standard Windows headers
// including shell scaling for high-DPI displays
// and shoobjidl_core for windows open file dialog
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <ShellScalingApi.h>
#include <shobjidl_core.h>

// Include Iris Core header
#include "IrisCore.hpp"

// Global Variables:
#define             MAX_LOADSTRING 100          // General max size of strings
HINSTANCE hInst;                                // current instance
WCHAR szTitle       [MAX_LOADSTRING];           // The title bar text
WCHAR szWindowClass [MAX_LOADSTRING];           // the main window class name
WCHAR szBundlePath  [MAX_LOADSTRING];           // Executable location

// Forward declarations of functions included in this code module:
ATOM                RegisterWin32Window(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, Iris::Viewer&);
HRESULT             open_slide_file(HWND, Iris::Viewer&)
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// WinMain Application Entrypoint
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);  // Not used
    UNREFERENCED_PARAMETER(lpCmdLine);      // Not used

    // Initialize global strings
    // This pulls from Win32 App Structure by Visual Studio
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_IRIS, szWindowClass, MAX_LOADSTRING);
    SetProcessDpiAwareness (PROCESS_PER_MONITOR_DPI_AWARE);
    RegisterWin32Window(hInstance);
    hInst = hInstance; // Store instance handle in our global variable

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //         Create the Iris::Viewer          //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    GetModuleFileNameW       (NULL, szBundlePath, MAX_LOADSTRING);
    PathRemoveFileSpecW      (szBundlePath);
    std::wstring wide_path   (szBundlePath);
    std::string  ASCI_path   (wide_path.begin(), wide_path.end());
    Iris::ViewerCreateInfo viewer_info {
        .ApplicationName         = IDS_APP_TITLE,
        .ApplicationVersion      = 20240101,
        .ApplicationBundlePath   = ASCI_path.c_str(),
    };  
    Iris::Viewer viewer = Iris::create_viewer(viewer_info);

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // Create the Window to house the Viewer    //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        // Iris recommended window characteristics for a viewer instance
        WS_DLGFRAME | WS_SYSMENU | WS_EX_WINDOWEDGE | WS_MAXIMIZEBOX | WS_SIZEBOX,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hWnd) return FALSE;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //    Bind the Iris::Viewer to the hWnd     //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    Iris::ViewerBindExternalSurfaceInfo bind_info {
    .viewer      = viewer,
    .instance    = hInstance,
    .window      = hWnd
    };
    Iris::viewer_bind_external_surface(bind_info);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);


    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //    Register the Viewer with the Window   //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // This is to ensure callbacks can access the viewer
    // object to invoke state changes in the viewer
    BOOL result = SetProp (hWnd, _T("VIEWER"), &viewer);
    if (result == false)  return FALSE;
    
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //          Choose a file to view           //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    open_slide_file(hWnd, viewer);


    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //          MAIN APPLICATION LOOP           // 
    // Begin the Application Main Message Loop  //
    // See WndProc(HWND, UINT, WPARAM, LPARAM)  //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // Set the Accelerator table from the Win32 App
    // made by Visual Studio and dispatch the loop
    HACCEL hAccelTable = 
    LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IRIS));
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM RegisterWin32Window(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex {
        .cbSize         = sizeof(WNDCLASSEXW),
        .style          = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc    = WndProc,
        .cbClsExtra     = 0,
        .cbWndExtra     = 0,
        .hInstance      = hInstance,
        .hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IRIS)),
        .hCursor        = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1),
        .lpszMenuName   = MAKEINTRESOURCEW(IDC_IRIS),
        .lpszClassName  = szWindowClass,
        .hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL)),
    };

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: open_slide_file (HWND  Iris::Viewer&)
//
//   PURPOSE: Create a open file dialog for use in opening slide files
//
//
const COMDLG_FILTERSPEC slide_filter[]{
        {L"Iris Slide File", L"*.iris"},
        {L"Aperio Slide File", L"*.svs"}
};
HRESULT open_slide_file(HWND hWnd, Iris::Viewer& viewer)
{
    // Call CoInitializeEx to initialize the COM library.
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        // Create the FileOpenDialog object.
        IFileOpenDialog* pFileOpen;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        if (SUCCEEDED(hr))
        {
            // Filter the results to only known file types
            hr = pFileOpen->SetFileTypes(2, slide_filter);

            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR wstr_path;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &wstr_path);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        // We need to convert the FileSysPath from a
                        // wide-string (windows) to normal UTF8 format.

                        // Get the size of the resultant standard string
                        int path_size =
                        WideCharToMultiByte(CP_UTF8, 0, &wstr_path[0], 
                        wcslen(wstr_path), NULL, 0, NULL, NULL);

                        // Create the string and copy out the literal path 
                        std::string file_path(path_size,' ');
                        WideCharToMultiByte(CP_UTF8, 0, &wstr_path[0], 
                        wcslen(wstr_path), file_path.data(), path_size, NULL, NULL);

                        // Free the wide string file path object after copy
                        CoTaskMemFree(wstr_path);

                        // Ask the viewer to open the slide
                        Iris::SlideOpenInfo open_info {
                            .type = Iris::SlideOpenInfo::SLIDE_OPEN_LOCAL,
                            .local = Iris::LocalSlideOpenInfo {
                                .filePath = file_path.c_str()
                            }
                        };
                        Iris::viewer_open_slide(viewer, open_info);
                        
                    }
                    // Release the underlying file object
                    pItem->Release();
                }
            }
            // Release the file dialog window
            pFileOpen->Release();
        }
        // Call CoUninitialize to uninitialize the COM library.
        CoUninitialize();
    }
    // Return the result flag
    return hr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//              USER INTERACTION CALLBACKS              //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// Build required methods and structures to track user interactions
// 
// Get the current time in milliseconds since the epoch
long get_millisec_timestamp()
{
    auto ms_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>
        (std::chrono::system_clock::now().time_since_epoch());
    return ms_since_epoch.count();
}
// Create an input tracker to track the location of the cursor over time
// We will keep it in global scope for now but this could be added to the hWnd
// as a property if desired (similarly to the way the viewer is stored).
struct InputTracker {
    float   x;
    float   y;
    float   x_vel;
    float   y_vel;
    long    timestamp;
} tracker;

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //    Retrieve the Viewer from the Window   //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // Get the viewer pointer, if it exists. If it does not, skip.
    Iris::Viewer* v_ptr  = static_cast<Iris::Viewer*>(GetProp(hWnd, _T("VIEWER")));
    if (!v_ptr) return DefWindowProc(hWnd, message, wParam, lParam);
    Iris::Viewer& viewer = *v_ptr;
    
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    // Destroy window message was sent.
    case WM_DESTROY: {
        PostQuitMessage(0);
    } break;
    // A change has occured affecting the window size
    case WM_SIZE: {
        // Inform the viewer that the window has resized
        Iris::viewer_window_resized(viewer);
    } break;
    case WM_KEYDOWN: {
        // Do something when a key in the keyboard is pressed...
        // That goes here.
    } break;
    case WM_KEYUP: {
        // In this example, we will have the key arrows move the screen the entire
        // view in a direction when pressed.

        switch (wParam) {
        // Assign some key for opening a new slide. Any key can work; 
        //'c' is used in this example; function keys may work well too.
        case 0x43 /*this is the 'C' key for 'close'*/:
            open_slide_file (hWnd, viewer);
            break;
        // If UP arrow key pressed, move the scope view an entire screen height up.
        case VK_UP:
            Iris::viewer_engine_translate(viewer, Iris::ViewerTranslateScope{
                .x_translate = 0.f,
                .y_translate = 1.f,
                });
            break;
        // If DOWN arrow key pressed, move the scope view an entire screen height down.
        case VK_DOWN:
            Iris::viewer_engine_translate(viewer, Iris::ViewerTranslateScope{
                .x_translate = 0.f,
                .y_translate = -1.f,
                });
            break;
        // If LEFT arrow key pressed, move the scope view an entire screen width to the left.
        case VK_LEFT:
            Iris::viewer_engine_translate(viewer, Iris::ViewerTranslateScope{
                .x_translate = 1.f,
                .y_translate = 0.f,
                });
            break;
        // If RIGHT arrow key pressed, move the scope view an entire screen width to the right.
        case VK_RIGHT:
        Iris::viewer_engine_translate(viewer, Iris::ViewerTranslateScope{
            .x_translate = -1.f,
            .y_translate = 0.f,
            });
        break;
        }
    } break;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //     Scope View Zooming       //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //  
    case WM_POINTERWHEEL:
    case WM_POINTERHWHEEL:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
        // If the scroll wheel is rotated, get the amount via GET_WHEEL_DELTA_WPARAM 
        // (always a uint16_t and a factor of 120; interpret it by factors of 0.1f)
        // Pass this to the engine to zoom. We will expand on this in the UI module.
        Iris::viewer_engine_zoom(viewer, Iris::ViewerZoomScope{
            .increment = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / 1200.f
        });
    } break;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //      DRAG SCOPE VIEW         //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //  
    case WM_LBUTTONDOWN: {
        // Get the window dimension to normalize the tracker location
        RECT __r;
        GetWindowRect(hWnd, &__r);
        float width = static_cast<float> (__r.right - __r.left);
        float height = static_cast<float> (__r.bottom - __r.top);
        // Get the current cursor location
        POINTS pts = MAKEPOINTS(lParam);
        // And set the tracker to this location
        tracker.x = static_cast<float>(pts.x) / width;
        tracker.y = static_cast<float>(pts.y) / height;
        tracker.x_vel = 0;
        tracker.y_vel = 0;
        // Save the time this happened; it is used for calculating velocity
        tracker.timestamp = get_millisec_timestamp();
    } break;
    case WM_MOUSEMOVE: {
        // If the left mouse button is being held down, the view is being dragged.
        if ((GetKeyState(VK_LBUTTON) & 0x8000) != 0) {
            // Get the window dimension to normalize the tracker location
            RECT __r;
            GetWindowRect(hWnd, &__r);
            float width = static_cast<float> (__r.right - __r.left);
            float height = static_cast<float> (__r.bottom - __r.top);
            // Get the current cursor location
            POINTS pts = MAKEPOINTS(lParam);
            // And set the tracker to this location
            float x = static_cast<float>(pts.x) / width;
            float y = static_cast<float>(pts.y) / height;
            long dt = get_millisec_timestamp() - tracker.timestamp;
            // Calculate the update. It gets the current locations, the current time, and velocities
            InputTracker update{
                .x = x,
                .y = y,
                .x_vel = (x - tracker.x) / static_cast<float>(dt / 1E7) / 2.f,
                .y_vel = (y - tracker.y) / static_cast<float>(dt / 1E7) / 2.f,
                .timestamp = get_millisec_timestamp(),
            };
            // Calculate the distance dragged during this event duration
            Iris::viewer_engine_translate(viewer, Iris::ViewerTranslateScope{
                // This x-translation calculation may seem confusing. An easy alternative is 
                // .x_translate = (update.x - tracker.x), which will track cursor movement 1:1
                // What is here is a personal preference that moves above 1:1 scaled with velocity
                //                  1:1 normal          Times 1 + velocity^1.1 then scaled back by 5 to keep the effect under control
                .x_translate = (update.x - tracker.x) * (std::pow(std::abs(update.x_vel), 4f) / 10.f + 1.f),
                .y_translate = (update.y - tracker.y) * (std::pow(std::abs(update.y_vel), 4f) / 10.f + 1.f),
                .x_velocity = update.x_vel,
                .y_velocity = update.y_vel
                });
            // Set the new tracker to the update
            tracker = update;
        }
    } break;
    case WM_POINTERDOWN: {
        return DefWindowProc(hWnd, message, wParam, lParam);
    } break;
    case WM_NCHITTEST: {
        return DefWindowProc(hWnd, message, wParam, lParam);
    } break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
