// Copyright 2019-2023 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_LAYERS_H
#define VARJO_LAYERS_H

#include "Varjo.h"
#include "Varjo_types.h"
#include "Varjo_types_layers.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Create a D3D11 swap chain that can be used when submitting layers.
 * A swap chain object contains the image surfaces that can be rendered into, and submitted to the Varjo Compositor for displaying.
 * Each swap chain contains multiple images in order to perform double- or triple-buffering when compositing the image.
 * The amount of swap chain images created is configured in the numberOfTextures member of the #varjo_SwapChainConfig2 struct.
 *
 * Each frame submission contains references to 1..n swap chain objects; the application is free to render all views
 * into a single swap chain image side-by-side as an atlas, or use a separate swap chain objects for each view (or any other combination imaginable).
 *
 * Use #varjo_FreeSwapChain to free the created swap chain.
 *
 * Possible errors:
 *   - #varjo_Error_NullPointer: config pointer is a null pointer.
 *   - #varjo_Error_ValidationFailure: #varjo_SwapChainConfig2::textureWidth or #varjo_SwapChainConfig2::textureHeight is outside range [0, 16384],
 *     #varjo_SwapChainConfig2::textureArraySize is outside range [0, 2048], or #varjo_SwapChainConfig2::numberOfTextures < 2.
 *
 * @param session Varjo session handle.
 * @param dev Pointer to D3D11 device
 * @param config Pointer to #varjo_SwapChainConfig2 structure describing the swap chain configuration.
 *
 * @return Pointer to a newly created varjo_SwapChain object, or nullptr on error.
 */
VARJO_API struct varjo_SwapChain* varjo_D3D11CreateSwapChain(struct varjo_Session* session, struct ID3D11Device* dev, struct varjo_SwapChainConfig2* config);

/**
 * Create an OpenGL swap chain that can be used when submitting layers.
 * A swap chain object contains the image surfaces that can be rendered into, and submitted to the Varjo Compositor for displaying.
 * Each swap chain contains multiple images in order to perform double- or triple-buffering when compositing the image.
 * The amount of swap chain images created is configured in the numberOfTextures member of the #varjo_SwapChainConfig2 struct.
 *
 * Each frame submission contains references to 1..n swap chain objects; the application is free to render all views
 * into a single swap chain image side-by-side as an atlas, or use a separate swap chain objects for each view (or any other combination imaginable).
 *
 * This function expects an OpenGL render context to be active for the calling thread.
 *
 * Use #varjo_FreeSwapChain to free the created swap chain.
 *
 * Possible errors:
 *   - #varjo_Error_NullPointer: config pointer is a null pointer.
 *   - #varjo_Error_ValidationFailure: #varjo_SwapChainConfig2::textureWidth or #varjo_SwapChainConfig2::textureHeight is outside range [0, 16384],
 *     #varjo_SwapChainConfig2::textureArraySize is outside range [0, 2048], or #varjo_SwapChainConfig2::numberOfTextures < 2.
 *
 * @param session Varjo session handle.
 * @param config Pointer to #varjo_SwapChainConfig2 structure describing the swap chain configuration.
 *
 * @return Pointer to a newly created varjo_SwapChain object, or nullptr on error.
 */
VARJO_API struct varjo_SwapChain* varjo_GLCreateSwapChain(struct varjo_Session* session, struct varjo_SwapChainConfig2* config);

/**
 * Create a D3D12 swap chain that can be used when submitting layers.
 * A swap chain object contains the image surfaces that can be rendered into, and submitted to the Varjo Compositor for displaying.
 * Each swap chain contains multiple images in order to perform double- or triple-buffering when compositing the image.
 * The amount of swap chain images created is configured in the numberOfTextures member of the #varjo_SwapChainConfig2 struct.
 *
 * Each frame submission contains references to 1..n swap chain objects; the application is free to render all views
 * into a single swap chain image side-by-side as an atlas, or use a separate swap chain objects for each view (or any other combination imaginable).
 *
 * Use #varjo_FreeSwapChain to free the created swap chain.
 *
 * Possible errors:
 *   - #varjo_Error_NullPointer: config pointer is a null pointer.
 *   - #varjo_Error_ValidationFailure: #varjo_SwapChainConfig2::textureWidth or #varjo_SwapChainConfig2::textureHeight is outside range [0, 16384],
 *     #varjo_SwapChainConfig2::textureArraySize is outside range [0, 2048], or #varjo_SwapChainConfig2::numberOfTextures < 2.
 *   - #varjo_Error_D3D11OnD12DeviceCreationFailed: creation of the underlying ID3D11On12Device failed.
 *
 * @param session Varjo session handle.
 * @param commandQueue Pointer to D3D12 command queue with that can be used to submit graphics commands.
 * @param config Pointer to #varjo_SwapChainConfig2 structure describing the swap chain configuration.
 *
 * @return Pointer to a newly created varjo_SwapChain object, or nullptr on error.
 */
VARJO_API struct varjo_SwapChain* varjo_D3D12CreateSwapChain(
    struct varjo_Session* session, struct ID3D12CommandQueue* commandQueue, struct varjo_SwapChainConfig2* config);

/**
 * Create a Vulkan swap chain that can be used when submitting layers.
 * A swap chain object contains the image surfaces that can be rendered into, and submitted to the Varjo Compositor for displaying.
 * Each swap chain contains multiple images in order to perform double- or triple-buffering when compositing the image.
 * The amount of swap chain images created is configured in the numberOfTextures member of the #varjo_SwapChainConfig2 struct.
 *
 * Each frame submission contains references to 1..n swap chain objects; the application is free to render all views
 * into a single swap chain image side-by-side as an atlas, or use a separate swap chain objects for each view (or any other combination imaginable).
 *
 * Use #varjo_FreeSwapChain to free the created swap chain.
 *
 * Possible errors:
 *   - #varjo_Error_NullPointer: config pointer is a null pointer.
 *   - #varjo_Error_ValidationFailure: #varjo_SwapChainConfig2::textureWidth or #varjo_SwapChainConfig2::textureHeight is outside range [0, 16384],
 *     #varjo_SwapChainConfig2::textureArraySize is outside range [0, 2048], or #varjo_SwapChainConfig2::numberOfTextures < 2.
 *
 * @param session Varjo session handle.
 * @param device Vulkan device, associated with physical device returned from #varjo_GetPhysicalDeviceVk
 * @param queueFamilyIndex Index of a Vulkan queue family that supports graphics operations
 * @param queueIndex Index of a Vulkan queue within the family that will be used for image synchronization
 * @param config Pointer to #varjo_SwapChainConfig2 structure describing the swap chain configuration.
 *
 * @return Pointer to a newly created varjo_SwapChain object, or nullptr on error.
 */
VARJO_API struct varjo_SwapChain* varjo_VKCreateSwapChain(
    struct varjo_Session* session, struct VkDevice_T* device, uint32_t queueFamilyIndex, uint32_t queueIndex, struct varjo_SwapChainConfig2* config);

/**
 * Retrieve a swap chain image with the given index.
 * A swap chain contains 1..n images to allow double- or triple-buffering in the compositor. This function can be used
 * to retrieve the underlying texture object for each of those images.
 *
 * The texture objects retrieved with this function are guaranteed to stay valid for the lifetime of the varjo_SwapChain object,
 * so the application is free to query the textures at application startup time to build other dependent objects (such as Render Target Views etc).
 *
 * @param swapChain Pointer to the varjo_SwapChain object.
 * @param index The index of the image to retrieve. This value must be in range [0, number of textures in the swap chain[
 *
 * @return The swap chain image texture at the given index.
 */
VARJO_API struct varjo_Texture varjo_GetSwapChainImage(struct varjo_SwapChain* swapChain, int32_t index);

/**
 * Acquire (lock) the next image in the swap chain for rendering.
 * This function acquires the next image in the swap chain that is available for rendering, and must be called by the client
 * before any rendering is performed into the swap chain. The index of the acquired swap chain image will be written to
 * the location pointed to by the index parameter. The actual texture for the swap chain can be retrieved with a call to #varjo_GetSwapChainImage.
 *
 * After the application is done with rendering to the swap chain image, the image must be released back to the compositor
 * by calling #varjo_ReleaseSwapChainImage. This must be done for each swap chain object that is being submitted to the compositor
 * before calling #varjo_EndFrameWithLayers.
 *
 * @param swapChain Pointer to the varjo_SwapChain object to acquire the image from.
 * @param index Pointer to the location where the acquired swap chain image index will be written to.
 */
VARJO_API void varjo_AcquireSwapChainImage(struct varjo_SwapChain* swapChain, int32_t* index);

/**
 * Release a swap chain image previously acquired with a call to #varjo_AcquireSwapChainImage.
 *
 * @param swapChain Pointer to the varjo_SwapChain object where an image was previously acquired from.
 */
VARJO_API void varjo_ReleaseSwapChainImage(struct varjo_SwapChain* swapChain);

/**
 * Delete a swap chain object.
 *
 * @param swapChain The varjo_SwapChain object to be deleted.
 */
VARJO_API void varjo_FreeSwapChain(struct varjo_SwapChain* swapChain);

/**
 * Begin rendering the frame.
 * This function should be called only from the rendering thread and before any
 * rendering work for the given frame is done.
 * Frame must be submitted with varjo_EndFrameWithLayers().
 *
 * @param session Varjo session handle.
 */
VARJO_API void varjo_BeginFrameWithLayers(struct varjo_Session* session);

/**
 * Submit a set of swap chains to the compositor to be displayed.
 *
 * The submitInfo parameter contains the list of layers that are to be displayed. See #varjo_SubmitInfoLayers
 * documentation for more information.
 *
 * #varjo_SubmitInfoLayers structure will be validated. Possible errors raised from validation:
 *   - #varjo_Error_NullPointer: submitInfo or other pointers inside the structure are null. Note that in some cases it's valid
 *     to provide a null pointer: if #varjo_SubmitInfoLayers::layerCount is 0 and for #varjo_ViewExtension::next.
 *   - #varjo_Error_ValidationFailure: #varjo_SubmitInfoLayers::layerCount is less than 0.
 *   - #varjo_Error_InvalidMatrix: #varjo_LayerMultiProjView::projection or #varjo_LayerMultiProjView::view contains NaNs or zeros.
 *   - #varjo_Error_InvalidSwapChainRect: x, y, width, or height (or any combination of) in #varjo_SwapChainViewport is not in the swap chain range.
 *   - #varjo_Error_ViewDepthExtensionValidationFailure: #varjo_ViewExtensionDepth::minDepth is greater than #varjo_ViewExtensionDepth::maxDepth.
 *   - #varjo_Error_InvalidSwapChain: swapchain pointer does not represent swapchain or is null.
 *   - #varjo_Error_WrongSwapChainTextureFormat: swapchain with depth texture format was attached to #varjo_LayerMultiProjView, or RGB swapchain was
 *     attached to #varjo_ViewExtensionDepth.
 *   - #varjo_Error_InvalidViewExtension: view extension is specified, but does not contain one of the allowed types.
 *   - #varjo_Error_SwapChainTextureIsNotReleased: attached texture was not released with #varjo_ReleaseSwapChainImage.
 *
 * @param session Varjo session handle.
 * @param submitInfo Pointer to layer submit info structure.
 */
VARJO_API void varjo_EndFrameWithLayers(struct varjo_Session* session, struct varjo_SubmitInfoLayers* submitInfo);

/**
 * Configures where Compositor should render mirror views.
 * Multiple views are supported. varjo_MirrorView structures defines viewport
 * where view should be rendered, and and index of the view.
 * @param session Varjo session handle.
 * @param mirrorViews structs defining mirror views (viewport defines target viewports in a given swapchain, and
 *       viewIndex defines which view needs to be rendered (0 - left, 1 - right,) see varjo_GetViewCount() and
 *       varjo_GetViewDescription().
 * @param count
 */
VARJO_API void varjo_SetMirrorConfig(struct varjo_Session* session, struct varjo_MirrorView* mirrorViews, uint32_t count);

/**
 * Resets mirror config, effectively disabling mirror view feature.
 * @param session Varjo session handle
 */
VARJO_API void varjo_ResetMirrorConfig(struct varjo_Session* session);

#if defined __cplusplus
}
#endif

#endif
