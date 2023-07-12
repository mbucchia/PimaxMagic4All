// Copyright 2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_VK_H
#define VARJO_VK_H

#include "Varjo_types_vk.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Get Vulkan instance extensions required for Varjo Runtime operation.
 *
 * To allocate memory for the \p extensionNames output buffer, you should first call this
 * function by passing null \p extensionNames and query the count for the extensions.
 *
 * @param session Varjo session handle.
 * @param count Output pointer for the extension count. Cannot be null.
 * @param extensionNames Output pointer for the extension names. Can be null.
 */
VARJO_API void varjo_GetInstanceExtensionsVk(struct varjo_Session* session, int32_t* count, const char** extensionNames);

/**
 * Get a Vulkan physical device suitable for sharing textures with Varjo compositor.
 *
 * @param session Varjo session handle.
 * @param instance Vulkan instance.
 *
 * @return A valid VkPhysicalDevice handle, or VK_NULL_HANDLE on error.
 */
VARJO_API VkPhysicalDevice varjo_GetPhysicalDeviceVk(struct varjo_Session* session, VkInstance instance);

/**
 * Get Vulkan device extensions required for Varjo Runtime operation.
 *
 * To allocate memory for the \p extensionNames output buffer, you should first call this
 * function by passing null \p extensionNames and query the count for the extensions.
 *
 * @param session Varjo session handle.
 * @param count Output pointer for the extension count. Cannot be null.
 * @param extensionNames Output pointer for the extension names. Can be null.
 */
VARJO_API void varjo_GetDeviceExtensionsVk(struct varjo_Session* session, int32_t* count, const char** extensionNames);

/**
 * Converts a Vulkan texture to #varjo_Texture.
 */
VARJO_API struct varjo_Texture varjo_FromVkTexture(VkImage texture);

/**
 * Converts #varjo_Texture to Vulkan texture.
 */
VARJO_API VkImage varjo_ToVkTexture(struct varjo_Texture texture);

#if defined __cplusplus
}
#endif

#endif  // VARJO_VK_H