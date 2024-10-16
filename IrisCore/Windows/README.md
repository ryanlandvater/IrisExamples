# Iris Windows Example

The included file demonstrates how to extend the main.cpp file of a Win32 App, created by Visual Studio, to integrate Iris. [Microsoft provides guides to create an initial Win32 application in C++](https://learn.microsoft.com/en-us/windows/win32/desktop-programming). The Win32 API (also called the Windows API) is the original platform for native C/C++ Windows applications that require direct access to Windows and hardware. This guide is written in a similar manner to the [Introduction to Programmin in Windows Guides](https://learn.microsoft.com/en-us/windows/win32/learnwin32/your-first-windows-program) and can provide a basic framework to the developemnt of an such an application written in C++ and calling the Iris API.

Due to the high-quality graphics that Iris generates, proper scaling for high-DPI monitors (which we highly recommend using for digital slides) must be explicitly implemented. The **Iris::Viewer** can be initialized by creating the viewer instance. We provide information about the application we are writing to the underlying engine. The application bundle path is particularly important to access runtime resources.
```C++
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
#include "IrisUI.hpp"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    //...Other setup code
    SetProcessDpiAwareness (PROCESS_PER_MONITOR_DPI_AWARE);
    
    // Create the Iris Viewer Instance
     GetModuleFileNameW       (NULL, szBundlePath, MAX_LOADSTRING);
    PathRemoveFileSpecW      (szBundlePath);
    std::wstring wide_path   (szBundlePath);
    std::string  ASCI_path   (wide_path.begin(), wide_path.end());
    Iris::ViewerCreateInfo viewer_info {
        .ApplicationName         = IDS_APP_TITLE,
        .ApplicationVersion      = 1,
        .ApplicationBundlePath   = ASCI_path.c_str(),
    };  
    Iris::Viewer viewer = Iris::create_viewer(viewer_info);
}
```

A [Window](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window?view=windows-app-sdk-1.6) object will be need to be constructed using the [CreateWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindoww) function in the WinAPI (`#include <windows.h>`) to house the Viewer and provide a surface upon which the Viewer object can draw scope view (and UI elements). This will allow the Iris Viewer to initialize the underlying rendering engine. Once bound, the system will initialize and begin the rendering process
```C++
//...Prior setup code
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

Iris::ViewerBindExternalSurfaceInfo bind_info {
    .viewer      = viewer,
    .instance    = hInstance,
    .window      = hWnd
    };
Iris::viewer_bind_external_surface(bind_info);
ShowWindow  (hWnd, nCmdShow);
UpdateWindow(hWnd);
```

Iris will be initialized at this point. Because of how Windows performs callbacks as a part of the application loop, it is a good idea to extend the window class with a reference to the window viewer (essentially make it a property of the window that it is rendering into) using the [SetProp](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setpropa) function.
```C++
// Establish the Window Property during setup
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // ...Prior setup code
    BOOL result = SetProp (hWnd, _T("VIEWER"), &viewer);
    if (result == false)  return FALSE;
}

// Now reference it in the message callback function
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Iris::Viewer* v_ptr  = static_cast<Iris::Viewer*>(GetProp(hWnd, _T("VIEWER")));
    if (!v_ptr) return DefWindowProc(hWnd, message, wParam, lParam);
    Iris::Viewer& viewer = *v_ptr;
    
    switch (message)
    {
        // DO SOMETHING HERE...
    }
}
```

It would also be wise to set up the OpenFile dialog to allow users to select a slide file for rendering. This is performed using the IFileOpenDialog class and based upon the [open-dialog-box-sample](https://learn.microsoft.com/en-us/windows/win32/learnwin32/open-dialog-box-sample) by Microsoft ([Github source code](https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/Win7Samples/begin/LearnWin32/OpenDialogBox)).

```C++
// Define the Filter
const COMDLG_FILTERSPEC slide_filter[]{
        {L"Iris Slide File", L"*.iris"},
        {L"Aperio Slide File", L"*.svs"}
};
// Define the open_slide_file funtion
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
```