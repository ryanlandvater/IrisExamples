# Iris iOS Example

The included file demonstrates how to extend the [UIViewController](https://developer.apple.com/documentation/uikit/uiviewcontroller) class of a iOS application to integrate Iris. Apple provides guides to create an initial iOS application [in the swift languge](https://developer.apple.com/ios/planning/). This guide can provide a basic framework to the developemnt of an application written in [Objective C](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Introduction/Introduction.html#//apple_ref/doc/uid/TP40011210), such as this example. When creating a new application, ensure you set the language to objective C or import Iris with Swift using a module map. 

An Iris View should be created that extends the functionality of a [Metal View / MTKView](https://developer.apple.com/documentation/metalkit/mtkview). In this instance, the Iris::Viewer is set as an unsafe (not objective C) property of this extended Metal View.
```ObjectiveC
@interface IrisView : MTKView <UIDocumentPickerDelegate, NSWindowDelegate>
@property (unsafe_unretained, nonatomic) Iris::Viewer handle;
@end
```

The Iris::Viewer can be initialized by creating the viewer instance. We provide information about the application we are writing to the underlying engine. The application bundle path is particularly important to access runtime resources. 
```ObjectiveC
Iris::ViewerCreateInfo ViewerCreateInfo {
	.ApplicationName        = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"] UTF8String],
	.ApplicationVersion     = U32_CAST([[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"] integerValue]),
	.ApplicationBundlePath  = [[[NSBundle mainBundle] resourcePath] UTF8String],
};
self = [super initWithFrame:frameRect];
[self setHandle: Iris::create_viewer(ViewerCreateInfo)];
```

The Iris::Viewer must bind the MKTView's underlying [CAMatalLayer](https://developer.apple.com/documentation/quartzcore/cametallayer), which is the Core Animation layer that Iris can render into to be displayed onscreen. This will allow the Iris Viewer to initialize the underlying rendering engine.
```ObjectiveC
- (void) bindSurface
{
    Iris::ViewerBindExternalSurfaceInfo bind_info {
        .viewer = self.handle,
        .layer  = (__bridge const void*) self.layer,
    };

    Iris::viewer_bind_external_surface (bind_info);
}
```