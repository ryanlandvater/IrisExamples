//
//  IrisTypes.hpp
//  Iris
//
//  Created by Ryan Landvater on 8/26/23.
//


// ALL STRUCTURE Variables SHALL have variables named in cammelCase
// ALL CLASSES Variables SHALL have underscores with _cammelCase
// ALL LOCAL variables SHALL use lower-case snake_case

#ifndef IrisTypes_h
#define IrisTypes_h

#define U8_CAST(X)          static_cast<uint8_t>(X)
#define U16_CAST(X)         static_cast<uint16_t>(X)
#define U32_CAST(X)         static_cast<uint32_t>(X)
#define FLOAT_CAST(X)       static_cast<float>(X)
#define BYTE_PTR_CAST(X)    static_cast<BYTE*>(X)
#define VOID_PTR_CAST(X)    reinterpret_cast<void*>(X)
#define ATOMIC(X)           std::atomic<X>
#define TILE_PIX_LENGTH     256U
#define TILE_PIX_FLOAT      256.f
#define TILE_PIX_AREA       65536U
#define TILE_PIX_BYTES_RGB  196608U
#define TILE_PIX_BYTES_RGBA 262144U
#define LAYER_STEP          4
#define LAYER_STEP_FLOAT    4.f

namespace Iris {
using BYTE              = uint8_t;
using BYTE_ARRAY        = std::vector<BYTE>;
using CString           = std::vector<char>;
using CStringList       = std::vector<const char*>;
using LambdaPtr         = std::function<void()>;
using LambdaPtrs        = std::vector<LambdaPtr>;
using atomic_bool       = std::atomic<bool>;
using atomic_byte       = std::atomic<uint8_t>;
using atomic_sint8      = std::atomic<int8_t>;
using atomic_uint8      = std::atomic<uint8_t>;
using atomic_sint16     = std::atomic<int16_t>;
using atomic_uint16     = std::atomic<uint16_t>;
using atomic_sint32     = std::atomic<int32_t>;
using atomic_uint32     = std::atomic<uint32_t>;
using atomic_sint64     = std::atomic<int64_t>;
using atomic_uint64     = std::atomic<uint64_t>;
using atomic_size       = std::atomic<size_t>;
using atomic_float      = std::atomic<float>;
using Threads           = std::vector<std::thread>;
using Mutex             = std::mutex;
using MutexLock         = std::unique_lock<Mutex>;
using SharedMutexLock   = std::shared_ptr<MutexLock>;
using SharedMutex       = std::shared_mutex;
using ExclusiveLock     = std::unique_lock<SharedMutex>;
using SharedLock        = std::shared_lock<SharedMutex>;
using ReadLock          = std::shared_lock<SharedMutex>;
using WriteLock         = std::unique_lock<SharedMutex>;
using Notification      = std::condition_variable;
using FilePaths         = std::vector<const char*>;
using CallbackDict      = std::unordered_map<std::string, LambdaPtr>;
using ViewerWeak        = std::weak_ptr<class __INTERNAL__Viewer>;

using LayerIndex        = uint32_t;
using TileIndex         = uint32_t;
using ImageIndex        = uint32_t;
using TileIndicies      = std::vector<TileIndex>;
using TileIndexSet      = std::unordered_set<TileIndex>;
using ImageIndicies     = std::vector<ImageIndex>;


enum Result {
    IRIS_SUCCESS = 0,
    IRIS_FAILURE = 1
};

/// Iris Buffer is a reference counted data object used to wrap datablocks. It can either strong reference or
/// weak reference the underlying data. The buffer can also shift between weak and strong referrences
/// if chosen; however, this is very dangerous obviously and you need to ensure you are tracking if you
/// have switched from weak to strong or vice versa.
using Buffer = std::shared_ptr<class __INTERNAL__Buffer>;

/// Iris Viewer Object is defines the API access point for calling applications.
/// Calling applications interact with a viewer to render a scope, draw and interact
/// with Iris Native User Interface Elements, and extend scope view functionality.
using Viewer = std::shared_ptr<class  __INTERNAL__Viewer>;

/// ViewerCreateInfo Structure defines necesary modificaitons to hte underlying
/// rendering engine. This includes markup files for different scenes and callback
/// methods taht can be called by the custom UI defined in the markup files. For
/// example a UI that defines a button should provide a callback for what happens
/// if that button is pressed.
struct ViewerCreateInfo {
    const char*         ApplicationName;
    uint32_t            ApplicationVersion;
    const char*         ApplicationBundlePath;
    FilePaths           UI_markups;
    CallbackDict        UI_callbacks;
};
struct ViewerBindExternalSurfaceInfo {
    const Viewer        viewer      = nullptr;
#if defined _WIN32
    HINSTANCE     instance          = NULL;
    HWND          window            = NULL;
#elif defined __APPLE__
    const void*         layer       = nullptr;
#endif
};
struct ViewerMouseMoved {
    float               x_location  = 0.f;
    float               y_location  = 0.f;
    bool                L_pressed   = false;
    bool                R_pressed   = false;
    float               x_velocity  = 0.f;
    float               y_velocity  = 0.f;
};
struct ViewerMouseEvent {
    enum {
        UNDEFINED_EVENT,
        MOUSE_LEFT_DOWN,
        MOUSE_LEFT_UP,
        MOUSE_RIGHT_DOWN,
        MOUSE_RIGHT_UP,
        MOUSE_LEFT_CLICK,
        MOUSE_RIGHT_CLICK,
        MOSUE_LEFT_DOUBLE_CLICK,
        MOUSE_RIGHT_DOUBLE_CLICK
    }                   type        = UNDEFINED_EVENT;
    float               x_location  = 0.f;
    float               y_location  = 0.f;
};
struct ViewerMultigesture {
    float               x_location  = 0.f;
    float               y_location  = 0.f;
    uint32_t            n_fingers   = 0;
};
struct ViewerTranslateScope {
    float               x_translate = 0.f;
    float               y_translate = 0.f;
    float               x_velocity  = 0.f;
    float               y_velocity  = 0.f;
};
struct ViewerZoomScope {
    float               increment   = 0.f;
};
enum AnnotationFormat {
    ANNOTATION_FORMAT_UNDEFINED     = -1,
    ANNOTATION_FORMAT_PNG,
    ANNOTATION_FORMAT_JPEG,
};
struct SlideAnnotation {
    AnnotationFormat    format      = ANNOTATION_FORMAT_UNDEFINED;
    float               x_offset    = 0.f;
    float               y_offset    = 0.f;
    float               width       = 0.f;
    float               height      = 0.f;
    Buffer              data;
};
struct ViewerMeasureSlide {
    float               x_start     = 0.f;
    float               y_start     = 0.f;
    float               x_end       = 0.f;
    float               y_end       = 0.f;
};

struct LayerExtent {
    uint32_t            xTiles      = 1;
    uint32_t            yTiles      = 1;
    float               scale       = 1.f;
    float               downsample  = 1.f;
};
using LayerExtents = std::vector<LayerExtent>;
struct Extent {
    uint32_t            width       = 1;
    uint32_t            height      = 1;
    LayerExtents        layers;
};
enum Format {
    FORMAT_UNDEFINED,
    FORMAT_B8G8R8,
    FORMAT_R8G8B8,
    FORMAT_B8G8R8A8,
    FORMAT_R8G8B8A8,
};

/// Iris Slide  encapsulates the slide data retrieval system used by Iris. It is the
/// recommended access point for data that either uses or does not use the
/// render engine and viewer. 
using Slide = std::shared_ptr<class  __INTERNAL__Slide>;
struct LocalSlideOpenInfo {
    const char*         filePath;
    enum : uint8_t {
        SLIDE_TYPE_UNKNOWN,         // Unknown file encoding
        SLIDE_TYPE_IRIS,            // Iris Codec File
        SLIDE_TYPE_OPENSLIDE,       // Vendor specific file (ex SVS)
    }                   type        = SLIDE_TYPE_UNKNOWN;
    
};
struct NetworkSlideOpenInfo {
    const char*         slideID;
};
struct SlideOpenInfo {
    enum : uint8_t {
        SLIDE_OPEN_UNDEFINED,           // Default / invalid file
        SLIDE_OPEN_LOCAL,               // Locally accessible / Mapped File
        SLIDE_OPEN_NETWORK,             // Sever hosted slide file
    }                   type            = SLIDE_OPEN_UNDEFINED;
    union {
    LocalSlideOpenInfo   local;
    NetworkSlideOpenInfo network;
    };
    // ~~~~~~~~~~~~~ OPTIONAL FEATURES ~~~~~~~~~~~~~~~ //
    // This is the default slide cache capacity
    // it determines the number of allowed cached tiles.
    // The default 1000 for RGBA images is 2 GB
    size_t               capacity       = 1000;
    // Advanced efficiency feature to avoid loading
    // stale / irrelevant tiles. Point to the current
    // high-resolution layer. The slide will ignore
    // any prior load requests that are not the
    // high or low (HR-1) resolution layers.
    atomic_uint32*       HR_index_ptr   = nullptr;
    // Advanced efficiency feature. Notifies once a tile
    // has been loaded into the slide tile cache and is
    // ready for use. Useful for updating the view via
    // informing a buffering thread that new data is available.
    Notification*        notification   = nullptr;
};

typedef std::shared_ptr<class  __INTERNAL__Buffer> Buffer;

} // END IRIS NAMESPACE

#endif /* IrisTypes_h */
