// Copyright 2019-2023 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_TYPES_H
#define VARJO_TYPES_H

#include <stdint.h>
#include "Varjo_export.h"

#if defined __cplusplus
extern "C" {
#endif

typedef int64_t varjo_Error;
static const varjo_Error varjo_Error_InvalidSession = 1;
static const varjo_Error varjo_Error_OutOfMemory = 2;
static const varjo_Error varjo_Error_InvalidVersion = 3;
static const varjo_Error varjo_Error_GraphicsNotInitialized = 4;
static const varjo_Error varjo_Error_FrameNotStarted = 5;
static const varjo_Error varjo_Error_FrameAlreadyStarted = 6;
static const varjo_Error varjo_Error_ViewIndexOutOfBounds = 7;
static const varjo_Error varjo_Error_InvalidPoseType = 8;
static const varjo_Error varjo_Error_NullPointer = 9;
static const varjo_Error varjo_Error_MixingTextures = 10;
static const varjo_Error varjo_Error_NaN = 11;
static const varjo_Error varjo_Error_NoHMDConnected = 12;
static const varjo_Error varjo_Error_ValidationFailure = 13;
static const varjo_Error varjo_Error_IndexOutOfBounds = 14;
static const varjo_Error varjo_Error_AlreadyLocked = 15;
static const varjo_Error varjo_Error_NotLocked = 16;
static const varjo_Error varjo_Error_InvalidSize = 17;
static const varjo_Error varjo_Error_InvalidParameter = 104;
static const varjo_Error varjo_Error_UnsupportedParameter = 105;

// Gaze errors (100 - 199)
static const varjo_Error varjo_Error_GazeNotInitialized = 100;
static const varjo_Error varjo_Error_GazeNotConnected = 101;
static const varjo_Error varjo_Error_GazeAlreadyInitialized = 102;
static const varjo_Error varjo_Error_GazeNotAllowed = 103;
VARJORUNTIME_DEPRECATED static const varjo_Error varjo_Error_GazeInvalidParameter = 104;      // instead use varjo_Error_InvalidParameter
VARJORUNTIME_DEPRECATED static const varjo_Error varjo_Error_GazeUnsupportedParameter = 105;  // instead use varjo_Error_UnsupportedParameter

// Graphics errors (200 - 299)
static const varjo_Error varjo_Error_D3D11DeviceCreationFailed = 200;
static const varjo_Error varjo_Error_GetD3D11_1DeviceFailed = 201;
static const varjo_Error varjo_Error_AcquireD3D11DeviceHandleFailed = 202;
static const varjo_Error varjo_Error_GLBackBufferAlreadyCreated = 203;
static const varjo_Error varjo_Error_GLExtensionNotFound = 204;
static const varjo_Error varjo_Error_GLCreateTextureFailed = 205;
static const varjo_Error varjo_Error_GLAcquireD3D11TextureHandleFailed = 206;
static const varjo_Error varjo_Error_GLLockTextureFailed = 207;
static const varjo_Error varjo_Error_GLCopyTextureFailed = 208;
static const varjo_Error varjo_Error_OpenSharedTextureFailed = 209;
static const varjo_Error varjo_Error_TextureMutexQueryFailed = 210;
static const varjo_Error varjo_Error_TextureMutexAcquireFailed = 211;
static const varjo_Error varjo_Error_TextureMutexReleaseFailed = 212;
static const varjo_Error varjo_Error_D3D11AlreadyInitialized = 213;
static const varjo_Error varjo_Error_D3D11ShutDownFailed = 214;
static const varjo_Error varjo_Error_GLAlreadyInitialized = 215;
static const varjo_Error varjo_Error_GLShutDownFailed = 216;
static const varjo_Error varjo_Error_UnsupportedTextureFormat = 217;
static const varjo_Error varjo_Error_SwapChainConfigInvalidWidth = 219;
static const varjo_Error varjo_Error_SwapChainConfigInvalidHeight = 220;
static const varjo_Error varjo_Error_SwapChainConfigInvalidTextureCount = 221;
static const varjo_Error varjo_Error_GraphicsShutDownFailed = 222;
static const varjo_Error varjo_Error_TextureIsNull = 223;
static const varjo_Error varjo_Error_InvalidFrameNumber = 224;
static const varjo_Error varjo_Error_InvalidRenderAPI = 225;
static const varjo_Error varjo_Error_InvalidMatrix = 226;
static const varjo_Error varjo_Error_UnsupportedDepthFormat = 227;
static const varjo_Error varjo_Error_InvalidClipDistances = 229;
static const varjo_Error varjo_Error_SwapChainInvalidTextureIndex = 230;
static const varjo_Error varjo_Error_D3D11CreateTextureFailed = 231;
static const varjo_Error varjo_Error_InvalidSwapChainRect = 232;
static const varjo_Error varjo_Error_ViewDepthExtensionValidationFailure = 233;
static const varjo_Error varjo_Error_D3D11OnD12DeviceCreationFailed = 234;
static const varjo_Error varjo_Error_D3D12CreateTextureFailed = 235;
static const varjo_Error varjo_Error_InvalidSwapChain = 236;
static const varjo_Error varjo_Error_WrongSwapChainTextureFormat = 237;
static const varjo_Error varjo_Error_InvalidViewExtension = 238;
static const varjo_Error varjo_Error_InvalidViewport = 239;
static const varjo_Error varjo_Error_GraphicsError = 240;
static const varjo_Error varjo_Error_InvalidTextureSizeType = 241;
static const varjo_Error varjo_Error_SwapChainTextureIsNotReleased = 242;
static const varjo_Error varjo_Error_GazeInvalid = 243;

// Compositor errors (300 - 399)
static const varjo_Error varjo_Error_ConnectToProgramManagerFailed = 300;
static const varjo_Error varjo_Error_RequestSwapChainFormatFailed = 301;
static const varjo_Error varjo_Error_OpenSharedEventFailed = 302;
static const varjo_Error varjo_Error_CreateIPCFailed = 303;
static const varjo_Error varjo_Error_InitializeCompositorLinkFailed = 304;
static const varjo_Error varjo_Error_RequestViewInfoFailed = 305;
static const varjo_Error varjo_Error_NoCompositorLink = 306;
static const varjo_Error varjo_Error_RequestCreateSwapChainFailed = 307;
static const varjo_Error varjo_Error_RequestMirrorConfigFailed = 308;

// Mixed reality errors (700 - 799)
static const varjo_Error varjo_Error_RequestFailed = 700;
static const varjo_Error varjo_Error_OperationFailed = 701;
static const varjo_Error varjo_Error_NotAvailable = 702;
static const varjo_Error varjo_Error_CapabilityNotAvailable = 703;
static const varjo_Error varjo_Error_CameraAlreadyLocked = 704;
static const varjo_Error varjo_Error_CameraNotLocked = 705;
static const varjo_Error varjo_Error_CameraInvalidPropertyType = 706;
static const varjo_Error varjo_Error_CameraInvalidPropertyValue = 707;
static const varjo_Error varjo_Error_CameraInvalidPropertyMode = 708;
static const varjo_Error varjo_Error_ChromaKeyAlreadyLocked = 709;
static const varjo_Error varjo_Error_ChromaKeyNotLocked = 710;
static const varjo_Error varjo_Error_ChromaKeyInvalidType = 711;
static const varjo_Error varjo_Error_ChromaKeyEstimatorAlreadyRunning = 712;
static const varjo_Error varjo_Error_ChromaKeyEstimatorNotRunning = 713;
static const varjo_Error varjo_Error_EnvironmentCubemapNotLocked = 714;
static const varjo_Error varjo_Error_EnvironmentCubemapInvalidMode = 715;

// Data stream errors (800 - 899)
static const varjo_Error varjo_Error_DataStreamInvalidCallback = 800;
static const varjo_Error varjo_Error_DataStreamInvalidId = 801;
static const varjo_Error varjo_Error_DataStreamAlreadyInUse = 802;
static const varjo_Error varjo_Error_DataStreamNotInUse = 803;
static const varjo_Error varjo_Error_DataStreamBufferInvalidId = 804;
static const varjo_Error varjo_Error_DataStreamBufferAlreadyLocked = 805;
static const varjo_Error varjo_Error_DataStreamBufferNotLocked = 806;
static const varjo_Error varjo_Error_DataStreamFrameExpired = 807;
static const varjo_Error varjo_Error_DataStreamDataNotAvailable = 808;

// World errors (900 - 999)
static const varjo_Error varjo_Error_WorldObjectMarkersNotInitialized = 900;
static const varjo_Error varjo_Error_WorldComponentDoesNotExist = 901;
static const varjo_Error varjo_Error_WorldMarkerExpirationDurationIsNegative = 902;

// Foveation and variable rate shading errors (1000 - 1099)
static const varjo_Error varjo_Error_InvalidFoveationMode = 1000;
static const varjo_Error varjo_Error_UnableToInitializeVariableRateShading = 1001;
static const varjo_Error varjo_Error_InvalidVariableRateShadingRadius = 1002;

// Special error codes
static const varjo_Error varjo_NoError = 0;
static const varjo_Error varjo_Error_Unknown = -1;

typedef int32_t varjo_Bool;
static const varjo_Bool varjo_False = 0;
static const varjo_Bool varjo_True = 1;

/**
 * Time in nanoseconds. Absolute times are relative to an epoch which is constant during execution of the program.
 */
typedef int64_t varjo_Nanoseconds;

/**
 * Gaze statuses.
 */
typedef int64_t varjo_GazeStatus;
static const varjo_GazeStatus varjo_GazeStatus_Invalid = 0;  //!< Data is not available, user is not wearing the device or eyes can not be found.
static const varjo_GazeStatus varjo_GazeStatus_Adjust = 1;   //!< User is wearing the device but gaze tracking is in middle of adjustment.
static const varjo_GazeStatus varjo_GazeStatus_Valid = 2;    //!< Data is valid.

typedef int64_t varjo_GazeEyeStatus;
static const varjo_GazeEyeStatus varjo_GazeEyeStatus_Invalid = 0;      //!< Eye is not tracked. (e.g. not visible or is shut).
static const varjo_GazeEyeStatus varjo_GazeEyeStatus_Visible = 1;      //!< Eye is visible but not reliably tracked (e.g. saccade or blink).
static const varjo_GazeEyeStatus varjo_GazeEyeStatus_Compensated = 2;  //!< Eye is tracked but quality compromised (e.g. headset has moved after calibration).
static const varjo_GazeEyeStatus varjo_GazeEyeStatus_Tracked = 3;      //!< Eye is tracked.

typedef int32_t varjo_GazeEyeCalibrationQuality;
static const varjo_GazeEyeCalibrationQuality varjo_GazeEyeCalibrationQuality_Invalid = 0;  //!< Gaze calibration was not performed or failed.
static const varjo_GazeEyeCalibrationQuality varjo_GazeEyeCalibrationQuality_Low = 1;      //!< Quality of performed gaze calibration assessed as low.
static const varjo_GazeEyeCalibrationQuality varjo_GazeEyeCalibrationQuality_Medium = 2;   //!< Quality of performed gaze calibration assessed as medium.
static const varjo_GazeEyeCalibrationQuality varjo_GazeEyeCalibrationQuality_High = 3;     //!< Quality of performed gaze calibration assessed as high.

/**
 * Rotation reset types.
 */
typedef int64_t varjo_RotationReset;
static const varjo_RotationReset varjo_RotationReset_None = 0;  //!< Rotation is not reset.
static const varjo_RotationReset varjo_RotationReset_Yaw = 1;   //!< Yaw rotation (around up Y axis) is reset.
static const varjo_RotationReset varjo_RotationReset_All = 7;   //!< All rotation axes are reset.

/**
 * System status properties.
 */
typedef int64_t varjo_PropertyKey;
static const varjo_PropertyKey varjo_PropertyKey_Invalid = 0x0;                    //!< Invalid property key.
static const varjo_PropertyKey varjo_PropertyKey_UserPresence = 0x2000;            //!< boolean. Is user wearing the HMD.
static const varjo_PropertyKey varjo_PropertyKey_GazeCalibrating = 0xA000;         //!< boolean. Is system currently calibrating the HMD gaze tracker.
static const varjo_PropertyKey varjo_PropertyKey_GazeCalibrated = 0xA001;          //!< boolean. Is the HMD gaze tracker calibrated.
static const varjo_PropertyKey varjo_PropertyKey_GazeCalibrationQuality = 0xA002;  //!< float [0.0-1.0]. Quality of the gaze calibration.
static const varjo_PropertyKey varjo_PropertyKey_GazeAllowed = 0xA003;             //!< boolean. Is the HMD gaze tracker allowed.
static const varjo_PropertyKey varjo_PropertyKey_GazeEyeCalibrationQuality_Left =
    0xA004;  //!< float [0.0-1.0]. Quality assessment of the left eye gaze calibration.
static const varjo_PropertyKey varjo_PropertyKey_GazeEyeCalibrationQuality_Right =
    0xA005;                                                                   //!< float [0.0-1.0]. Quality assessment of the right eye gaze calibration.
static const varjo_PropertyKey varjo_PropertyKey_GazeIPDEstimate = 0xA010;    //!< float. User interpupillary distance estimate in millimeters.
static const varjo_PropertyKey varjo_PropertyKey_HMDConnected = 0xE001;       //!< boolean. Is HMD connected.
static const varjo_PropertyKey varjo_PropertyKey_HMDProductName = 0xE002;     //!< string. Product name.
static const varjo_PropertyKey varjo_PropertyKey_HMDSerialNumber = 0xE003;    //!< string. Product serial number.
static const varjo_PropertyKey varjo_PropertyKey_IPDPosition = 0xE010;        //!< float. HMD interpupillary distance position in millimeters.
static const varjo_PropertyKey varjo_PropertyKey_IPDAdjustmentMode = 0xE011;  //!< string. Current interpupillary distance adjustment mode.
static const varjo_PropertyKey varjo_PropertyKey_MRAvailable = 0xD000;        //!< boolean. Is Mixed Reality capable hardware present.

typedef int64_t varjo_PoseType;
static const varjo_PoseType varjo_PoseType_LeftEye = 0x1;   //!< Pose for the left eye.
static const varjo_PoseType varjo_PoseType_Center = 0x2;    //!< Pose for the head (in the middle of the eyes).
static const varjo_PoseType varjo_PoseType_RightEye = 0x3;  //!< Pose for the right eye.

typedef int64_t varjo_DisplayType;
static const varjo_DisplayType varjo_DisplayType_Focus = 0x1;    //!< Focus display.
static const varjo_DisplayType varjo_DisplayType_Context = 0x2;  //!< Context display.

typedef int64_t varjo_Eye;
static const varjo_Eye varjo_Eye_Left = 0x1;   //!< Left eye.
static const varjo_Eye varjo_Eye_Right = 0x2;  //!< Right eye.

typedef int64_t varjo_WindingOrder;
static const varjo_WindingOrder varjo_WindingOrder_Clockwise = 0x1;         //!< Clockwise triangle winding.
static const varjo_WindingOrder varjo_WindingOrder_CounterClockwise = 0x2;  //!< Counter-clockwise triangle winding.

typedef int64_t varjo_TextureFormat;
static const varjo_TextureFormat varjo_TextureFormat_INVALID = 0;                 //!< Invalid format
static const varjo_TextureFormat varjo_TextureFormat_R8G8B8A8_SRGB = 1;           //!< sRgb 8-bit RGBA format
static const varjo_TextureFormat varjo_TextureFormat_B8G8R8A8_SRGB = 2;           //!< sRgb 8-bit BGRA format
static const varjo_TextureFormat varjo_DepthTextureFormat_D32_FLOAT = 3;          //!< 32-bit floating point depth format
static const varjo_TextureFormat varjo_MaskTextureFormat_A8_UNORM = 4;            //!< 8-bit alpha mask
static const varjo_TextureFormat varjo_TextureFormat_YUV422 = 5;                  //!< YUV 4:2:2 semi-planar, 8bit Y plane, 8+8bit interleaved UV plane
static const varjo_TextureFormat varjo_TextureFormat_RGBA16_FLOAT = 6;            //!< RGBA 16 bit floating point
static const varjo_TextureFormat varjo_DepthTextureFormat_D24_UNORM_S8_UINT = 7;  //!< 24-bit UNORM depth and 8-bit UINT stencil format
static const varjo_TextureFormat varjo_DepthTextureFormat_D32_FLOAT_S8_UINT = 8;  //!< 32-bit FLOAT depth abd 8-bit UINT stencil format
static const varjo_TextureFormat varjo_TextureFormat_R8G8B8A8_UNORM = 9;          //!< Rgb 8-bit RGBA format
static const varjo_TextureFormat varjo_TextureFormat_R32_FLOAT = 10;              //!< Single channel 32-bit float format
static const varjo_TextureFormat varjo_TextureFormat_R32_UINT = 11;               //!< Single channel 32-bit uint format
static const varjo_TextureFormat varjo_VelocityTextureFormat_R8G8B8A8_UINT = 12;  //!< Two 16-bit integers packed into 4 bytes
static const varjo_TextureFormat varjo_TextureFormat_NV12 =
    13;  //!< YUV 4:2:0 semi-planar in NV12 format (8bit Y plane, 8+8bit interleaved UV plane in half resolution)
static const varjo_TextureFormat varjo_TextureFormat_B8G8R8A8_UNORM = 14;  //!< Rgb 8-bit BGRA format
static const varjo_TextureFormat varjo_TextureFormat_Y8_UNORM = 15;        //!< 8-bit Y plane only

/**
 * Render API flags
 */
typedef int64_t varjo_RenderAPI;
static const varjo_RenderAPI varjo_RenderAPI_D3D11 = 0x1;   //!< Direct3D11 rendering API
static const varjo_RenderAPI varjo_RenderAPI_GL = 0x2;      //!< OpenGL rendering API
static const varjo_RenderAPI varjo_RenderAPI_D3D12 = 0x3;   //!< Direct3D12 rendering API
static const varjo_RenderAPI varjo_RenderAPI_Vulkan = 0x4;  //!< Vulkan rendering API

typedef int64_t varjo_EulerOrder;
static const varjo_EulerOrder varjo_EulerOrder_XYZ = 0;
static const varjo_EulerOrder varjo_EulerOrder_ZYX = 1;
static const varjo_EulerOrder varjo_EulerOrder_XZY = 2;
static const varjo_EulerOrder varjo_EulerOrder_YZX = 3;
static const varjo_EulerOrder varjo_EulerOrder_YXZ = 4;
static const varjo_EulerOrder varjo_EulerOrder_ZXY = 5;

typedef int64_t varjo_RotationDirection;
static const varjo_RotationDirection varjo_RotationDirection_Clockwise = -1;
static const varjo_RotationDirection varjo_RotationDirection_CounterClockwise = 1;

typedef int64_t varjo_Handedness;
static const varjo_Handedness varjo_Handedness_RightHanded = 1;
static const varjo_Handedness varjo_Handedness_LeftHanded = -1;

typedef int64_t varjo_ClipRange;
static const varjo_ClipRange varjo_ClipRangeZeroToOne = 0;
static const varjo_ClipRange varjo_ClipRangeMinusOneToOne = 1;

/**
 * Submit flags.
 */
typedef int64_t varjo_SubmitFlags;

/**
 * Interpupillary distance (IPD) parameters
 */
static const char* varjo_IPDParametersKey_AdjustmentMode = "AdjustmentMode";        //!< Interpupillary distance adjustment mode.
static const char* varjo_IPDParametersValue_AdjustmentModeManual = "Manual";        //!< Interpupillary distance is adjusted manually.
static const char* varjo_IPDParametersValue_AdjustmentModeAutomatic = "Automatic";  //!< Interpupillary distance is adjusted automatically.

static const char* varjo_IPDParametersKey_RequestedPositionInMM =
    "RequestedPositionInMM";  //!< Requested interpupillary distance position in millimeters in manual adjustment mode. E.g. if pointer to "64.5"
                              //!< string is passed, manual IPD position will be adjusted to 64.5 mm. Passed string must be NUL-terminated, and must
                              //!< represent non-negative finite floating point value, optionally with decimal point for sub-millimeter accuracy
                              //!< (but not decimal comma). Values out of supported IPD range by current HMD will be clamped to the supported range.
                              //!< Applications could pass true measured user IPD value for getting optimal manual IPD adjustment in HMD.

/**
 * Gaze parameters
 */
static const char* varjo_GazeParametersKey_OutputFilterType = "OutputFilterType";  //!< Gaze output filter type.
static const char* varjo_GazeParametersValue_OutputFilterStandard = "Standard";    //!< Standard smoothing output filter.
static const char* varjo_GazeParametersValue_OutputFilterNone = "None";            //!< Output filter disabled.

static const char* varjo_GazeParametersKey_OutputFrequency = "OutputFrequency";              //!< Gaze output update frequency.
static const char* varjo_GazeParametersValue_OutputFrequency100Hz = "OutputFrequency100Hz";  //!< 100Hz output frequency. Default mode.
static const char* varjo_GazeParametersValue_OutputFrequency200Hz = "OutputFrequency200Hz";  //!< 200Hz output frequency. May be not supported
                                                                                             //!< on some Varjo devices. In that case
                                                                                             //!< #varjo_Error_GazeUnsupportedParameter will be set.
static const char* varjo_GazeParametersValue_OutputFrequencyMaximumSupported =
    "OutputFrequencyMaximumSupported";  //!< Maximum output frequency supported by connected HMD. Currently it is either 100hz or 200Hz.

/**
 * Gaze calibration parameters
 */
static const char* varjo_GazeCalibrationParametersKey_CalibrationType = "GazeCalibrationType";  //!< Gaze calibration type.
static const char* varjo_GazeCalibrationParametersValue_CalibrationFast = "Fast";               //!< Fast (5-dot) gaze calibration.
static const char* varjo_GazeCalibrationParametersValue_CalibrationOneDot = "OneDot";           //!< One dot gaze calibration

VARJORUNTIME_DEPRECATED static const char* varjo_GazeCalibrationParametersValue_CalibrationLegacy =
    "Legacy";  //!< Deprecated. Don't use in new code. This will just trigger a fallback to 'Fast' calibration.

static const char* varjo_GazeCalibrationParametersKey_HeadsetAlignmentGuidanceMode =
    "HeadsetAlignmentGuidanceMode";  //!< Controls behavior of headset alignment guidance user interface during calibration process.
static const char* varjo_GazeCalibrationParametersValue_WaitForUserInputToContinue =
    "WaitForUserInputToContinue";  //!< UI should wait for user input to continue even after headset alignment has been detected as acceptable. This is default
                                   //!< mode.
static const char* varjo_GazeCalibrationParametersValue_AutoContinueOnAcceptableHeadsetPosition =
    "AutoContinueOnAcceptableHeadsetPosition";  //!< UI should continue automatically to actual calibration after headset alignment has been accepted.

VARJORUNTIME_DEPRECATED static const char* varjo_GazeCalibrationParametersKey_OutputFilterType = "OutputFilterType";
VARJORUNTIME_DEPRECATED static const char* varjo_GazeCalibrationParametersValue_OutputFilterStandard = "Standard";
VARJORUNTIME_DEPRECATED static const char* varjo_GazeCalibrationParametersValue_OutputFilterNone = "None";

struct varjo_Session;

/**
 * Double precision 4x4 matrix.
 *
 * The matrix usage convention is that they are stored in column-major order and transforms are stacked before
 * column-vector points when multiplying. That is, a pure translation matrix will have the position offset in elements 12..14.
 *
 * Unless otherwise specified, the coordinate system is right-handed: X goes right, Y goes up and negative Z goes forward.
 *
 */
struct varjo_Matrix {
    double value[16];
};

/**
 * Double precision 3x3 matrix.
 * The matrix usage convention is that values are stored in column-major order.
 */
struct varjo_Matrix3x3 {
    double value[9];
};

/**
 * Ray is a vector starting from an origin.
 */
struct varjo_Ray {
    double origin[3];   //!< Origin of the ray.
    double forward[3];  //!< Direction of the ray.
};

/**
 * 32bit floating point 2D vector.
 */
struct varjo_Vector2Df {
    float x;  //!< X coordinate.
    float y;  //!< Y coordinate.
};

/*
 * 64bit floating point 3D vector.
 */
struct varjo_Vector3D {
    double x;  //!< X coordinate.
    double y;  //!< Y coordinate.
    double z;  //!< Z coordinate.
};

/*
 * 32bit floating point 3D vector.
 */
struct varjo_Vector3Df {
    float x;  //!< X coordinate.
    float y;  //!< Y coordinate.
    float z;  //!< Z coordinate.
};

/*
 * 32bit integer point 3D vector.
 */
struct varjo_Vector3Di {
    int32_t x;  //!< X coordinate.
    int32_t y;  //!< Y coordinate.
    int32_t z;  //!< Z coordinate.
};

/**
 * 64bit floating point size of a 3D object.
 */
struct varjo_Size3D {
    double width;   //!< Width along X-axis.
    double height;  //!< Height along Y-axis.
    double depth;   //!< Depth along Z-axis.
};

/**
 * 2D triangle list mesh.
 */
struct varjo_Mesh2Df {
    struct varjo_Vector2Df* vertices;  //!< Vertex data.
    int32_t vertexCount;               //!< Number of vertices.
};


/**
 * Axis aligned tangents from a projection matrix.
 * The tangents are between the edge planes and projection centre and defined so that if
 * the projection is centered, all of them are positive.
 */
struct varjo_AlignedView {
    double projectionTop;     //!< Tangent of the top edge angle.
    double projectionBottom;  //!< Tangent of the bottom edge angle.
    double projectionLeft;    //!< Tangent of the left edge angle.
    double projectionRight;   //!< Tangent of the right edge angle.
};

/**
 * View information for a frame.
 */
struct varjo_ViewInfo {
    double projectionMatrix[16];  //!< The projection matrix. See #varjo_Matrix for conventions.
    double viewMatrix[16];        //!< The view matrix, world-to-eye. See #varjo_Matrix for conventions.
    int32_t preferredWidth;       //!< Preferred width of the viewport.
    int32_t preferredHeight;      //!< Preferred height of the viewport.
    varjo_Bool enabled;           //!< Whether this view should be rendered during current frame.
    int32_t reserved;             //!< Unused.
};

/**
 * Per-frame information.
 */
struct varjo_FrameInfo {
    struct varjo_ViewInfo* views;   //!< Views for the frame. Array size is #varjo_GetViewCount.
    varjo_Nanoseconds displayTime;  //!< When the frame is estimated to be displayed.
    int64_t frameNumber;            //!< Current frame number.
};

/**
 * View description.
 * Information about the associated eye, display and resolution.
 */
struct varjo_ViewDescription {
    int32_t width;              //!< Default view width in pixels.
    int32_t height;             //!< Default view height in pixels.
    varjo_DisplayType display;  //!< Which display the view is for.
    varjo_Eye eye;              //!< Which eye the view is for.
};

/**
 * Gaze tracker vectors and tracking state.
 */
struct varjo_Gaze {
    struct varjo_Ray leftEye;                       //!< Left eye gaze ray.
    struct varjo_Ray rightEye;                      //!< Right eye gaze ray.
    struct varjo_Ray gaze;                          //!< Normalized gaze direction ray.
    double focusDistance;                           //!< Estimated gaze direction focus point distance.
    double stability;                               //!< Focus point stability.
    varjo_Nanoseconds captureTime;                  //!< Varjo time when this data was captured, see varjo_GetCurrentTime()
    varjo_GazeEyeStatus leftStatus;                 //!< Status of left eye data.
    varjo_GazeEyeStatus rightStatus;                //!< Status of right eye data.
    varjo_GazeStatus status;                        //!< Tracking main status.
    int64_t frameNumber;                            //!< Frame number, increases monotonically.
    VARJORUNTIME_DEPRECATED double leftPupilSize;   //!< Normalized [0..1] left eye pupil size.
    VARJORUNTIME_DEPRECATED double rightPupilSize;  //!< Normalized [0..1] right eye pupil size.
};

/**
 * Gaze tracker estimates of user's eye measurements.
 */
struct varjo_EyeMeasurements {
    int64_t frameNumber;                //!< Frame number, increases monotonically.
    varjo_Nanoseconds captureTime;      //!< Varjo time when this data was captured, see varjo_GetCurrentTime()
    float interPupillaryDistanceInMM;   //!< Estimate of user's IPD in mm.
    float leftPupilIrisDiameterRatio;   //!< Ratio of left pupil to iris diameter in mm. In range [0..1].
    float rightPupilIrisDiameterRatio;  //!< Ratio of right pupil to iris diameter in mm. In range [0..1].
    float leftPupilDiameterInMM;        //!< Estimate of left eye pupil diameter in mm.
    float rightPupilDiameterInMM;       //!< Estimate of right eye pupil diameter in mm.
    float leftIrisDiameterInMM;         //!< Estimate of left eye iris diameter in mm.
    float rightIrisDiameterInMM;        //!< Estimate of right eye iris diameter in mm.
    float leftEyeOpenness;              //!< Estimate of the openness ratio of the left eye; 1 corresponds to a fully open eye and 0 to a fully closed eye.
    float rightEyeOpenness;             //!< Estimate of the openness ratio of the right eye; 1 corresponds to a fully open eye and 0 to a fully closed eye.
};

/**
 * Minimum and maximum limits for swap chain texture count and size.
 */
struct varjo_SwapChainLimits {
    int32_t minimumNumberOfTextures;  //!< Minimum number of swap chain textures.
    int32_t maximumNumberOfTextures;  //!< Maximum number of swap chain textures.
    int32_t minimumTextureWidth;      //!< Minimum width of the swap chain textures.
    int32_t minimumTextureHeight;     //!< Minimum height of the swap chain textures.
    int32_t maximumTextureWidth;      //!< Maximum width of the swap chain textures.
    int32_t maximumTextureHeight;     //!< Maximum height of the swap chain textures.
};

/**
 * Viewport rectangle that defines viewport area.
 */
struct varjo_Viewport {
    int32_t x;       //!< X coordinate for the view in pixels.
    int32_t y;       //!< Y coordinate for the view in pixels.
    int32_t width;   //!< Width of the view in pixels.
    int32_t height;  //!< Height of the view in pixels.
};

/**
 * API-agnostic texture handle.
 * Use #varjo_FromD3D11Texture, varjo_ToD3D11Texture,
 * #varjo_FromD3D12Texture, #varjo_ToD3D12Texture,
 * #varjo_FromGLTexture, #varjo_ToGLTexture,
 * #varjo_FromVkTexture and #varjo_ToVkTexture
 * to convert textures between Varjo and graphics APIs.
 */
struct varjo_Texture {
    int64_t reserved[4];
};

/**
 * Parameters passed to #varjo_SetInterPupillaryDistanceParameters function.
 */
struct varjo_InterPupillaryDistanceParameters {
    const char* key;
    const char* value;
};

/**
 * Parameters passed to #varjo_GazeInitWithParameters function.
 */
struct varjo_GazeParameters {
    const char* key;
    const char* value;
};

/**
 * Parameters passed to #varjo_RequestGazeCalibrationWithParameters function.
 */
struct varjo_GazeCalibrationParameters {
    const char* key;
    const char* value;
};

typedef int64_t varjo_TextureSize_Type;
VARJORUNTIME_DEPRECATED const varjo_TextureSize_Type varjo_TextureSize_Type_BestQuality = 0;  //!< Maximum resolution which provides best possible quality
VARJORUNTIME_DEPRECATED const varjo_TextureSize_Type varjo_TextureSize_Type_Recommended =
    1;  //!< Recommended resolution, may not be as large as best quality but still provides great visual results
const varjo_TextureSize_Type varjo_TextureSize_Type_Quad = 1;              //!< Resolution for static quad rendering mode
const varjo_TextureSize_Type varjo_TextureSize_Type_DynamicFoveation = 2;  //!< Best resolution for foveation mode
const varjo_TextureSize_Type varjo_TextureSize_Type_Stereo = 3;            //!< Resolution for stereo mode

typedef int64_t varjo_TextureSize_Type_Mask;
VARJORUNTIME_DEPRECATED const varjo_TextureSize_Type_Mask varjo_TextureSize_Type_BestQuality_Mask = 0x1;  //!< Bitmask for BestQuality texture size type
VARJORUNTIME_DEPRECATED const varjo_TextureSize_Type_Mask varjo_TextureSize_Type_Recommended_Mask = 0x2;  //!< Bitmask for Recommended texture size type
const varjo_TextureSize_Type_Mask varjo_TextureSize_Type_Quad_Mask = 0x2;                                 //!< Bitmask for Quad texture size type
const varjo_TextureSize_Type_Mask varjo_TextureSize_Type_DynamicFoveation_Mask = 0x4;                     //!< Bitmask for DynamicFoveation texture size type
const varjo_TextureSize_Type_Mask varjo_TextureSize_Type_Stereo_Mask = 0x8;                               //!< Bitmask for Stereo texture size type

struct varjo_FovTangents {
    double top;     // Signed tangent of the top FOV half angle
    double bottom;  // Signed tangent of the bottom FOV half angle
    double left;    // Signed tangent of the left FOV half angle
    double right;   // Signed tangent of the right FOV half angle
};

struct varjo_FoveatedFovTangents_Hints {
    int64_t reserved[128];
};

typedef int64_t varjo_VariableRateShadingFlags;
const varjo_VariableRateShadingFlags varjo_VariableRateShadingFlag_None = 0;
const varjo_VariableRateShadingFlags varjo_VariableRateShadingFlag_Stereo = 1;  //!< Generates VRS map for stereo mode (2 views).
const varjo_VariableRateShadingFlags varjo_VariableRateShadingFlag_Gaze = 2;    //!< Generates VRS map taking gaze into account.
const varjo_VariableRateShadingFlags varjo_VariableRateShadingFlag_GazeCombined =
    4;  //!< Generates VRS map taking gaze into account with left and right gaze on one viewport.
const varjo_VariableRateShadingFlags varjo_VariableRateShadingFlag_OcclusionMap =
    8;  //!< Generates VRS with coarsest shading rate in corners which are not visible.

typedef int varjo_ShadingRate;
static const varjo_ShadingRate varjo_ShadingRate_Cull = 0;
static const varjo_ShadingRate varjo_ShadingRate_X16PerPixel = 1;
static const varjo_ShadingRate varjo_ShadingRate_X8PerPixel = 2;
static const varjo_ShadingRate varjo_ShadingRate_X4PerPixel = 3;
static const varjo_ShadingRate varjo_ShadingRate_X2PerPixel = 4;
static const varjo_ShadingRate varjo_ShadingRate_1x1 = 5;
static const varjo_ShadingRate varjo_ShadingRate_1x2 = 6;
static const varjo_ShadingRate varjo_ShadingRate_2x1 = 7;
static const varjo_ShadingRate varjo_ShadingRate_2x2 = 8;
static const varjo_ShadingRate varjo_ShadingRate_2x4 = 9;
static const varjo_ShadingRate varjo_ShadingRate_4x2 = 10;
static const varjo_ShadingRate varjo_ShadingRate_4x4 = 11;

struct varjo_ShadingRateTable {
    varjo_ShadingRate shadingRates[16];
};

typedef int64_t varjo_StructureType;

struct varjo_StructureExtension {
    varjo_StructureType type;
    struct varjo_StructureExtension* next;
};

struct varjo_VariableRateShadingConfig {
    struct varjo_StructureExtension* next;
    struct varjo_Gaze gaze;          //!< Valid gaze, returned by varjo_GetRenderingGaze().
    int32_t viewIndex;               //!< View for which VRS map should be generated.
    struct varjo_Viewport viewport;  //!< Viewport where VRS map should be generated inside given texture (can be whole texture or a part of it).
    varjo_VariableRateShadingFlags flags;
    /**
     * Radius of the best quality shading rate of the foveated circle if gaze is enabled and calibrated
     * (flags should contain varjo_VariableRateShadingFlag_Gaze).
     * Normalized value between 0 and 1 where 1 is half width of the viewport.
     */
    float innerRadius;
    /**
     * Radius of the outer edge of foveated circle. Medium quality shading rate is generated between
     * range [innerRadius, outerRadius]. Should be bigger than innerRadius.
     * Normalized value between 0 and 1 where 1 is half width of the viewport.
     */
    float outerRadius;
};

#if defined __cplusplus
}
#endif

#endif  // VARJO_TYPES_H
