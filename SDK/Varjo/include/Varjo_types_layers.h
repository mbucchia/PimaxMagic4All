// Copyright 2019-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_TYPES_LAYERS_H
#define VARJO_TYPES_LAYERS_H

#include "Varjo_types.h"
#include "Varjo_export.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Swap chain configuration structure.
 */
struct varjo_SwapChainConfig2 {
    varjo_TextureFormat textureFormat;  //!< Texture format of the swap chain images
    int32_t numberOfTextures;           //!< Number of textures in the swap chain.
    int32_t textureWidth;               //!< Texture width in pixels.
    int32_t textureHeight;              //!< Texture height in pixels.
    int32_t textureArraySize;           //!< Texture array size, or 1 if creating a plain 2D texture swap chain.
};

/** Opaque type for the swap chain object. */
struct varjo_SwapChain;

/** Swap chain image source rectangle.  */
struct varjo_SwapChainViewport {
    struct varjo_SwapChain* swapChain;  //!< The swap chain to retrieve the image from.
    int32_t x;                          //!< X offset within the swap chain for the input rectangle.
    int32_t y;                          //!< Y offset within the swap chain for the input rectangle.
    int32_t width;                      //!< Width of the input rectangle in pixels.
    int32_t height;                     //!< Height of the input rectangle in pixels.
    int32_t arrayIndex;                 //!< Texture array slice (if the swap chain is a texture array), otherwise 0.
    int32_t reserved;                   //!< Reserved for future use.
};

/** Describes mirror view. Compositor will render specified view into specified viewport. */
struct varjo_MirrorView {
    struct varjo_SwapChainViewport viewport;  //!< The viewport where mirror view should be rendered.
    int32_t viewIndex;                        //!< Index of the view.
    int32_t reserved;                         //!< Reserved for future use.
};

typedef int64_t varjo_LayerType;
/** Layer type enum corresponding to #varjo_LayerMultiProj. */
static const varjo_LayerType varjo_LayerMultiProjType = 0x1;

typedef int64_t varjo_LayerFlags;
/** Default layer rendering */
static const varjo_LayerFlags varjo_LayerFlagNone = 0x0;

static const varjo_LayerFlags varjo_LayerFlag_BlendMode_AlphaBlend = 0x2;  //!< Set blend mode to be alpha blend.
static const varjo_LayerFlags varjo_LayerFlag_DepthTesting = 0x4;          //!< Set depth testing to enabled.
static const varjo_LayerFlags varjo_LayerFlag_InvertAlpha = 0x8;           //!< Invert alpha channel before blending.
static const varjo_LayerFlags varjo_LayerFlag_UsingOcclusionMesh = 0x10;   //!< Signal that occlusion mesh is in use.
static const varjo_LayerFlags varjo_LayerFlag_ChromaKeyMasking = 0x20;     //!< Layer is masked by video pass through chroma key if flag is given.
static const varjo_LayerFlags varjo_LayerFlag_Foveated = 0x40;             //!< Indicate to compositor that layer was rendered with foveation.

typedef int64_t varjo_Space;
/**
 * The local space, world-locked origin.
 */
static const varjo_Space varjo_SpaceLocal = 0x0;
/**
 * Reference space relative to the user's head.
 * This space can be used to render face-locked layers.
 */
static const varjo_Space varjo_SpaceView = 0x1;

/**
 * Layer header for different submission layer types.
 */
struct varjo_LayerHeader {
    /** The actual type of the layer; currently, the only supported type is varjo_LayerMultiProjType */
    varjo_LayerType type;
    /** Flags for this layer. See #varjo_LayerFlags. */
    varjo_LayerFlags flags;
};

typedef int64_t varjo_ViewExtensionType;
static const varjo_ViewExtensionType varjo_ViewExtensionDepthType = 1;
static const varjo_ViewExtensionType varjo_ViewExtensionDepthTestRangeType = 2;
static const varjo_ViewExtensionType varjo_ViewExtensionVelocityType = 3;
static const varjo_ViewExtensionType varjo_ViewExtensionBlendControlMaskType = 4;

/**
 * View Extension header.
 */
struct varjo_ViewExtension {
    /** View extension type. */
    varjo_ViewExtensionType type;
    /** Optional pointer to the next extension; allows chaining multiple extensions into a single view. */
    struct varjo_ViewExtension* next;
};

/**
 * A #varjo_LayerMultiProjView extension that allows submission of a depth surface
 * alongside the color surface for a view.
 */
struct varjo_ViewExtensionDepth {
    /** Extension header, with type = #varjo_ViewExtensionDepthType */
    struct varjo_ViewExtension header;
    /** Smallest depth value allowed in the depth surface, typically 0.0 */
    double minDepth;
    /** Largest depth value allowed in the depth surface, typically 1.0 */
    double maxDepth;
    /** Positive distance in meters. To indicate depth values reversed, can be greater than farZ. */
    double nearZ;
    /** Positive distance in meters. To indicate depth values reversed, can be smaller than nearZ. */
    double farZ;
    /** Image source for the depth surface. */
    struct varjo_SwapChainViewport viewport;
};

/**
 * A #varjo_LayerMultiProjView extension that defines range for which the depth test is enabled.
 * Outside the given range the layer is alpha-blended in the layer order.
 * This extension is only meaningful when used with #varjo_ViewExtensionDepth.
 *
 * When video-pass-through depth estimation is enabled using #varjo_MRSetVideoDepthEstimation,
 * the depth test range for video layer is limited to close range (hands) by default. If any
 * application specifies its own range by providing this extension, the application range is
 * used instead. This way application can use full range depth.
 */
struct varjo_ViewExtensionDepthTestRange {
    /** Extension header, with type = #varjo_ViewExtensionDepthTestRangeType */
    struct varjo_ViewExtension header;
    /** Positive distance in meters. */
    double nearZ;
    /** Positive distance in meters. */
    double farZ;
};

/**
 * A #varjo_LayerMultiProjView extension that allows submission of a pixel velocity surface
 * alongside the color surface for a view.
 */
struct varjo_ViewExtensionVelocity {
    /** Extension header, with type = #varjo_ViewExtensionVelocityType */
    struct varjo_ViewExtension header;
    /** Scale multiplier for all velocity vectors in the surface, so that
    after the scaling, the velocities are in pixels per second.*/
    double velocityScale;
    /** Flag to indicate if motion vectors include also the HMD motion or not.*/
    varjo_Bool includesHMDMotion;
    /** Image source for the velocity surface. */
    struct varjo_SwapChainViewport viewport;
};

/** Masking mode type. */
typedef int64_t varjo_BlendControlMaskingMode;
/** No masking. Masking layer is not added to composited scenes. */
static const varjo_BlendControlMaskingMode varjo_BlendControlMaskingMode_Off = 0;
/** Use chroma keying in masked area (or plain VST if not active), elsewhere show VR content. */
static const varjo_BlendControlMaskingMode varjo_BlendControlMaskingMode_Restricted = 1;
/** Show VR content in masked area, elsewhere use chroma keying (or plain VST if not active). */
static const varjo_BlendControlMaskingMode varjo_BlendControlMaskingMode_Extended = 2;
/** Show VST content in masked area, elsewhere use chroma keying (or plain VST if not active). */
static const varjo_BlendControlMaskingMode varjo_BlendControlMaskingMode_Reduced = 3;
/** Do VST depth test in masked area, elsewhere always fail depth test (show VST). */
static const varjo_BlendControlMaskingMode varjo_BlendControlMaskingMode_Depth_Test_Or_Always_Fail = 4;
/** Do VST depth test in masked area, elsewhere always pass depth test (show VR). */
static const varjo_BlendControlMaskingMode varjo_BlendControlMaskingMode_Depth_Test_Or_Always_Pass = 5;

/** Debug mode type. */
typedef int64_t varjo_BlendControlDebugMode;
/** No debug visualization. */
static const varjo_BlendControlDebugMode varjo_BlendControlDebugMode_Off = 0;
/** Visualize mask alpha value. */
static const varjo_BlendControlDebugMode varjo_BlendControlDebugMode_ShowMask = 1;
/** Visualize mask rgb values (meaningful only with RGBA texture format). */
static const varjo_BlendControlDebugMode varjo_BlendControlDebugMode_ShowColors = 2;

/**
 * A #varjo_LayerMultiProjView extension that defines blend control masking layer flags.
 */
struct varjo_ViewExtensionBlendControlMask {
    /** Extension header, with type = #varjo_ViewExtensionBlendControlMaskType */
    struct varjo_ViewExtension header;
    /** Force VR view offset for all clients when masking enabled. (See #varjo_MRSetVRViewOffset for more info) */
    varjo_Bool forceGlobalViewOffset;
    /** Masking mode used. */
    varjo_BlendControlMaskingMode maskingMode;
    /** Debug visualization mode. */
    varjo_BlendControlDebugMode debugMode;
    /** Reserved for future use. */
    int64_t reserved[32];
};

/**
 * A single view in a Multi-projection layer
 */
struct varjo_LayerMultiProjView {
    /**
     * An optional extension structure, or nullptr if none.
     */
    struct varjo_ViewExtension* extension;
    /** The projection matrix this view was rendered with. */
    struct varjo_Matrix projection;
    /** The view matrix this view was rendered with. */
    struct varjo_Matrix view;
    /** Image source for this view. See #varjo_SwapChainViewport for more info. */
    struct varjo_SwapChainViewport viewport;
};

/** Layer type describing a set of planar projected images rendered from the perspective of each eye. */
struct varjo_LayerMultiProj {
    /** The layer header. */
    struct varjo_LayerHeader header;
    /**
     * The reference space for this layer. See #varjo_Space for valid options.
     */
    varjo_Space space;
    /** The amount of entries in the #views array */
    int32_t viewCount;
    /** Array of views to be rendered. See #varjo_LayerMultiProjView for more information. */
    struct varjo_LayerMultiProjView* views;
};

/**
 * A structure describing the list of layers that the application submits to the compositor each frame.
 */
struct varjo_SubmitInfoLayers {
    /**
     * The frame number of the frame being submitted. This should match the frame number returned
     * in #varjo_FrameInfo when calling #varjo_WaitSync.
     */
    int64_t frameNumber;
    int64_t reserved;

    /** The number of elements in the #layers array below */
    int32_t layerCount;
    /**
     * Array of pointers to layer structures, one for each layer.
     * Note that the elements in this array are not really of #varjo_LayerHeader, but
     * rather structures such as #varjo_LayerMultiProj that contain #varjo_LayerHeader as
     * a first element. #varjo_LayerHeader.type is used to detect the actual structure type.
     * Currently the only supported layer type is #varjo_LayerMultiProjType, that corresponds
     * to #varjo_LayerMultiProj structure.
     *
     * When submitting multiple layers, the layers are rendered back-to-front in the order
     * they're provided in this array.
     */
    struct varjo_LayerHeader** layers;
};

#if defined __cplusplus
}
#endif

#endif
