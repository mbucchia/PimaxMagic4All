// Copyright 2019-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_MR_H
#define VARJO_MR_H

#include "Varjo_export.h"
#include "Varjo_types.h"
#include "Varjo_types_mr.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Try to lock the given lock type.
 *
 * Client must successfully acquire lock before any calls to features behind
 * the lock.
 *
 * #varjo_Unlock must be called after the feature is not needed anymore and
 * it is free to be used by other clients.
 *
 * Each lock type can be locked by one application at a time. Lock call
 * returns #varjo_True if the calling client holds the lock after the call. If
 * the lock was held by the calling client already, #varjo_Error_AlreadyLocked
 * error is set. Return value is #varjo_False if another client is holding the
 * lock.
 *
 * Note: Application needs to hold the lock as long it is going to use the feature.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_AlreadyLocked
 *
 * @param session Varjo session handle.
 * @param lockType Lock type to be locked.
 * @return varjo_True if the client holds the lock after the call, otherwise varjo_False.
 */
VARJO_API varjo_Bool varjo_Lock(struct varjo_Session* session, varjo_LockType lockType);

/**
 * Unlock a previously locked lock.
 *
 * If the lock was not held by the calling client, #varjo_Error_NotLocked
 * error is set. This is the case if the lock is not locked at all or another
 * client has the lock.
 *
 * Note: After unlocking the lock other applications are free to acquire the
 * lock and access locked features at any time.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_NotLocked
 *
 * @param session Varjo session handle.
 * @param lockType Lock type to be locked.
 */
VARJO_API void varjo_Unlock(struct varjo_Session* session, varjo_LockType lockType);

/**
 * Start video pass through rendering on the HMD displays.
 *
 * Video is rendered as a background layer. i.e. Any application rendering is done on top of the video,
 * unless occlusion is enabled (#varjo_MRSetVideoDepthEstimation).
 *
 * Possible errors: #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 * @param enabled On/off.
 */
VARJO_API void varjo_MRSetVideoRender(struct varjo_Session* session, varjo_Bool enabled);

/**
 * Start depth estimation for the video pass through image. This enables occlusion between VR and MR content
 * when the VR content is submitted as a layer with a depth buffer attachment (#varjo_ViewExtensionDepth) and
 * depth testing enabled (#varjo_LayerFlag_DepthTesting).
 *
 * The default depth test range is [0m, 0.75m]. This means that depth test against the video layer is performed
 * only at close distance (hands), and beyond that VR content is always visible. Applications can override this
 * behavior by providing explicit depth test range in #varjo_ViewExtensionDepthTestRange extension.
 * In that case, default is not used and depth test is calculated according to the application provided range.
 * This is useful for enabling the full depth range.
 *
 * This setting is ignored unless #varjo_MRSetVideoRender has been enabled.
 *
 * Possible errors: #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 * @param enabled On/off toggle.
 */
VARJO_API void varjo_MRSetVideoDepthEstimation(struct varjo_Session* session, varjo_Bool enabled);

/**
 * Change virtual camera rendering position between user's eyes and video pass through cameras.
 *
 * The scene is rendered twice from the position of the user's eyes. In full VR the eye position corresponds to the physical
 * position of the user's eyes. When using video pass through there is a physical offset between the user's eyes and the
 * stereo camera pair. So in contrast to full VR, when rendering in MR mode: To remove stereo disparity problems between the
 * real and virtual world and prevent 'floating' of the VR objects anchored to the real world, the scene should be rendered
 * from the physical position of the camera pair in most cases. This is the default mode and corresponds to setting eye offset
 * 'percentage' to 1.0.
 *
 * But there can be scenes that are predominantly VR where it makes sense to render the scene using the VR eye position.
 * e.g. The scene only contains a small 'magic window' to view the real world or the real world is only shown as a backdrop.
 *
 * This function can be used to switch the rendering position. It can be used for smooth interpolated change in case it
 * needs to be done the middle of the scene.
 *
 * Note: This setting is ignored if eye reprojection is enabled (#varjo_CameraPropertyType_EyeReprojection). In this case
 * the rendering is always done from the user's eye position (full VR position, corresponds to 'percentage' 0.0).
 *
 * Possible errors: #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 * @param percentage [0.0, 1.0] Linear interpolation of the rendering position between the position of HMD user's eyes and
 *        the video pass through camera position. Given value will be clamped to the accepted range.
 */
VARJO_API void varjo_MRSetVRViewOffset(struct varjo_Session* session, double percentage);

/**
 * Start chroma keying for the video pass through image. This enables occlusion between VR and MR content
 * when the VR content is submitted as a layer with chroma key testing enabled (#varjo_LayerFlag_ChromaKeyMasking).
 *
 * This setting is ignored unless #varjo_MRSetVideoRender has been enabled.
 *
 * Possible errors: #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 * @param enabled On/off toggle.
 */
VARJO_API void varjo_MRSetChromaKey(struct varjo_Session* session, varjo_Bool enabled);

/**
 * Enables video pass through and starts chroma keying for all application layers regardless if they have chroma key
 * flag set. Varjo system layers are not chroma keyed. This is used as force override to make it possible to run
 * non-MR applications with chroma keying. For application specific chroma keying, use #varjo_MRSetChromaKey.
 *
 * Possible errors: #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 * @param enabled Toggle global override on/off.
 */
VARJO_API void varjo_MRSetChromaKeyGlobal(struct varjo_Session* session, varjo_Bool enabled);

/**
 * Try to lock the chroma key configuration.
 *
 * This call needs to succeed before any calls altering the chroma key config state.
 *
 * #varjo_MRUnlockChromaKeyConfig should be called after the configuration has been changed.
 *
 * The configuration can be locked by one application at a time. Locking fails,
 * if another client has the configuration locked already.
 *
 * Note: If an application wants to prevent others from changing the chroma key settings,
 * the lock can be held for a long period of time.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_ChromaKeyAlreadyLocked
 *
 * @param session Varjo session handle.
 * @return varjo_True if locked successfully, otherwise varjo_False.
 * @deprecated #varjo_Lock provides generic lock functionality for mixed reality features.
 */
VARJO_DEPRECATED_API varjo_Bool varjo_MRLockChromaKeyConfig(struct varjo_Session* session);

/**
 * Unlock previously locked chroma key configuration.
 *
 * Note: After unlocking the configuration other applications are free to modify the settings.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_ChromaKeyNotLocked
 *
 * @param session Varjo session handle.
 * @deprecated #varjo_Unlock provides generic unlock functionality for mixed reality features.
 */
VARJO_DEPRECATED_API void varjo_MRUnlockChromaKeyConfig(struct varjo_Session* session);

/**
 * Get number of chroma key config indices supported. Maximum index will be count-1.
 *
 * Possible errors:
 *      #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 * @return Number of supported chroma key configs.
 */
VARJO_API int32_t varjo_MRGetChromaKeyConfigCount(struct varjo_Session* session);

/**
 * Set video chroma keying configuration to select color matcher and setting parameters for it. Index can be used to
 * set multiple chroma key configs that will all be evaluated into a single mask. Use #varjo_MRGetChromaKeyConfigCount
 * for getting maximum count. Config values will be clamped to valid range.
 *
 * Before calling this function, chroma key config has to be locked succesfully with #varjo_MRLockChromaKeyConfig.
 *
 * Possible errors:
 *      #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *      #varjo_Error_ChromaKeyNotLocked, #varjo_Error_ChromaKeyInvalidType, #varjo_Error_IndexOutOfBounds
 *
 * @param session Varjo session handle.
 * @param index Chroma key index.
 * @param config Pointer to chroma key configuration structure to be applied.
 */
VARJO_API void varjo_MRSetChromaKeyConfig(struct varjo_Session* session, int32_t index, const struct varjo_ChromaKeyConfig* config);

/**
 * Get currently applied chroma keying configuration for given index. Use #varjo_MRGetChromaKeyConfigCount for getting maximum count.
 *
 * Possible errors:
 *      #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *      #varjo_Error_IndexOutOfBounds
 *
 * @param session Varjo session handle.
 * @param index Chroma key index.
 * @return Currently applied chroma keying configuration.
 */
VARJO_API struct varjo_ChromaKeyConfig varjo_MRGetChromaKeyConfig(struct varjo_Session* session, int32_t index);

/**
 * Set environment cubemap configuration to select parameters for it.
 *
 * Before calling this function, environment cubemap config has to be locked successfully with #varjo_Lock.
 *
 * Possible errors:
 *      #varjo_Error_NullPointer, #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *      #varjo_Error_EnvironmentCubemapNotLocked, #varjo_Error_EnvironmentCubemapInvalidMode
 *
 * @param session Varjo session handle.
 * @param config Pointer to environment cubemap configuration to be applied.
 */
VARJO_API void varjo_MRSetEnvironmentCubemapConfig(struct varjo_Session* session, const struct varjo_EnvironmentCubemapConfig* config);

/**
 * Get current environment cubemap configuration.
 *
 * Possible errors:
 *      #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 * @return Current environment cubemap configuration.
 */
VARJO_API struct varjo_EnvironmentCubemapConfig varjo_MRGetEnvironmentCubemapConfig(struct varjo_Session* session);

/**
 * Try to lock the camera configuration.
 *
 * This call needs to succeed before calls to
 *  - #varjo_MRSetCameraPropertyMode
 *  - #varjo_MRSetCameraPropertyValue
 *  - #varjo_MRResetCameraProperty
 *  - #varjo_MRResetCameraProperties
 *
 * #varjo_MRUnlockCameraConfig should be called after the configuration has been changed.
 *
 * The configuration can be locked by one application at a time. Locking fails,
 * if another client has the configuration locked already.
 *
 * Note: If an application wants to prevent others from changing the camera settings,
 * the lock can be held for a long period of time.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraAlreadyLocked
 *
 * @param session Varjo session handle.
 * @return varjo_True if locked successfully, otherwise varjo_False.
 * @deprecated #varjo_Lock provides generic lock functionality for mixed reality features.
 */
VARJO_DEPRECATED_API varjo_Bool varjo_MRLockCameraConfig(struct varjo_Session* session);

/**
 * Unlock previously locked camera configuration.
 *
 * Note: After unlocking the configuration other applications are free to modify the settings.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraNotLocked
 *
 * @param session Varjo session handle.
 * @deprecated #varjo_Unlock provides generic unlock functionality for mixed reality features.
 */
VARJO_DEPRECATED_API void varjo_MRUnlockCameraConfig(struct varjo_Session* session);

/**
 * Get amount of camera property modes.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType
 *
 * @param session Varjo session handle.
 * @param prop Camera property type.
 */
VARJO_API int32_t varjo_MRGetCameraPropertyModeCount(struct varjo_Session* session, varjo_CameraPropertyType prop);

/**
 * Get available property modes.
 *
 * The needed size for the 'modes' output parameter can be obtained by calling #varjo_MRGetCameraPropertyModeCount.
 *
 * If manual property mode is supported, #varjo_MRSetCameraPropertyValue can be used to set the manual property value.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType
 *
 * @param session Varjo session handle.
 * @param prop Camera property type.
 * @param modes Fills this array with the possible property modes.
 * @param maxSize Maximum amount of modes that fit into 'modes'. Should be >= #varjo_MRGetCameraPropertyModeCount().
 */
VARJO_API void varjo_MRGetCameraPropertyModes(struct varjo_Session* session, varjo_CameraPropertyType prop, varjo_CameraPropertyMode* modes, int32_t maxSize);

/**
 * Returns currently set mode for given property type.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType
 *
 * @param session Varjo session handle.
 * @param type Camera property type.
 * @return Current mode.
 */
VARJO_API varjo_CameraPropertyMode varjo_MRGetCameraPropertyMode(struct varjo_Session* session, varjo_CameraPropertyType type);

/**
 * Sets property mode.
 *
 * Before calling this function, camera config has to be locked succesfully with #varjo_MRLockCameraConfig.
 *
 * While property mode is set to manual, the camera uses the latest manual property value set with #varjo_MRSetCameraPropertyValue.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType, #varjo_Error_CameraInvalidPropertyMode, #varjo_Error_CameraNotLocked
 *
 * @param session Varjo session handle.
 * @param type Camera property type.
 * @param mode Camera property mode to be set.
 */
VARJO_API void varjo_MRSetCameraPropertyMode(struct varjo_Session* session, varjo_CameraPropertyType type, varjo_CameraPropertyMode mode);

/**
 * Get manual property config type for given property.
 *
 * Config type can be:
 * - List: List of discrete values.
 * - Range: Min and max values.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType
 *
 * @param session Varjo session handle.
 * @param prop Camera property type.
 */
VARJO_API varjo_CameraPropertyConfigType varjo_MRGetCameraPropertyConfigType(struct varjo_Session* session, varjo_CameraPropertyType prop);

/**
 * Get amount of possible manual camera property values.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType
 *
 * @param session Varjo session handle.
 * @param prop Camera property type.
 */
VARJO_API int32_t varjo_MRGetCameraPropertyValueCount(struct varjo_Session* session, varjo_CameraPropertyType prop);

/**
 * Get possible manual property values.
 *
 * The needed size for the 'values' output parameter can be obtained by calling GetCameraPropertyValueCount.
 *
 * Values have different meaning depending on the property.
 * - ExposureTime: Floating point frames per second. e.g. 90.0 -> ~11ms exposure time.
 * - ISOValue: Integer ISO values. e.g. "200" -> ISO200.
 * - WhiteBalance: Integer color temperatures in Kelvin.
 * - FlickerCompensation: Integer in Hz. e.g "50" -> compensation for 50 Hz.
 *                        This is useful when using the HMD indoors with mostly artificial light bulbs,
 *                        which flicker at the frequency of 50Hz or 60Hz and can cause visual flicker
 *                        artifacts on the video pass through image. The correct setting depends on the
 *                        underlying power grid's frequency. e.g. In most parts of Africa/Asia/Australia/Europe
 *                        the frequency is 50 Hz and in most parts of North and South America 60 Hz.
 * - Sharpness: Integer denoting the power of the sharpness filter. The lowest value corresponds to small
 *              amount of filtering and the highest value corresponds to the largest amount of filtering.
 * - Eye reprojection: There is a physical offset between the user's eyes and the video pass through cameras,
 *                     which can cause the perception of the real world to feel different than without the HMD.
 *                     When the eye reprojection is turned on, the software reprojects the video pass through
 *                     image from the physical camera position into the physical position of the user's eyes.
 *                     While this will improve the perception it can also cause visual artifacts and therefore
 *                     can be unwanted behavior for some applications.
 *                     This property offers two different reprojection modes:
 *                       * When mode is AUTO, the view is automatically reprojected based on the depth map. This is the
 *                         most physically accurate reprojection.
 *                       * When mode is MANUAL, the view is reprojected into a certain static distance. Value is a float
 *                         specifying the reprojection distance in meters.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType
 *
 * @param session Varjo session handle.
 * @param prop Camera property type.
 * @param values Fills this array with the possible property values.
 *               It's either a list or a range depending on the property. This can be determined from GetCameraPropertyConfigType-call.
 * @param maxSize Maximum amount of values that fit into 'values'. Should be >= GetCameraPropertyValueCount().
 */
VARJO_API void varjo_MRGetCameraPropertyValues(
    struct varjo_Session* session, varjo_CameraPropertyType prop, struct varjo_CameraPropertyValue* values, int32_t maxSize);

/**
 * Returns the latest manual property value set with #varjo_MRSetCameraPropertyValue for the given property type.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType
 *
 * @param session Varjo session handle.
 * @param type Camera property type.
 * @return Current value.
 */
VARJO_API struct varjo_CameraPropertyValue varjo_MRGetCameraPropertyValue(struct varjo_Session* session, varjo_CameraPropertyType type);

/**
 * Sets manual property value.
 *
 * Before calling this function, camera config has to be locked succesfully with #varjo_MRLockCameraConfig.
 *
 * While property mode is set to manual, the camera uses the latest set property value.
 * The value can be set even if property mode is not set to manual, in which case the camera starts using it once mode is set to manual.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType, #varjo_Error_CameraInvalidPropertyValue, #varjo_Error_CameraNotLocked
 *
 * @param session Varjo session handle.
 * @param type Camera property type.
 * @param value Const pointer to camera property value to be set.
 */
VARJO_API void varjo_MRSetCameraPropertyValue(struct varjo_Session* session, varjo_CameraPropertyType type, const struct varjo_CameraPropertyValue* value);

/**
 * Resets the camera property mode and manual mode value to defaults.
 *
 * Before calling this function, camera config has to be locked succesfully with #varjo_MRLockCameraConfig.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType, #varjo_Error_CameraNotLocked
 *
 * @param session Varjo session handle.
 * @param type Camera property type.
 */
VARJO_API void varjo_MRResetCameraProperty(struct varjo_Session* session, varjo_CameraPropertyType type);

/**
 * Resets all camera properties to the default mode and value.
 *
 * Before calling this function, camera config has to be locked succesfully with #varjo_MRLockCameraConfig.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_CameraInvalidPropertyType, #varjo_Error_CameraNotLocked
 *
 * @param session Varjo session handle.
 */
VARJO_API void varjo_MRResetCameraProperties(struct varjo_Session* session);

#if defined __cplusplus
}
#endif
#endif  // VARJO_MR_H
