//
//  IrisCore.hpp
//  Iris
//
//  Created by Ryan Landvater on 8/26/23.
//  Copyright Ryan Landvater 2023-24
//  All rights reserved.
//

#include <stdint.h>
#include <vector>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <thread>
#include <shared_mutex>
#include <functional>
#include "IrisTypes.hpp"


#ifndef IrisCore_h
#define IrisCore_h

namespace Iris {
/// Get the major build version of Iris that you are using.
int get_major_version ();
/// Get the minor build version of Iris that you are using.
int get_minor_version ();
/// Get the current build number.
int get_build_number  ();


/// The viewer the the primary control class that interfaces between external applications and
/// their views, and the iris rendering system. It contains interface capabilities between external
/// controllers, coordinates display presentations between external surfaces, and creates any
/// user interface functionalities defined in user interface markup strctures. 
using Viewer = std::shared_ptr <class __INTERNAL__Viewer>;

/// Create in Iris image viewer. The viewer is the primary interface between the whole slide
/// rendering system and the calling application that is using Iris to draw slide views.
/// This function ONLY CREATES / INSTANTIATES the viewer but does NOT initialize it.
/// The viewer must be bound to a application surface generated by the operating system
/// before it can be used. Use the below @ref viewer_bind_external_surface to bind and
/// initialize the system.
Viewer create_viewer                (const ViewerCreateInfo&);

/// Bind a viewer to an external surface controlled by the calling application.
/// the provided surface MUST outlive the link between the viewer and surface.
/// Unbind the viewer before destroying the view or allow the viewer to exit scope
/// and it will automatically unbind the surface.
bool viewer_bind_external_surface   (const ViewerBindExternalSurfaceInfo&);

/// Unbind the external drawing surface controlled by the calling application.
/// NOTE: For Objective C systems, this or the destructor MUST be called due to ARC.
void viewer_unbind_surface          (const Viewer& viewer);

/// Inform a viewer that the attached window was resized. This will force a reconstruction of the
/// viewer rendering engine's swapchain to accomodate the new window size.
void viewer_window_resized          (const Viewer& viewer);

/// Request the viewer create and open a slide for viewing. This will close any currently opened slide and attempt to
/// open the slide with the access information defined within the Slide Open Info structure.
void viewer_open_slide              (const Viewer& viewer, const SlideOpenInfo&);

/// Request the viewer load and begin rendering the supplied Iris Slide. This will close any currently opened slide
/// and attempt to render the provided slide object, if valid.
void viewer_open_slide              (const Viewer& viewer, const Slide&);

/// Close the current slide being viewed by the viewer. Nothing happens if no slide is loaded.
void viewer_close_slide             (const Viewer& viewer);

/// Get the current slide being viewed by the viewer. Returns a Slide nullptr on failure
Slide viewer_get_active_slide       (const Viewer& viewer);

void viewer_mouse_moved             (const Viewer& viewer, const ViewerMouseMoved&) noexcept;

void viewer_mouse_event             (const Viewer& viewer, const ViewerMouseEvent&) noexcept;

void viewer_multigesture_event      (const Viewer& viewer, const ViewerMultigesture&) noexcept;

void viewer_engine_translate        (const Viewer& viewer, const ViewerTranslateScope&) noexcept;

void viewer_engine_zoom             (const Viewer& viewer, const ViewerZoomScope&) noexcept;

void viewer_engine_set_gamma        (const Viewer& viewer, const float gamma_value) noexcept;

/// Insert an image slide annotation into the current active slide at the location within the screen
void viewer_annotate_slide          (const Viewer& viewer, const SlideAnnotation&) noexcept;

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//     Iris Viewer UI System                                                //
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

//Result viewer_create_scene          (const Viewer&, std::string& SceneName) noexcept;
//
//Result viewer_add_objects_to_scene  (const Viewer&, std::string& SceneName, UI::Object& rootObject) noexcept;

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//     Iris Slide Image Handler                                             //
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

Slide create_slide                  (const SlideOpenInfo&);

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//      Data Buffer Wrapper                                                 //
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

/// Create a STRONG buffer without memory backing. Configure the memory backing afterwards.
/// PLEASE NOTE, despite being a strong buffer, there is no owned data block yet. It must be initialized first.
/// @return Newly constructed strong buffer object with 'capacity' and 'size' of 0-bytes, ie NO backed memory.
/// NULL-ptr buffer is returned on failure. This is copy constructable and will maintain the life of the underlying
/// data (once allocated) as long as one copy persists.
Buffer  Create_strong_buffer        ();

/// Create a STRONG blank buffer with an initial capacity of @ref "buffer_size_in_bytes" bytes long.
/// The size will be 0, despite the capacity being defined.
/// @arg size_t buffer_size_in_bytes: the initial CAPACITY (in bytes). NOTE: 'size' is '0' bytes
/// @return Newly constructed strong buffer object with 'capacity' of given bytes and size of '0', ie blank buffer.
/// NULL-ptr buffer is returned on failure. This is copy constructable and will maintain the life of the underlying
/// data as long as one copy persists.
Buffer  Create_strong_buffer        (size_t buffer_size_in_bytes);

/// Create a buffer and copy the data pointed to by @ref dataptr and @ref bytes in length (in bytes).
/// This will return a STRONG buffer (one that owns the underlying data). The data pointed to by dataptr
/// will be copied into the returned buffer and the data source can be safely freed at any time.
/// @return Newly constructed strong buffer object. NULL-ptr buffer on failure. This is copy constructable
/// and will maintain the life of the underlying data as long as one copy persists.
Buffer Copy_strong_buffer_from_data (const void* data_ptr, size_t bytes);

/// Wrap a weak buffer around foreign data. This wrapper is used for implementing Iris Codec functions on
/// foreign data blocks without having to copy the data. It is a convenience function for such tasks as decoding
/// network derived large buffers sequences to reduce data copy and redundancy. Weak buffers can become strong
/// should it be required. Doing so will force the buffer to adopt responsibility for freeing the underlying data
/// AND MAY CHANGE/INVALIDATE the pointer to the underlying data should it need to expand the block.
/// @return Newly constructed weak buffer object. NULL-ptr buffer on failure. This is copy constructable but
/// freeing the underlying data will have undefined behavior.
Buffer  Wrap_weak_buffer_fom_data   (const void* const data_ref, size_t bytes);

/// Write data into a buffer in a safe manner. This is useful if you don't want to include the IrisCodec_buffer.h header
/// and expose yourself to accidentally using it wrong and corrupting your data. This function works like a c-style
/// array exposed to a method for writing. Provide the buffer and size. This method functions differently based
/// upon the strength of the reference (see discussion below)
/// @par If the reference is **WEAK**, this method will expose the begining of the buffer sequence OR THROW AN
/// EXCEPTION if there is insufficient space within the buffer (as weak buffers are not permitted to expand a buffer).
/// If you are worried about a buffer overflow, you may consider strengthening the buffer reference to allow for expansion.
/// @par If the reference is **STRONG**, this method will expose the begining of the next writable segment and will
/// expand the buffer if there is insufficient space. If the buffer is new, the exposed segment will be the begining
/// of the buffer sequence. The buffer's internal size metric will reflect the new data.
/// @return A c-style array pointer to the writable memory (for memcopy or other api) or a NULL-pointer
/// in the event of failure.
void*   Buffer_write_into_buffer    (const Buffer&, size_t bytes);

/// Extract the data reference from the underlying buffer structure. This is useful if you don't want to include
/// the IrisCodecBuffer.h header and expose yourself to accidentally using it incorrectly and corrupting your memory.
void    Buffer_get_data             (const Buffer&, void*& data, size_t& bytes);
}

#endif /* IrisCore_h */