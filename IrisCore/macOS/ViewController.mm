//
//  ViewController.mm
//  macOS implementation
//
//  Created by Ryan Landvater on 8/26/23.
//  Copyright Ryan Landvater 2023-2024
//  All rights reserved
//



// EXAMPLE IMPLEMENTATION OF IRIS CORE
// The following Code describes how to properly implement
// the Iris WSI program
// Start by importing the classic Apple required headers
// Metal Kit is needed to interface Vulkan and Metal
#import <MetalKit/MetalKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

// Extend the View Controller
@interface ViewController : NSViewController
- (void) destroyIrisView;
@end

// IMPORT Iris and some basic C++ headers
// used for user input tracking (chrono for time)
// and debugging output (iostream)
#import "IrisCore.hpp"
#import <chrono>
#import <iostream>

// NOTE: iOS has more advanced UI capabilities with the 
// touch screen interface (UI Gesture recognizers)
// In macOS the NS Gesture recognizers are not as helpful
// so we will do the math ourselves in these example implementations.
// You may use NS Gesture Recognizers or UI gesture recognizers if that
// is easier and makes more sense to you.
// Get the current time so we can track time between user interaction frames.
long get_current_microsecond_timestamp () {
    auto ms_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>
                            (std::chrono::system_clock::now().time_since_epoch());
    return ms_since_epoch.count();
}
// Create a touch tracker to track the location of fingers on a laptop trackpad
struct TouchTracker {
    float x;
    float y;
    long  timestamp;
    float x_vel;
    float y_vel;
};

// ~~~~~~ IrisView ObjectiveC++ API WRAPPER ~~~~~~~~
// Iris View -> Per the Iris API, the Iris Viewer (Iris::Viewer) class
// is the entry class and interface class for the Iris API.
// We recommend wrapping it in a Metal View (MTKView) for a single concise
// ObjectiveC++ class (as the MTKView is needed for Iris Configuration).
// The only property needed is the Iris::Viewer handle.
@interface IrisView : MTKView <UIDocumentPickerDelegate, NSWindowDelegate>
@property (unsafe_unretained, nonatomic) Iris::Viewer handle;
// There are only 3 functionalities this wrapper should handle:
- (void) bindSurface;           // Bind a surface
- (void) destroyViewer;         // Destroy viewer (cleanup)
- (void) openLoadSlideDialog    // Create an open file dialog

@end


@implementation ViewController
- (void) viewWillAppear {
    [super viewWillAppear];

    NSWindow* __window = self.view.window;
    [__window setTitle:@"Iris Slide Viewer"];
    [__window setTitlebarAppearsTransparent:YES];
    [__window setAcceptsMouseMovedEvents:YES];
    [__window setStyleMask:(__window.styleMask|NSWindowStyleMaskFullSizeContentView)];
    [__window setFrame:[[NSScreen mainScreen] visibleFrame] display:YES];
    
    IrisView*   __view  = [[IrisView alloc] initWithFrame:self.view.frame];
    // NOTE: NSGestureRecognizer objects do not receive trackpad touches!!
    [self setView:__view];
    
    [__view bindSurface];
    [__view generateUserInterface];
    
    
    
    [__window makeFirstResponder:__view];
    [__window setDelegate:__view];
    [__view openLoadSlideDialog];
}
- (void) destroyIrisView
{
    if ([self.view isKindOfClass:[IrisView class]])
        [(IrisView*)self.view destroyViewer];
}
- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];
    // Update the view, if already loaded.
}
@end



@implementation IrisView
// Initialize the Iris Viewer when creating the IrisView Objective C++ class.
// This will NOT generate the Iris Render Engine yet; A window must be bound before the engine
// is initialized. This is because the window provides the draw surface parameters needed for configuration.
- (instancetype) initWithFrame:(NSRect)frameRect
{
    // NOTE** If you wish, you could not override initWithFrame and
    // fold all initialization routines into the bindSurface method.
    // My preference simply is to break them up into separate calls...

    // Provide the application name for which Iris Core Render Engine will provide scope rendering
    // Provide the Application Version
    // Provide the Application bundle path (the local path) for loading runtime files (REQUIRED).
    Iris::ViewerCreateInfo ViewerCreateInfo {
        .ApplicationName        = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"] UTF8String],
        .ApplicationVersion     = static_cast<uint32_t>([[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"] integerValue]),
        .ApplicationBundlePath  = [[[NSBundle mainBundle] resourcePath] UTF8String],
    };
    
    // Call the Iris API to Create the viewer.
    // This will NOT generate the render engine yet; just prepare the system.
    self = [super initWithFrame:frameRect];
    [self setHandle: Iris::create_viewer(ViewerCreateInfo)];
    [self setAllowedTouchTypes:(NSTouchTypeMaskDirect|NSTouchTypeMaskIndirect)];
    
    return self;
}
// Bind the viewer surface. This initializes the render engine initialization.
- (void) bindSurface
{
    // Bind the Metal Surface to the Iris Viewer instance.
    // This bridges the MTKView's underlying CaMetalLayer
    // With Vulkan to create a VKSurface which Iris Draws on.
    // Iris::ViewerBindExternalSurfaceInfo changes based upon
    // the underlying platform. For Apple, CaMetalLayer is used.
    Iris::ViewerBindExternalSurfaceInfo bind_info {
        .viewer = self.handle,
        .layer  = (__bridge const void*) self.layer,
    };
    
    // Call the Iris API to bind the surface and
    // inplement Iris' plug-and-play initiazation
    // routines to initialize the full Render Engine system.
    Iris::viewer_bind_external_surface (bind_info);
}
// Destroy the underlying viewer and close the Iris' API.
// This is required on Apple systems due to the Objecive C ARC system.
// Iris uses reference counted pointers but the ARC system
// and this internal ref. count system interfere with one another.
// Destroy viewer must be called during the shutdown of the program.
- (void) destroyViewer
{
    if (self.handle != nullptr)
        [self setHandle: nil];
}
// If the window size changes, inform Iris that the window size
// changed and that it must reformat the rendering pipelines to
// accomodate the change in draw surface size.
- (void) windowDidResize:(NSNotification *)notification
{
    Iris::viewer_window_resized(self.handle);
}
// An example implementation of an open-file dialog to open slide files
- (void) openLoadSlideDialog
{
    NSWindow*    __window   = self.window;
    NSOpenPanel* open_panel = [NSOpenPanel openPanel];
    [open_panel setAllowsMultipleSelection:         NO];
    [open_panel setCanChooseDirectories:            NO];
    [open_panel setReleasedWhenClosed:              YES];
    [open_panel setCanResolveUbiquitousConflicts:   NO];
    [open_panel setAccessoryViewDisclosed:          NO];
    [open_panel setTitle:                           @"Iris Select Image"];
    [open_panel setPrompt:                          @"Open Slide"];
    [open_panel setMessage:                         @"Select Whole Slide to Load"];

    // Allow for Iris Codec files and SVS files for example.
    // We can accept any file format known to OpenSlide.
    auto IRIS = [UTType importedTypeWithIdentifier: @"com.iris.documents.iris"];
    auto SVS  = [UTType importedTypeWithIdentifier: @"com.iris.documents.svs"];
    [open_panel setAllowedContentTypes:             @[IRIS,SVS]];
    
    // Create a lambda function completion handler that drives the system to open the file
    // when a valid selection is made.
    [open_panel beginSheetModalForWindow:__window completionHandler:^(NSModalResponse status){
        if (status == NSModalResponseOK) {
            auto slide_file_path = [[[open_panel URL] path] UTF8String];
            Iris::viewer_open_slide(self.handle, Iris::SlideOpenInfo {
                .type           = Iris::SlideOpenInfo::SLIDE_OPEN_LOCAL,
                .local          = Iris::LocalSlideOpenInfo {
                    .filePath   = slide_file_path,
                }
            });
        }
    }];
}
// If the mouse is dragged, lets tell the engine to translate the view
- (void) mouseDragged:(NSEvent *)event
{
    Iris::viewer_engine_translate (self.handle, Iris::ViewerTranslateScope {
        .x_translate = static_cast<float>(event.deltaX / self.frame.size.width),
        .y_translate = static_cast<float>(event.deltaY / self.frame.size.height),
    });
}
// If a magnification gesture was interpreted by the window, change the scope zoom
- (void) magnifyWithEvent:(NSEvent *)event
{
    Iris::viewer_engine_zoom(self.handle, Iris::ViewerZoomScope {
        .increment  = static_cast<float>(event.magnification),
    });
}
// If a touch event was capture
- (void) touchesBeganWithEvent:(NSEvent *)event
{
    int touches = 0;
    float x = 0;
    float y = 0;
    for (NSTouch* touch in event.allTouches) {
        if (touch.resting) continue;
        touches ++;
        x += touch.normalizedPosition.x;
        y += touch.normalizedPosition.y;
    }
    
    // Record where the two fingers touch down
    // it will then be used by the tracker for 
    // movement / translation calls (touchesMovedWithEvent)
    if (touches != 2) return;
    _tracker        = TouchTracker {
        .x          = x / static_cast<float>(touches),
        .y          = y / static_cast<float>(touches),
        .x_vel      = 0.f,
        .y_vel      = 0.f,
        .timestamp  = get_current_microsecond_timestamp()
    };
}
// This is the primary means by which we will translate the scope view.
// Many pathologists use two fingers to shift the slide on the stage
// so we will attempt to replicate this motion on the trackpad:
// 2 fingers will shift the slide in the opposite direction the fingers move
// imitating the 'pushing' of a glass slide
- (void) touchesMovedWithEvent:(NSEvent *)event
{
    int touches     = 0;
    float x         = 0;
    float y         = 0;
    auto  t         = get_current_microsecond_timestamp ();
    // Look at each touch (finger) contacting trackpad.
    for (NSTouch* touch in event.allTouches) {
        if (touch.resting) continue;
        touches ++;
        x += touch.normalizedPosition.x;
        y += touch.normalizedPosition.y;
    }
    x /= static_cast<float>(touches); // Mean X location
    y /= static_cast<float>(touches); // Mean Y location
    t -= _tracker.timestamp;

    // Define the update (where we are now).
    TouchTracker update = {
        .x          = x,
        .y          = y,
        .timestamp  = t,
        .x_vel      = (_tracker.x_vel + (_tracker.x - x) / static_cast<float>(t / 1E7))/2.f,
        .y_vel      = (_tracker.y_vel + (_tracker.y - y) / static_cast<float>(t / 1E7))/2.f,
    };
    // If the velocity was really high, set a max of 2.0f
    if (abs(update.x_vel) > 2.f) update.x_vel = 2.f;
    if (abs(update.y_vel) > 2.f) update.y_vel = 2.f;
    
    // We have decided to have 2-finger touch events
    // be the events that translate the scope view.
    // This allows single finger movement to move the
    // cursor but not affect slide location.
    if (touches != 2) return;

    // If a shift key is pressed, interpret as a zoom command
    if ([event modifierFlags] & NSEventModifierFlagShift){
        Iris::viewer_engine_zoom(self.handle,{
            .increment = (_tracker.y - y) * 2
        });
    } 

    // Otherwise, the user is attempting to translate the view
    else {

        #if FALSE // MAKE TRUE for simple movement example
        // Here is a simple example of handling slide movement
        // the 2.5f is an arbitrary / empiric scale factor 
       Iris::viewer_engine_translate(self.handle, Iris::ViewerTranslateScope {
           .x_translate = - static_cast<float>(_tracker.x - x) * 2.5f,
           .y_translate =   static_cast<float>(_tracker.y - y) * 2.5f,
       });
        #else
        // This is a more advanced translation technique that incorporates
        // the relative velocity of the finger movement to increase the relative
        // scope view translation. The translation amount increases exponentially
        // with respect to the velocity of the finger movement. This makes movement 
        // 'feel' easier. All factors are arbitray / empiric based on trial and error.
        Iris::viewer_engine_translate(self.handle, Iris::ViewerTranslateScope {
            .x_translate = -(_tracker.x - x) * (std::pow(abs(update.x_vel), 2.f)/1.5f+2.f),
            .y_translate =  (_tracker.y - y) * (std::pow(abs(update.y_vel), 2.f)/1.5f+2.f),
            .x_velocity  = update.x_vel,
            .y_velocity  = update.y_vel,
            
        });
        #endif
    }
    
    // Always update the touch tracker afterwards
    _tracker = update;
}
// Keyboad event examples
// These show how to translate the view by a complete screen width or hight
// and were used in the publication to derive TeFOV values.
- (void) keyUp:(NSEvent *)event {
    // If 'C' is pressed, close the slide and open the open file dialog
    // to open a new file
    if (event.keyCode == 8) {
        Iris::viewer_close_slide(self.handle);
        [self openLoadSlideDialog];
    }

    // If LEFT arrow key pressed, move the scope view an entire screen width
    // to the left.
    else if (event.keyCode == 123 /*left*/) {
        Iris::viewer_engine_translate(self.handle, Iris::ViewerTranslateScope {
            .x_translate = 1.f,
            .y_translate = 0.f,
        });
    } 
    // If RIGHT arrow key pressed, move the scope view an entire screen width
    // to the right.
    else if (event.keyCode == 124 /*right*/) {
        Iris::viewer_engine_translate(self.handle, Iris::ViewerTranslateScope {
            .x_translate = -1.f,
            .y_translate = 0.f,
        });
    } 
    // If DOWN arrow key pressed, move the scope view an entire screen height down.
    else if (event.keyCode == 125 /*down*/) {
        Iris::viewer_engine_translate(self.handle, Iris::ViewerTranslateScope {
            .x_translate = 0.f,
            .y_translate = -1.f,
        });
    } 
    // If UP arrow key pressed, move the scope view an entire screen height up.
    else if (event.keyCode == 126 /*up*/) {
        Iris::viewer_engine_translate(self.handle, Iris::ViewerTranslateScope {
            .x_translate = 0.f,
            .y_translate = 1.f,
        });
    }
}
@end
