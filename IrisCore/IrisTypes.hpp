/**
 * @file IrisTypes.hpp
 * @author Ryan Landvater
 * @brief Iris Core API Types and Structure Definitions
 * @version 2024.0.3
 * @date 2023-08-26
 * 
 * @copyright Copyright (c) 2023-24
 * Created by Ryan Landvater on 8/26/23.
 * 
 * \note ALL STRUCTURE Variables SHALL have variables named in cammelCase
 * \note ALL CLASSES Variables SHALL have underscores with _cammelCase
 * \note ALL LOCAL variables SHALL use lower-case snake_case
 * 
 */


#ifndef IrisTypes_h
#define IrisTypes_h

namespace Iris {
/**
 * @brief Result flags returned by Iris as part of API calls.
 * 
 */
enum Result {
    IRIS_SUCCESS = 0,
    IRIS_FAILURE = 1
};

/**
 * @brief Reference counted data object used to wrap datablocks.
 * 
 * It can either strong reference or weak reference the underlying data. 
 * The buffer can also shift between weak and strong referrences if chosen; 
 * however, this is very dangerous obviously and you need to ensure you 
 * are tracking if you have switched from weak to strong or vice versa.
 */
using Buffer = std::shared_ptr<class __INTERNAL__Buffer>;
using Viewer = std::shared_ptr<class  __INTERNAL__Viewer>;
using Slide  = std::shared_ptr<class  __INTERNAL__Slide>;

/**
 * @brief Defines necesary runtime parameters for starting the Iris rendering engine.
 * 
 * @param ApplicationName informs the rendering engine of the calling application's name
 * @param ApplicationVersion informs the engine of the calling application version
 * @param ApplicationBundlePath provides the executable location. This is needed for runtime
 * loading of application files such as UI markup files and shader code.
 * 
 */
struct ViewerCreateInfo {
    const char*         ApplicationName;
    uint32_t            ApplicationVersion;
    const char*         ApplicationBundlePath;
};
/**
 * @brief  System specific binding information to configure Iris' rendering engine
 * for the given operating system draw surface. 
 * 
 * Compilier macros control the structure's definition and backend implementation
 * and thus define the nature of the OS draw surface handles.
 *  - Windows: requires HINSTANCE and HWND handles from the WIN32 API
 *  - Apple: macOS and iOS require a __bridge pointer to a CAMetalLayer
 * 
 */
struct ViewerBindExternalSurfaceInfo {
    const Viewer        viewer      = nullptr;  
#if defined _WIN32
    HINSTANCE           instance    = NULL;    
    HWND                window      = NULL;    
#elif defined __APPLE__
    const void*         layer       = nullptr; 
#endif
};
/**
 * @brief  Information to translate the rendered scope view as a fraction of the active
 * view space with direction given by the sign.
 * 
 * An x translation value of 0.5 will shift the view to the right by half of the current
 * view sapce while -1.0 will shift the scope view to the left by an entire screen.
 * 
 */
struct ViewerTranslateScope {
    float               x_translate = 0.f;
    float               y_translate = 0.f;
    float               x_velocity  = 0.f;
    float               y_velocity  = 0.f;
};
/**
 * @brief Information to change the zoom objective.
 * 
 * A positive zoom increment will increase the scope view 
 * zoom while a negative increment will decrease the current zoom.
 * 
 */
struct ViewerZoomScope {
    float               increment   = 0.f;
};
/**
 * @brief Defines the image encoding format for an image annotation.
 * 
 */
enum AnnotationFormat {
    ANNOTATION_FORMAT_UNDEFINED     = -1,
    ANNOTATION_FORMAT_PNG,
    ANNOTATION_FORMAT_JPEG,
};
/**
 * @brief Structure defining requirements to create an image-based
 * slide annotation.
 * 
 * The required information includes the location of the slide annotation
 * on the slide and the size of the annotation. The offset locations are 
 * fractions of the current view window (for example an annotation that
 * starts in the middle of the current view would have an offset of 0.5)
 * The engine will immediately begin rendering the image on top of the 
 * rendered slide layers.
 * 
 * @param format the AnnotationFormat of the image data to be rendered
 * @param x_offset the x_offset of the current scope view window where the image starts [0,1.f]
 * @param y_offset the x_offset of the current scope view window where the image starts [0,1.f]
 * @param width the number of horizontal pixels in the image annotation
 * @param height the number of vertical pixels in the image annotation
 * @param data the encoded pixel data that comprises the image, width wide and hight tall
 * 
 */
struct SlideAnnotation {
    AnnotationFormat    format      = ANNOTATION_FORMAT_UNDEFINED;
    float               x_offset    = 0.f;
    float               y_offset    = 0.f;
    float               width       = 0.f;
    float               height      = 0.f;
    Buffer              data;
};
/**
 * @brief  Slide objective layer extent detailing the extent of each objective layer in
 * the number of 256 pixel tiles in each dimension.  
 * 
 * The relative scale (zoom amount) as well as how downsampled the layer is relative to 
 * the highest zoom layer (the reciprocal of the scale).
 * 
 * @param xTiles Number of horizontal 256 pixel tiles
 * @param yTiles Number of vertical 256 pixel tiles
 * @param scale Zoom factor of this level
 * @param downsample Reciprocal zoom factor relative to most zoomed layer (one at highest objective layer)
 */
struct LayerExtent {
    uint32_t            xTiles      = 1; 
    uint32_t            yTiles      = 1; 
    float               scale       = 1.f;
    float               downsample  = 1.f;
};
using LayerExtents = std::vector<LayerExtent>;
/**
 * @brief The extent, in pixels, of a whole side image file. 
 * 
 * These are in terms of the initial layer presented (most zoomed out layer).
 * 
 * @param width Base layer width extent in pixels
 * @param height Base layer height extent in pixels
 * @param layers Slide objective layer extent list
 */
struct Extent {
    uint32_t            width       = 1; 
    uint32_t            height      = 1; 
    LayerExtents        layers; 
};
/**
 * @brief Image channel byte order in little-endian format
 * 
 * Assign this format to match the image source bits per
 * pixel and bit-ordering. 
 */
enum Format {
    FORMAT_UNDEFINED,
    FORMAT_B8G8R8,
    FORMAT_R8G8B8,
    FORMAT_B8G8R8A8,
    FORMAT_R8G8B8A8,
};
/**
 * @brief Information to open a slide file located on a local volume.
 * 
 * Provide the file location and the 
 * 
 */
struct LocalSlideOpenInfo {
    const char*         filePath;
    /**
     * @brief Local slide file encoding type
     * 
     * This informs the Iris::Slide object how it should
     * attempt to open and map the slide file. If unknown,
     * it will attempt both encoding sequences. OpenSlide
     * is not supported on all platforms (iOS for example).
     * 
     */
    enum : uint8_t {
        SLIDE_TYPE_UNKNOWN,         // Unknown file encoding
        SLIDE_TYPE_IRIS,            // Iris Codec File
        SLIDE_TYPE_OPENSLIDE,       // Vendor specific file (ex SVS)
    }                   type        = SLIDE_TYPE_UNKNOWN;
    
};
/**
 * @brief Information needed to open a server-hosted slide file.
 * 
 * This requires use of the Iris Networking module.
 * 
 */
struct NetworkSlideOpenInfo {
    const char*         slideID;
};
/**
 * @brief Parameters required to create an Iris::Slide WSI file handle.
 * 
 * This parameter structure is a wrapped union of either
 * a local slide file open information struct or a network hosted
 * slide file open information struct. To allow the system to access
 * the correct union member, a type enumeration must also be defined
 * prior to passing this information stucture to the calling method
 * Iris::create_slide(const SlideOpenInfo&) or
 * Iris::viewer_open_slide(const Viewer&, const SlideOpenInfo&)
 * 
 * Optional parameters that can be used to optimize performance
 * characteristics are also included in the struct. Some are used interally
 * by the Iris rendering engine, and these are invoked when using 
 * Iris::viewer_open_slide(const Viewer&, const SlideOpenInfo&)
 * rather than the more generic Iris::create_slide(const SlideOpenInfo&),
 * so the former should be preferred when available.
 * 
 */
struct SlideOpenInfo {
    enum : uint8_t {
        SLIDE_OPEN_UNDEFINED,           // Default / invalid file
        SLIDE_OPEN_LOCAL,               // Locally accessible / Mapped File
        SLIDE_OPEN_NETWORK,             // Sever hosted slide file
    }                   type            = SLIDE_OPEN_UNDEFINED;
    union {
    /**
     * @brief Information for opening a file on the local machine
     */
    LocalSlideOpenInfo   local;
    /**
     * @brief Information for opening a network hosted file
     */
    NetworkSlideOpenInfo network;
    };
    // ~~~~~~~~~~~~~ OPTIONAL FEATURES ~~~~~~~~~~~~~~~ //
    /**
     * @brief This is the default slide cache capacity
     *
     * The capacity determines the number of allowed cached tiles.
     * This is the primary way in which Iris consumes RAM.
     * Greater values cache more in-memory decompressed tile data
     * for greater performance. Less require more pulls from
     * disk (which is slower)
     * The default 1000 for RGBA images consumes 2 GB of RAM.
     */
    size_t               capacity       = 1000;
};
using LambdaPtr         = std::function<void()>;
using LambdaPtrs        = std::vector<LambdaPtr>;
} // END IRIS NAMESPACE