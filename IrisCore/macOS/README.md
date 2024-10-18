# Iris macOS Example

The included file demonstrates how to extend the [NSViewController](https://developer.apple.com/documentation/appkit/nsviewcontroller) class of a macOS application to integrate Iris. Apple provides guides to create an initial macOS application [in the swift languge](https://developer.apple.com/tutorials/swiftui/creating-a-macos-app/). This guide can provide a basic framework to the developemnt of an application written in [Objective C](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Introduction/Introduction.html#//apple_ref/doc/uid/TP40011210), such as this example. When creating a new application, ensure you set the language to objective C or import Iris with Swift using a module map. 

The extension of the [NSViewController](https://developer.apple.com/documentation/appkit/nsviewcontroller) is the basic method for implementing custom code in an macOS application. This is achieved by overriding standard methods. 

[!NOTE]
The default method for initialization Apple proposes `NSViewController::viewDidLoad()` will incorrectly return the window dimensions to the MTKView / the render engine on retina displays when Iris binds the window surface. You must instead override `NSViewController::viewWillAppear()` for initialization instead for high-DPI monitors.

An example of the initialization via overriding `NSViewController::viewWillAppear()` is shown below.
```ObjectiveC
@implementation ViewController
- (void) viewWillAppear {
    [super viewWillAppear];

    NSWindow* __window  = self.view.window;
    IrisView* __view    = [[IrisView alloc] initWithFrame:self.view.frame];
    [self setView: __view];
    
    [__view bindSurface];
    [__window makeFirstResponder:__view];
    [__window setDelegate:__view];
}
// Additional ViewController implementations here (see example file for more detail)...
@end
```


An Iris View should be created that extends the functionality of a [Metal View / MTKView](https://developer.apple.com/documentation/metalkit/mtkview). In this instance, the Iris::Viewer is set as an unsafe (not objective C) property of this extended Metal View.
```ObjectiveC
@interface IrisView : MTKView <UIDocumentPickerDelegate, NSWindowDelegate>
@property (unsafe_unretained, nonatomic) Iris::Viewer handle;
- (instancetype) initWithFrame:(NSRect)frameRect;
- (void) bindSurface;
- (void) destroyViewer
@end
```

The Iris::Viewer can be initialized by creating the viewer instance. We provide information about the application we are writing to the underlying engine. The application bundle path is particularly important to access runtime resources. 
```ObjectiveC
- (instancetype) initWithFrame:(NSRect)frameRect
{
    Iris::ViewerCreateInfo ViewerCreateInfo {
        .ApplicationName        = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"] UTF8String],
        .ApplicationVersion     = U32_CAST([[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"] integerValue]),
        .ApplicationBundlePath  = [[[NSBundle mainBundle] resourcePath] UTF8String],
    };
    
	// Init MTKLayer
    self = [super initWithFrame:frameRect];
    [self setHandle: Iris::create_viewer(ViewerCreateInfo)];
    return self;
}
```

The Iris::Viewer must bind the MKTView's underlying [CAMatalLayer](https://developer.apple.com/documentation/quartzcore/cametallayer), which is the Core Animation layer that Iris can render into to be displayed onscreen. This will allow the Iris Viewer to initialize the underlying rendering engine.
```ObjectiveC
- (void) bindSurface
{
    Iris::ViewerBindExternalSurfaceInfo bind_info {
        .viewer = self.handle,
		// MTKLayer::self.layer = CAMetalLayer
        .layer  = (__bridge const void*) self.layer,
    };

    Iris::viewer_bind_external_surface (bind_info);
}
```

Iris will be initialized at this point. Because of how Apple's Automatic Reference Counter works, you must destroy the Iris::Viewer at program shutdown. This is because uses its own internal reference counting that was built to be used outside of Apple's platforms.
```ObjectiveC
- (void) destroyViewer
{
    if (self.handle != nullptr)
        [self setHandle: nil];
}
```

In order to initialize the macOS application, most