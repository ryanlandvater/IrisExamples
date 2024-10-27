# Iris Windows Example

The included file demonstrates how to extend the main.cpp file of a Win32 App, created by Visual Studio, to integrate Iris. [Microsoft provides guides to create an initial Win32 application in C++](https://learn.microsoft.com/en-us/windows/win32/desktop-programming). The Win32 API (also called the Windows API) is the original platform for native C/C++ Windows applications that require direct access to Windows and hardware. This guide is written in a similar manner to the [Introduction to Programmin in Windows Guides](https://learn.microsoft.com/en-us/windows/win32/learnwin32/your-first-windows-program) and can provide a basic framework to the developemnt of an such an application written in C++ and calling the Iris API.

## Create an Iris Instance
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
## Bind a Window to Initialize Iris
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
## Set up Application Event Loop and Event Callbacks
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

We should create a mechanism for tracking the movement of user's inputs. To track dragging movements we will need the relative location of the cursor or touch event within the window $[0.0,1.0]$ in the x and y-dimensions. Based upon Windows conventions the top right corner is 1.0 while the bottom left is 0.0 in window space. The Iris engine allows the inclusion on velocity as a parameter too, so we will need a way to measure time as well to calculate movement velocity. This is all optional, but I will include an example of how to do this below.
```C++
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
```
We can then implement the drag-to-translate the scope view in the standard user interface `WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)` method. When the mouse is originally clicked, or a touch lands down on a touch-surface, record it, see `case WM_LBUTTONDOWN:`. This involves getting the window dimensions and the pixel location of the pointer to calculate the normalized / relative location of the pointer in the window (*Iris uses normalized float locations rather than raw pixel locations*). We will also save when this happens for calculating velocity.

Next move to `case WM_LBUTTONDOWN:` where we implement the actual translation. Perform this action on mouse move only if the mouse is being held down (`(GetKeyState(VK_LBUTTON) & 0x8000) != 0`). We will again find the relative x and y-locations of the pointer but this time we will also calculate velocity of movement (previously it was zero as there was no prior movement):
```math
\displaylines{
v_x = \frac{x_{prev}-x}{\Delta t}\newline
v_y = \frac{y_{prev}-y}{\Delta t}
}
```
We can then calculate the drag distance in each dimension and submit it to Iris for view translation ($\Delta x$ and $\Delta y$):
```math
\displaylines{
\Delta x = x_{prev}-x\newline
\Delta y = y_{prev}-y
}
```
I prefer to add in sensitivity to movement velocity and to translate the view with greater magnitude when the user is moving the mouse quickly. This can be linearly or exponentially scaled and the below example is simply how I choose to implement this. All constants are empiric and you can certainly play around with these equations to get the responsiveness that matches how you would like your implementation to work. You will notice in the below implementation tracks the cursor movement 1:1 at low velocity.
```math
\displaylines{
\Delta x = (x_{prev}-x) * \left(1+\frac{|v_x|^{4}}{10}\right)\newline
\Delta y = (y_{prev}-y) * \left(1+\frac{|v_y|^{4}}{10}\right)\newline
\lim_{v_x\to0}\Delta x = (x_{prev}-x)\newline
\lim_{v_y\to0}\Delta y = (y_{prev}-y)
}
```
Finally, the tracker is set to the update to perpetuate the movements. We should not compare next movements to the original; rather they should always be compared to the previous location.
```C++
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Iris::Viewer* v_ptr  = static_cast<Iris::Viewer*>(GetProp(hWnd, _T("VIEWER")));
    if (!v_ptr) return DefWindowProc(hWnd, message, wParam, lParam);
    Iris::Viewer& viewer = *v_ptr;
    
    switch (message)
    {
        // Handle other application messages from other user inputs
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
                .x_vel = (tracker.x - x) / static_cast<float>(dt / 1E7),
                .y_vel = (tracker.y - y) / static_cast<float>(dt / 1E7),
                .timestamp = get_millisec_timestamp(),
            };
            // Calculate the distance dragged during this event duration
            Iris::viewer_engine_translate(viewer, Iris::ViewerTranslateScope{
                // This x-translation calculation may seem confusing. An easy alternative is 
                // .x_translate = (update.x - tracker.x), which will track cursor movement 1:1
                // What is here is a personal preference that moves above 1:1 scaled with velocity
                //                  1:1 normal          Times 1 + velocity^1.1 then scaled back by 5 to keep the effect under control
                .x_translate = (update.x - tracker.x) * (std::pow(std::abs(update.x_vel), 4.0f) / 10.f + 1.f),
                .y_translate = (update.y - tracker.y) * (std::pow(std::abs(update.y_vel), 4.0f) / 10.f + 1.f),
                .x_velocity = update.x_vel,
                .y_velocity = update.y_vel
                });
            // Set the new tracker to the update
            tracker = update;
        }
        } break;
    }
}
```

## Create an Open File Dialog to open Slide Files
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
