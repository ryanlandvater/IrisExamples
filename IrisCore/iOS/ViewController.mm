//
//  ViewController.mm
//  iOS
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
// Pencil Kit is needed for using the Apple Pencil for annotations.
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <MetalKit/MetalKit.h>
#import <PencilKit/PencilKit.h>
#include <iostream>

@interface ViewController : UIViewController
- (void) destroyIrisView;
@end

// Import Iris
#import "IrisCore.hpp"

// ~~~~~~ IrisView ObjectiveC++ API WRAPPER ~~~~~~~~
// Iris View -> Per the Iris API, the Iris Viewer (Iris::Viewer) class
// is the entry class and interface class for the Iris API.
// We recommend wrapping it in a Metal View (MTKView) for a single concise
// ObjectiveC++ class (as the MTKView is needed for Iris Configuration).
// The only property needed is the Iris::Viewer handle.
@interface IrisView : MTKView <UIDocumentPickerDelegate, PKCanvasViewDelegate>
@property (unsafe_unretained, nonatomic) Iris::Viewer handle;
// There are only 3 functionalities this wrapper should handle:
// 1) initWithFrame (not re-defined here)
- (void) bindSurface;    // Bind a surface 
- (void) destroyViewer;  // Destroy viewer
// ** If you wish, you could not override initWithFrame and 
// fold all initialization routines into the bindSurface method.
// My preference simply is to break them up into separate calls.
@end
@implementation IrisView

// Initialize the Iris Viewer when creating the IrisView Objective C++ class.
// This will NOT generate the Iris Render Engine yet; A window must be bound before the engine
// is initialized. This is because the window provides the draw surface parameters needed for configuration.
- (instancetype) initWithFrame:(CGRect)frame
{
    // Provide the application name for which Iris Core Render Engine will provide scope rendering
    // Provide the Application Version
    // Provide the Application bundle path (the local path) for loading runtime files (REQUIRED).
    Iris::ViewerCreateInfo viewer_create_info {
        .ApplicationName = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"] UTF8String],
        .ApplicationVersion = static_cast<uint32_t>([[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"] integerValue]),
        .ApplicationBundlePath = [[[NSBundle mainBundle] resourcePath] UTF8String],
    };
    
    // Call the Iris API to Create the viewer.
    // This will NOT generate the render engine yet; just prepare the system.
    self = [super initWithFrame:frame];
    [self setHandle: Iris::create_viewer(viewer_create_info)];
    
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
- (void) resizeView
{
    // Most of this code involves letting the CaMetalLayer
    // calculate it's Apple specific internal systems.
    CGPoint position = [[UIScreen mainScreen] bounds].origin;
    position.x = position.x / 2;
    position.y = position.y / 2;
    
    [(CAMetalLayer*)[self layer] setContentsScale:[[UIScreen mainScreen] scale]];
    [(CAMetalLayer*)[self layer] setBounds:[[UIScreen mainScreen] bounds]];
    [(CAMetalLayer*)[self layer] setPosition: position];
    [[self layer] setFrame: self.bounds];
    
    // Inform Iris that the window resized. It will
    // invoke it's plug-and-play routines and self-format
    // to the resize.
    Iris::viewer_window_resized(self.handle);
}

// An example implementation of an open-file dialog to open slide files
- (void) openLoadSlideDialog
{
    // Only allow .iris Files
    auto IRIS   = [UTType importedTypeWithIdentifier:@"com.iris.documents.iris"];
    
    // File picker
    auto picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[IRIS]];
    [picker setAllowsMultipleSelection: NO];
    [picker setTitle:                   @"Iris Select Image"];
    [picker setModalPresentationStyle:  UIModalPresentationFormSheet];
    // ADDN DOCUMENT PICKER OPTIONS HERE
    [picker setDelegate:self];
    
    [self.window.rootViewController presentViewController:picker animated:YES completion:nil];
}
// What to do when the above Open Slide Dialog selects a file (ie how to open a file with Iris)
- (void) documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
    // Get the file path.
    auto file_path = [urls firstObject];
    
    // Ensure we have access to that file
    if (file_path.startAccessingSecurityScopedResource == NO) {
        NSLog(@"FAILED TO ACCESS SECURITY CONTROLLED RESOURCE");
        return;
    }
    
    // Open the slide by invoking the Iris API via the viewer
    Iris::viewer_open_slide(self.handle, Iris::SlideOpenInfo {
        .type           = Iris::SlideOpenInfo::SLIDE_OPEN_LOCAL,
        .local          = Iris::LocalSlideOpenInfo {
            .filePath   = [[file_path path] UTF8String],
        }
    });
}
// Add an annotation to the slide using the Apple Pencil / PencilKit API
// This occurs when a user draws an annotation and lifts the pencil up upon completion.
- (void) canvasViewDrawingDidChange:(PKCanvasView *)canvasView
{
    // If the cavas view received drawing strokes (ie >0 strokes)
    if (canvasView.drawing.strokes.count) {
        auto __drawing  = canvasView.drawing;
        auto __image    = [__drawing imageFromRect:__drawing.bounds scale:2.f/*Retina display 2x*/];
        auto __data     = UIImagePNGRepresentation(__image); // Get PNG version
        auto data       = Iris::Copy_strong_buffer_from_data(__data.bytes, __data.length); // Get img array
        Iris::SlideAnnotation slide_annotation {
            .format     = Iris::ANNOTATION_FORMAT_PNG, // PNG in this example
            // Fractional x and y location within the screen (self.frame.size) [0.0->1.0]
            .x_offset   = static_cast<float>(__drawing.bounds.origin.x   / self.frame.size.width), // X location
            .y_offset   = static_cast<float>(__drawing.bounds.origin.y   / self.frame.size.height),// Y location
            // Fractional size of the drawing within the screen ((self.frame.size) [0.0->1.0]
            .width      = static_cast<float>(__drawing.bounds.size.width / self.frame.size.width),
            .height     = static_cast<float>(__drawing.bounds.size.height / self.frame.size.height),
            // Byte pixel array
            .data       = data
        };
        // Invoke the Iris API via the viewer.
        Iris::viewer_annotate_slide(self.handle, slide_annotation);
        // Clear the canvas view of the drawing. Iris will take over drawing the image
        // within internal rendering systems. Failure to realloc the drawing will have 2 versions.
        canvasView.drawing = [[PKDrawing alloc] init];
    }
}

// Arbitarily assigned double tap to pull up the open-file dialog
- (void) doubleTapGesture: (UITapGestureRecognizer*) recognizer
{
    Iris::viewer_close_slide(self.handle);
    [self openLoadSlideDialog];
}

// Pan gesture to navigate across the slide.
- (void) panGesture: (UIPanGestureRecognizer*) recognizer
{
    auto fingers     = [recognizer numberOfTouches];
    auto translation = [recognizer translationInView:self];
    auto velocity    = [recognizer velocityInView:self];
    auto frame       = self.frame;
    
    // Provide the translate scope information (X/Y and displacement/velocity)
    Iris::ViewerTranslateScope translate_scope {
        // Get the translocation x location relative to the screen size [0.0->1.0]
        .x_translate = static_cast<float>(translation.x/frame.size.width) *
        // This below line moves the slide further based upon swipe velocity
        // I find that uses prefer saccade like movements to more quickly across the slide
                        (std::pow(static_cast<float>(velocity.x/frame.size.width)/3.f,2.f)+1),
        .y_translate = static_cast<float>(translation.y/frame.size.height) *
                        (std::pow(static_cast<float>(velocity.y/frame.size.height)/3.f,2.f)+1),
        // Get the relative velocity normalized to screen size
        .x_velocity  = static_cast<float>(velocity.x/frame.size.width),
        .y_velocity  = static_cast<float>(velocity.y/frame.size.height),
    };
    
    // Reset the recognizer's origin point so that each call is relative to previous
    [recognizer setTranslation:CGPoint{0,0} inView:self];
    
    // And translate the view
    Iris::viewer_engine_translate(self.handle, translate_scope);
}

// Pinch to adjust the zoom level of the slide view
- (void) pinchAdjustScale: (UIPinchGestureRecognizer*) recognizer
{
    // Divide the velocity by a scale to slow it down. 1.5e2 is an empiric value.
    Iris::viewer_engine_zoom(self.handle, Iris::ViewerZoomScope {
        .increment  = static_cast<float>(recognizer.velocity/recognizer.scale/1.5e2),
    });
}
@end

// Definition of the annotation view. This sits on top of the IrisViewer
// and allow for the passage of image annotations from the Apple Pencil Kit.
@interface AnnotationView : PKCanvasView
@end
@implementation AnnotationView
- (instancetype) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    [self setOpaque:NO];
    // Only allow annotations by the Pencil (UITouchTypePencil), else finger touches
    // for navigation will also draw, which is bad.
    [[self drawingGestureRecognizer] setAllowedTouchTypes:@[@(UITouchTypePencil)]];
    
    return self;
}
@end

@implementation ViewController
// DO NOT set up the Iris View here!
// The window has not been set up by the operating system yet
// and therefore cannot be used to properly size and configure Iris.
- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

// Iris NOW can be configured! The OS has decided the size of the window
// and can give this information to Iris for configuration.
- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    // Weakly reference the window
    UIWindow*       __window    = self.view.window;
    
    // ADDN WNDOW SETUP HERE (optional)
    
    // Create a view and set its size to the full size of the window if you want
    // Iris to fill up the full window draw space (most often).
    IrisView*       __view      = [[IrisView alloc] initWithFrame:__window.bounds];
    [__view setMultipleTouchEnabled:true];
    
    // Create a double tap gesture recognizer to implement
    // - (void) doubleTapGesture: (UITapGestureRecognizer*) recognizer; defined above
    id doubleTapGestureRecognizer   = [[UITapGestureRecognizer alloc] initWithTarget:__view action:@selector(doubleTapGesture:)];
    [doubleTapGestureRecognizer     setNumberOfTapsRequired:2];
    [doubleTapGestureRecognizer     setAllowedTouchTypes:@[@(UITouchTypeDirect), @(UITouchTypeIndirect)]];
    
    // Create a pan gesture recognizer for slide nagivation to implement
    // - (void) panGesture: (UIPanGestureRecognizer*) recognizer; defined above
    id panGestureRecognizer         = [[UIPanGestureRecognizer alloc] initWithTarget:__view action:@selector(panGesture:)];
    [panGestureRecognizer           setAllowedTouchTypes:@[@(UITouchTypeDirect), @(UITouchTypeIndirect)]];
    
    // Create a pinch gesture recognizer for zoom control to implement
    // - (void) pinchAdjustScale: (UIPinchGestureRecognizer*) recognizer;
    id pinchGestureRecognizer       = [[UIPinchGestureRecognizer alloc] initWithTarget:__view action:@selector(pinchAdjustScale:)];
    [__view addGestureRecognizer: doubleTapGestureRecognizer];
    [__view addGestureRecognizer: tapGestureRecognizer];
    [__view addGestureRecognizer: panGestureRecognizer];
    [__view addGestureRecognizer: pinchGestureRecognizer];
    
    // ADDN VIEW SETUP HERE
    
    // Finally bind the surface; This initializes the Iris Render Engine
    [self setView:__view];
    [__view bindSurface];
    
    // Create the Apple Pencil Pencilkit annotation view wrapper (defined above)
    // Set it as a deligate view to the IrisView
    // And make it a subview. (Learn more about Apple's iOS architecture if curious)
    AnnotationView* __markup    = [[AnnotationView alloc] initWithFrame:__window.bounds];
    [__markup                   setDelegate:__view];
    [__view                     addSubview:__markup];
    // Create a PKInkingTool that defines what the annotation looks like.
    // I chose to use a blue chisel marker style of pen.
    // I think this looks best / similar to marking pen, but it does not matter what you choose.
    __markup.tool               = [[PKInkingTool alloc] initWithInkType:PKInkTypeMarker color:[
        [UIColor alloc] initWithRed:0.08f green:0.3f blue:0.85f alpha:1.f] width:2.f];
    
    // In this example implementation, lets open a open-file dialog at the start.
    // We will need to select a slide file to render so there is no reason not to
    // begin by requesting that file.
    [__view openLoadSlideDialog];
}
// This is part of the iOS callback chain when the iOS device is rotated and the
// view changes size. This is primarily boiler plate code that calls the IrisView resizeView method.
- (void) viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    [coordinator animateAlongsideTransitionInView:self.view animation:^(id<UIViewControllerTransitionCoordinatorContext>  _Nonnull context) {
        if([self.view isKindOfClass:[IrisView class]])
            [(IrisView*)self.view resizeView];
    } completion: nil];
}
// Destroy the Iris View. This MUST be implemented due to
// the fact that Iris uses reference counted pointers and
// iOS uses something called automatic reference counting (ARC)
// These two memory management systems fight one another due to
// internals of how Vulkan and Apple's operating systems work.
// Calling this when closing the program mitigates these issues.
- (void) destroyIrisView {
    if ([self.view isKindOfClass:[IrisView class]])
        [(IrisView*)self.view destroyViewer];
}
@end
