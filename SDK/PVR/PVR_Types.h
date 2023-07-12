/************************************************************************************

Filename    :   PVR_Types.h
Content     :   type definitions.

Copyright   :   Copyright 2017 Pimax, Inc. All Rights reserved.
************************************************************************************/
#ifndef PVR_TYPE_H
#define PVR_TYPE_H
#include <stdint.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())
#pragma warning(disable: 4359) // The alignment specified for a type is less than the alignment of the type of one of its data members
#endif



//-----------------------------------------------------------------------------------
// ***** PVR_OS
//
#if !defined(PVR_OS_WIN32) && defined(_WIN32)
#define PVR_OS_WIN32
#endif

#if !defined(PVR_OS_MAC) && defined(__APPLE__)
#define PVR_OS_MAC
#endif

#if !defined(PVR_OS_LINUX) && defined(__linux__)
#define PVR_OS_LINUX
#endif


#  define   PVR_UNUSED(a)   (a)

#define     PVR_UNUSED1(a1) PVR_UNUSED(a1)
#define     PVR_UNUSED2(a1,a2) PVR_UNUSED(a1); PVR_UNUSED(a2)
#define     PVR_UNUSED3(a1,a2,a3) PVR_UNUSED2(a1,a2); PVR_UNUSED(a3)
#define     PVR_UNUSED4(a1,a2,a3,a4) PVR_UNUSED3(a1,a2,a3); PVR_UNUSED(a4)
#define     PVR_UNUSED5(a1,a2,a3,a4,a5) PVR_UNUSED4(a1,a2,a3,a4); PVR_UNUSED(a5)
#define     PVR_UNUSED6(a1,a2,a3,a4,a5,a6) PVR_UNUSED4(a1,a2,a3,a4); PVR_UNUSED2(a5,a6)
#define     PVR_UNUSED7(a1,a2,a3,a4,a5,a6,a7) PVR_UNUSED4(a1,a2,a3,a4); PVR_UNUSED3(a5,a6,a7)
#define     PVR_UNUSED8(a1,a2,a3,a4,a5,a6,a7,a8) PVR_UNUSED4(a1,a2,a3,a4); PVR_UNUSED4(a5,a6,a7,a8)
#define     PVR_UNUSED9(a1,a2,a3,a4,a5,a6,a7,a8,a9) PVR_UNUSED4(a1,a2,a3,a4); PVR_UNUSED5(a5,a6,a7,a8,a9)


//-----------------------------------------------------------------------------------
// ***** PVR_CPP
//
#if !defined(PVR_CPP)
#if defined(__cplusplus)
#define PVR_CPP(x) x
#else
#define PVR_CPP(x) /* Not C++ */
#endif
#endif



//-----------------------------------------------------------------------------------
// ***** PVR_CDECL
//
/// LibPVR calling convention for 32-bit Windows builds.
//
#if !defined(PVR_CDECL)
#if defined(_WIN32)
#define PVR_CDECL __cdecl
#else
#define PVR_CDECL
#endif
#endif



//-----------------------------------------------------------------------------------
// ***** PVR_EXTERN_C
//
/// Defined as extern "C" when built from C++ code.
//
#if !defined(PVR_EXTERN_C)
#ifdef __cplusplus
#define PVR_EXTERN_C extern "C"
#else
#define PVR_EXTERN_C
#endif
#endif

//-----------------------------------------------------------------------------------
// ***** PVR_ALIGNAS
//
#if !defined(PVR_ALIGNAS)
#if defined(__GNUC__) || defined(__clang__)
#define PVR_ALIGNAS(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define PVR_ALIGNAS(n) __declspec(align(n))
#elif defined(__CC_ARM)
#define PVR_ALIGNAS(n) __align(n)
#else
#error Need to define PVR_ALIGNAS
#endif
#endif


//-----------------------------------------------------------------------------------
// ***** PVR_CC_HAS_FEATURE
//
// This is a portable way to use compile-time feature identification available
// with some compilers in a clean way. Direct usage of __has_feature in preprocessing
// statements of non-supporting compilers results in a preprocessing error.
//
// Example usage:
//     #if PVR_CC_HAS_FEATURE(is_pod)
//         if(__is_pod(T)) // If the type is plain data then we can safely memcpy it.
//             memcpy(&destObject, &srcObject, sizeof(object));
//     #endif
//
#if !defined(PVR_CC_HAS_FEATURE)
#if defined(__clang__) // http://clang.llvm.org/docs/LanguageExtensions.html#id2
#define PVR_CC_HAS_FEATURE(x) __has_feature(x)
#else
#define PVR_CC_HAS_FEATURE(x) 0
#endif
#endif


// ------------------------------------------------------------------------
// ***** PVR_STATIC_ASSERT
//
// Portable support for C++11 static_assert().
// Acts as if the following were declared:
//     void PVR_STATIC_ASSERT(bool const_expression, const char* msg);
//
// Example usage:
//     PVR_STATIC_ASSERT(sizeof(int32_t) == 4, "int32_t expected to be 4 bytes.");

#if !defined(PVR_STATIC_ASSERT)
#if !(defined(__cplusplus) && (__cplusplus >= 201103L)) /* Other */ && \
        !(defined(__GXX_EXPERIMENTAL_CXX0X__)) /* GCC */ && \
        !(defined(__clang__) && defined(__cplusplus) && PVR_CC_HAS_FEATURE(cxx_static_assert)) /* clang */ && \
        !(defined(_MSC_VER) && (_MSC_VER >= 1600) && defined(__cplusplus)) /* VS2010+  */

#if !defined(PVR_SA_UNUSED)
#if defined(PVR_CC_GNU) || defined(PVR_CC_CLANG)
#define PVR_SA_UNUSED __attribute__((unused))
#else
#define PVR_SA_UNUSED
#endif
#define PVR_SA_PASTE(a,b) a##b
#define PVR_SA_HELP(a,b)  PVR_SA_PASTE(a,b)
#endif

#if defined(__COUNTER__)
#define PVR_STATIC_ASSERT(expression, msg) typedef char PVR_SA_HELP(compileTimeAssert, __COUNTER__) [((expression) != 0) ? 1 : -1] PVR_SA_UNUSED
#else
#define PVR_STATIC_ASSERT(expression, msg) typedef char PVR_SA_HELP(compileTimeAssert, __LINE__) [((expression) != 0) ? 1 : -1] PVR_SA_UNUSED
#endif

#else
#define PVR_STATIC_ASSERT(expression, msg) static_assert(expression, msg)
#endif
#endif


//-----------------------------------------------------------------------------------
// ***** Padding
//
/// Defines explicitly unused space for a struct.
/// When used correcly, usage of this macro should not change the size of the struct.
/// Compile-time and runtime behavior with and without this defined should be identical.
///
#if !defined(PVR_UNUSED_STRUCT_PAD)
#define PVR_UNUSED_STRUCT_PAD(padName, size) char padName[size]
#endif


//-----------------------------------------------------------------------------------
// ***** Word Size
//
/// Specifies the size of a pointer on the given platform.
///
#if !defined(PVR_PTR_SIZE)
#if defined(__WORDSIZE)
#define PVR_PTR_SIZE ((__WORDSIZE) / 8)
#elif defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(_M_IA64) || defined(__ia64__) || defined(__arch64__) || defined(__64BIT__) || defined(__Ptr_Is_64)
#define PVR_PTR_SIZE 8
#elif defined(__CC_ARM) && (__sizeof_ptr == 8)
#define PVR_PTR_SIZE 8
#else
#define PVR_PTR_SIZE 4
#endif
#endif


//-----------------------------------------------------------------------------------
// ***** PVR_ON32 / PVR_ON64
//
#if PVR_PTR_SIZE == 8
#define PVR_ON32(x)
#define PVR_ON64(x) x
#define PVR_64  1
#define PVR_32  0
#else
#define PVR_ON32(x) x
#define PVR_ON64(x)
#define PVR_64  0
#define PVR_32  1
#endif

#define PVR_FORCE_INLINE __inline

//-----------------------------------------------------------------------------------
// ***** pvrBool

typedef char pvrBool;
#define pvrFalse 0      ///< false.
#define pvrTrue  1      ///< true.


/// A integer 2D vector.
typedef struct PVR_ALIGNAS(4) pvrVector2i_
{
	int x, y;
} pvrVector2i;

/// A integer 2D size.
typedef struct PVR_ALIGNAS(4) pvrSizei_
{
	int w, h;
} pvrSizei;

/// A integer 2D rectangle with a position and size.
typedef struct PVR_ALIGNAS(4) pvrRecti_
{
	pvrVector2i Pos;
	pvrSizei    Size;
} pvrRecti;

/// A quaternion rotation.
typedef struct PVR_ALIGNAS(4) pvrQuatf_
{
	float x, y, z, w;
} pvrQuatf;

/// A float 2D vector.
typedef struct PVR_ALIGNAS(4) pvrVector2f_
{
	float x, y;
} pvrVector2f;

/// A float 3D vector.
typedef struct PVR_ALIGNAS(4) pvrVector3f_
{
	float x, y, z;
} pvrVector3f;

/// float 4x4 matrix.
typedef struct PVR_ALIGNAS(4) pvrMatrix4f_
{
	float M[4][4];
} pvrMatrix4f;

//tracked device.
typedef enum pvrTrackedDeviceType_
{
	pvrTrackedDevice_HMD = 0x00000001,
	pvrTrackedDevice_LeftController = 0x00000002,
	pvrTrackedDevice_RightController = 0x00000004,
	pvrTrackedDevice_Tracker0 = 0x00000008,
	pvrTrackedDevice_Tracker1 = 0x00000010,
	pvrTrackedDevice_Tracker2 = 0x00000020,
	pvrTrackedDevice_Tracker3 = 0x00000040,
	pvrTrackedDevice_Tracker4 = 0x00000080,
	pvrTrackedDevice_Tracker5 = 0x00000100,
	pvrTrackedDevice_Tracker6 = 0x00000200,
	pvrTrackedDevice_Tracker7 = 0x00000400,
	pvrTrackedDevice_Tracker8 = 0x00000800,
	pvrTrackedDevice_Tracker9 = 0x00001000,
	pvrTrackedDevice_Tracker10 = 0x00002000,
	pvrTrackedDevice_Tracker11 = 0x00004000,
	pvrTrackedDevice_Tracker12 = 0x00008000,

	pvrTrackedDevice_Custom1 = 0x00010000,
	pvrTrackedDevice_Custom2 = 0x00020000,
	pvrTrackedDevice_Custom3 = 0x00040000,
	pvrTrackedDevice_Custom4 = 0x00080000,
	pvrTrackedDevice_Custom5 = 0x00100000,
	pvrTrackedDevice_Custom6 = 0x00200000,
	pvrTrackedDevice_Custom7 = 0x00400000,
	pvrTrackedDevice_Custom8 = 0x00800000,
	pvrTrackedDevice_Custom9 =  0x01000000,
	pvrTrackedDevice_Custom10 = 0x02000000,
	pvrTrackedDevice_Custom11 = 0x04000000,
	pvrTrackedDevice_Custom12 = 0x08000000,
	pvrTrackedDevice_Custom13 = 0x10000000,
	pvrTrackedDevice_Custom14 = 0x20000000,
	pvrTrackedDevice_Custom15 = 0x40000000,
	pvrTrackedDevice_Custom16 = 0x80000000,
} pvrTrackedDeviceType;

//capabilities of tracked device.
typedef enum pvrTrackedDeviceCap_
{
	pvrTrackedDeviceCap_Oritentation = 0x0001,
	pvrTrackedDeviceCap_Position = 0x0002,
} pvrTrackedDeviceCap;

typedef enum pvrHmdTrackingStyle_
{
	pvrHmdTrackingStyle_Unknown = 0,
	pvrHmdTrackingStyle_Imu = 1,
	pvrHmdTrackingStyle_Lighthouse = 2,
	pvrHmdTrackingStyle_InsideOutCameras = 3,
} pvrHmdTrackingStyle;

//properties of tracked device.
typedef enum pvrTrackedDeviceProp_
{
	pvrTrackedDeviceProp_None = 0,
	pvrTrackedDeviceProp_RenderModelTranslation_Vector3f,
	pvrTrackedDeviceProp_RenderModelRotation_Quatf,
	pvrTrackedDeviceProp_BatteryLevel_int,//pvrTrackedDeviceBateryLevel
	pvrTrackedDeviceProp_BatteryPercent_int,//0-100, -1 not support
	pvrTrackedDeviceProp_PoseRefreshRate_Float,//0 not support
	pvrTrackedDeviceProp_TrackerHFovInRadians_Float,
	pvrTrackedDeviceProp_TrackerVFovInRadians_Float,
	pvrTrackedDeviceProp_TrackerNearZInMeters_Float,
	pvrTrackedDeviceProp_TrackerFarZInMeters_Float,
	pvrTrackedDeviceProp_Product_String,
	pvrTrackedDeviceProp_Manufacturer_String,
	pvrTrackedDeviceProp_VenderId_int,
	pvrTrackedDeviceProp_ProductId_int,
	pvrTrackedDeviceProp_RenderModelName_String,
	pvrTrackedDeviceProp_InputProfilePath_String,
	pvrTrackedDeviceProp_ControllerType_String,
	pvrTrackedDeviceProp_Serial_String,
	pvrTrackedDeviceProp_ModeLabel_String,
	pvrTrackedDeviceProp_Firmware_UpdateAvailable_Bool,
	pvrTrackedDeviceProp_Firmware_ForceUpdateRequired_Bool,
	pvrTrackedDeviceProp_Firmware_ManualUpdate_Bool,
	pvrTrackedDeviceProp_Firmware_ManualUpdateURL_String,
	pvrTrackedDeviceProp_Firmware_ProgrammingTarget_String,
	pvrTrackedDeviceProp_TrackingFirmwareVersion_String,
	pvrTrackedDeviceProp_FirmwareVersion_Uint64,
	pvrTrackedDeviceProp_RegisteredDeviceType_String,
	pvrTrackedDeviceProp_HardwareRevision_Uint64,
	pvrTrackedDeviceProp_HardwareRevision_String,
	pvrTrackedDeviceProp_ResourceRoot_String,
	pvrTrackedDeviceProp_FPGAVersion_Uint64,
	pvrTrackedDeviceProp_VRCVersion_Uint64,
	pvrTrackedDeviceProp_RadioVersion_Uint64,
	pvrTrackedDeviceProp_DongleVersion_Uint64,
	pvrTrackedDeviceProp_Identifiable_Bool,
	pvrTrackedDeviceProp_ConnectedWirelessDongle_String,
	pvrTrackedDeviceProp_InputButtons_Uint64,
	pvrTrackedDeviceProp_Audio_PlaybackDeviceId_String,
	pvrTrackedDeviceProp_Audio_RecordingDeviceId_String,
	pvrTrackedDeviceProp_Prop_HmdTrackingStyle_Int,
	pvrTrackedDeviceProp_ScreenState_Bool,

	pvrTrackedDeviceProp_Private = 0x0000FFFF,
	pvrTrackedDeviceProp_Max = 0x0FFFFFFF,
} pvrTrackedDeviceProp;

typedef enum pvrTrackedDeviceBateryLevel_
{
	pvrTrackedDeviceBateryLevel_NotSupport = 0,
	pvrTrackedDeviceBateryLevel_Low,
	pvrTrackedDeviceBateryLevel_Middle,
	pvrTrackedDeviceBateryLevel_High,
}pvrTrackedDeviceBateryLevel;

/// A 3D pose contains orientation and position.
typedef struct PVR_ALIGNAS(4) pvrPosef_
{
	pvrQuatf     Orientation;
	pvrVector3f  Position;
} pvrPosef;

// tracking status.
typedef enum pvrStatusBits_
{
	pvrStatus_OrientationTracked = 0x0001,
	pvrStatus_PositionTracked = 0x0002,
} pvrStatusBits;

// tracker desc. 
typedef struct PVR_ALIGNAS(4) pvrTrackerDesc_
{
	float HFovInRadians;
	float VFovInRadians;
	float NearZInMeters;
	float FarZInMeters;
} pvrTrackerDesc;

// tracker pose
typedef struct PVR_ALIGNAS(8) _pvrTrackerPose
{
	unsigned int StatusFlags;
	pvrPosef     Pose;
} pvrTrackerPose;


// tracked device pose, status, time, velocity and acceleration.
typedef struct PVR_ALIGNAS(8) pvrPoseStatef_
{
	pvrPosef     ThePose;               ///< Position and orientation.
	pvrVector3f  AngularVelocity;       ///< Angular velocity in radians per second.
	pvrVector3f  LinearVelocity;        ///< Velocity in meters per second.
	pvrVector3f  AngularAcceleration;   ///< Angular acceleration in radians per second per second.
	pvrVector3f  LinearAcceleration;    ///< Acceleration in meters per second per second.
	double       TimeInSeconds;         ///< Absolute time that this pose refers to.
	unsigned int   StatusFlags;
	char		pad[4];
} pvrPoseStatef;

//tracking state for head(HMD), and two hand(controller)
typedef struct PVR_ALIGNAS(8) pvrTrackingState_
{
	pvrPoseStatef  HeadPose;
	pvrPoseStatef  HandPoses[2];
} pvrTrackingState;

//fov
typedef struct PVR_ALIGNAS(4) pvrFovPort_
{
	float UpTan; 
	float DownTan;
	float LeftTan;
	float RightTan;
} pvrFovPort;

//viewport
typedef struct PVR_ALIGNAS(4) pvrViewPort_
{
	int x;
	int y;
	int width;
	int height;
} pvrViewPort;


// result code.
typedef enum 
{
	pvr_success = 0,
	pvr_failed,
	pvr_dll_failed,
	pvr_dll_wrong,
	pvr_interface_not_found,
	pvr_invalid_param,
	pvr_rpc_failed,
	pvr_share_mem_failed,
	pvr_unsupport_render_name,
	pvr_no_display,
	pvr_no_render_device,
	pvr_app_not_visible,
	pvr_srv_not_ready,
	pvr_dll_srv_mismatch,
	pvr_app_adapter_mismatch,
	pvr_not_support,
}pvrResult;

//eye type
typedef enum
{
	pvrEye_Left = 0,
	pvrEye_Right = 1,
	pvrEye_Count = 2
} pvrEyeType;


typedef enum 
{
	pvrEyeDisplay_horz_side_by_side = 0,
	pvrEyeDisplay_vert_side_by_side,
	pvrEyeDisplay_full
}pvrEyeDisplayType;

typedef enum 
{
	pvrEye_rotate_0 = 0,
	pvrEye_rotate_90,
	pvrEye_rotate_180,
	pvrEye_rotate_270
}pvrEyeRotateType;

typedef enum 
{
	pvrDispState_None = 0,
	pvrDispState_Direct,
	pvrDispState_Extend
}pvrDispStateType;

//graphics adapter id, used for multi adapter supporter.
typedef struct PVR_ALIGNAS(8) pvrGraphicsLuid_
{
	char        Reserved[8];
} pvrGraphicsLuid;

//display info
typedef struct PVR_ALIGNAS(8) pvrDisplayInfo_
{
	int edid_vid;
	int edid_pid;
	pvrDispStateType disp_state;
	pvrEyeRotateType eye_rotate;
	pvrEyeDisplayType eye_display;
	int width;
	int height;
	int pos_x;
	int pos_y;
	float refresh_rate;
	pvrGraphicsLuid luid;
} pvrDisplayInfo;


// hmd info.
typedef struct PVR_ALIGNAS(8) pvrHmdInfo_
{
	char         ProductName[64];
	char         Manufacturer[64];
	int        VendorId;
	int        ProductId;
	char         SerialNumber[24];
	int        FirmwareMajor;
	int        FirmwareMinor;
	pvrSizei     Resolution;
} pvrHmdInfo;


//hmd status. Note other members should be used when ServiceReady be pvrTrue.
typedef struct PVR_ALIGNAS(8) pvrHmdStatus
{
	pvrBool IsVisible;
	pvrBool HmdPresent;
	pvrBool HmdMounted;
	pvrBool DisplayLost;
	pvrBool ServiceReady;
	pvrBool ShouldQuit;
}pvrHmdStatus;

// render info for each eye.
typedef struct PVR_ALIGNAS(8) pvrEyeRenderInfo_
{
	pvrFovPort   Fov;
	pvrVector2f  PixelsPerTanAngleAtCenter;
	pvrPosef  HmdToEyePose;
	pvrRecti     DistortedViewport;
} pvrEyeRenderInfo;

typedef struct PVR_ALIGNAS(8) pvrEyeTrackingInfo_
{
	pvrVector2f  GazeTan[pvrEye_Count]; //tan of eye gaze.
	double TimeInSeconds;
} pvrEyeTrackingInfo;

// texture type
typedef enum pvrTextureType_
{
	pvrTexture_2D,              ///< 2D textures.
	pvrTexture_Count,
	pvrTexture_EnumSize = 0x7fffffff 
} pvrTextureType;

// texture bind flags.
typedef enum pvrTextureBindFlags_
{
	pvrTextureBind_None,
	pvrTextureBind_DX_RenderTarget = 0x0001,    ///< The application can write into the chain with pixel shader
	pvrTextureBind_DX_UnorderedAccess = 0x0002, ///< The application can write to the chain with compute shader
	pvrTextureBind_DX_DepthStencil = 0x0004,    ///< The chain buffers can be bound as depth and/or stencil buffers

	pvrTextureBind_EnumSize = 0x7fffffff
} pvrTextureBindFlags;

typedef enum pvrTextureFormat_
{
	PVR_FORMAT_UNKNOWN,
	PVR_FORMAT_R8G8B8A8_UNORM,
	PVR_FORMAT_R8G8B8A8_UNORM_SRGB,
	PVR_FORMAT_B8G8R8A8_UNORM,
	PVR_FORMAT_B8G8R8A8_UNORM_SRGB, ///< Not supported for OpenGL applications
	PVR_FORMAT_B8G8R8X8_UNORM,      ///< Not supported for OpenGL applications
	PVR_FORMAT_B8G8R8X8_UNORM_SRGB, ///< Not supported for OpenGL applications
	PVR_FORMAT_R16G16B16A16_FLOAT,
	PVR_FORMAT_D16_UNORM,
	PVR_FORMAT_D24_UNORM_S8_UINT,
	PVR_FORMAT_D32_FLOAT,
	PVR_FORMAT_D32_FLOAT_S8X24_UINT,

	PVR_FORMAT_BC1_UNORM,
	PVR_FORMAT_BC1_UNORM_SRGB,
	PVR_FORMAT_BC2_UNORM,
	PVR_FORMAT_BC2_UNORM_SRGB,
	PVR_FORMAT_BC3_UNORM,
	PVR_FORMAT_BC3_UNORM_SRGB,
	PVR_FORMAT_BC6H_UF16,
	PVR_FORMAT_BC6H_SF16,
	PVR_FORMAT_BC7_UNORM,
	PVR_FORMAT_BC7_UNORM_SRGB,
	PVR_FORMAT_R11G11B10_FLOAT,
	
	PVR_FORMAT_ENUMSIZE = 0x7fffffff  ///< \internal Force type int32_t.
} pvrTextureFormat;

typedef enum pvrTextureMiscFlags_
{
	pvrTextureMisc_None,

	/// DX only: 
	pvrTextureMisc_DX_Typeless = 0x0001,

	/// DX only: 
	pvrTextureMisc_AllowGenerateMips = 0x0002,

	pvrTextureMisc_EnumSize = 0x7fffffff  ///< \internal Force type int32_t.
} pvrTextureFlags;

//texture swapchain description.
typedef struct PVR_ALIGNAS(4) pvrTextureSwapChainDesc_
{
	pvrTextureType      Type;
	pvrTextureFormat    Format;
	int                 ArraySize;      ///< Only supported with ovrTexture_2D.
	int                 Width;
	int                 Height;
	int                 MipLevels;
	int                 SampleCount; 
	pvrBool             StaticImage;
	unsigned int        MiscFlags;      ///< pvrTextureFlags
	unsigned int        BindFlags;      ///< pvrTextureBindFlags. Not used for GL.
} pvrTextureSwapChainDesc;


// for mirror texture.
typedef struct PVR_ALIGNAS(4) pvrMirrorTextureDesc_
{
	pvrTextureFormat    Format;
	int                 Width;
	int                 Height;
	int                 SampleCount;
	unsigned int        MiscFlags;      ///< pvrTextureFlags
} pvrMirrorTextureDesc;

typedef struct pvrTextureSwapChainData* pvrTextureSwapChain;
typedef struct pvrMirrorTextureData* pvrMirrorTexture;

typedef struct PVR_ALIGNAS(4) pvrDepthProjectionDesc_ {
	float Projection22;
	float Projection23;
	float Projection32;
} pvrDepthProjectionDesc;

enum {
	pvrMaxLayerCount = 16
};

typedef enum pvrLayerType_
{
	pvrLayerType_Disabled = 0,         ///< disabled.
	pvrLayerType_EyeFov = 1,         ///< pvrLayerEyeFov.
	pvrLayerType_Quad = 2,         ///< pvrLayerQuad.
	pvrLayerType_EyeFovDepth = 3,         ///< pvrLayerEyeFovDepth.

	pvrLayerType_ScreenDebug = 0x0000ffff,
	pvrLayerType_EnumSize = 0x7fffffff ///< Force type int32_t.
} pvrLayerType;

typedef enum pvrLayerFlags_
{
	/// pvrLayerFlag_TextureOriginAtBottomLeft: the opposite is TopLeft.
	/// Generally this is false for D3D, true for OpenGL.
	pvrLayerFlag_TextureOriginAtBottomLeft = 0x01,
	pvrLayerFlag_HeadLocked = 0x02
} pvrLayerFlags;

typedef struct PVR_ALIGNAS(PVR_PTR_SIZE) pvrLayerHeader_
{
	pvrLayerType    Type;   ///< Described by pvrLayerType.
	unsigned        Flags;  ///< Described by pvrLayerFlags.
} pvrLayerHeader;


typedef struct PVR_ALIGNAS(PVR_PTR_SIZE) pvrLayerEyeFov_
{
	pvrLayerHeader      Header;

	pvrTextureSwapChain  ColorTexture[pvrEye_Count];

	pvrViewPort         Viewport[pvrEye_Count];

	pvrFovPort          Fov[pvrEye_Count];

	pvrPosef            RenderPose[pvrEye_Count];

	double              SensorSampleTime;
} pvrLayerEyeFov;

typedef struct PVR_ALIGNAS(PVR_PTR_SIZE) pvrLayerEyeFovDepth_
{
	pvrLayerHeader      Header;

	pvrTextureSwapChain  ColorTexture[pvrEye_Count];

	pvrViewPort         Viewport[pvrEye_Count];

	pvrFovPort          Fov[pvrEye_Count];

	pvrPosef            RenderPose[pvrEye_Count];

	double              SensorSampleTime;

	pvrTextureSwapChain DepthTexture[pvrEye_Count];

	pvrDepthProjectionDesc DepthProjectionDesc;
} pvrLayerEyeFovDepth;

typedef struct PVR_ALIGNAS(PVR_PTR_SIZE) pvrLayerQuad_
{
	pvrLayerHeader      Header;

	pvrTextureSwapChain  ColorTexture;

	/// Specifies the ColorTexture sub-rect UV coordinates.
	pvrViewPort            Viewport;

	pvrPosef            QuadPoseCenter;

	pvrVector2f         QuadSize;

} pvrLayerQuad;

typedef struct PVR_ALIGNAS(PVR_PTR_SIZE) pvrLayerScreenDebug_
{
	pvrLayerHeader      Header;

	pvrTextureSwapChain  ColorTexture;

	/// Specifies the ColorTexture sub-rect UV coordinates.
	pvrViewPort          Viewport;
} pvrLayerScreenDebug;


typedef union pvrLayer_Union_
{
	pvrLayerHeader      Header;
	pvrLayerEyeFov      EyeFov;
	pvrLayerEyeFovDepth EyeFovDepth;
	pvrLayerQuad		Quad;
	pvrLayerScreenDebug ScreenDebug;
} pvrLayer_Union;

typedef enum pvrButton_
{
	pvrButton_System = 0x00000001,
	pvrButton_ApplicationMenu = 0x00000002,
	pvrButton_Trigger = 0x00000004,
	pvrButton_Grip = 0x00000008,
	pvrButton_TouchPad = 0x00000010,
	pvrButton_JoyStick = 0x00000020,
	pvrButton_A = 0x00000040,
	pvrButton_B = 0x00000080,
	pvrButton_X = pvrButton_A,
	pvrButton_Y = pvrButton_B,
	pvrButton_FingerIndex = 0x00000100,
	pvrButton_FingerMiddle = 0x00000200,
	pvrButton_FingerRing = 0x00000400,
	pvrButton_FingerPinky = 0x00000800,

	pvrButton_EnumSize = 0xffffffff ///< \internal Force type uint32_t.
} pvrButton;

typedef struct PVR_ALIGNAS(PVR_PTR_SIZE) pvrInputState_
{
	/// System type when the controller state was last updated.
	double              TimeInSeconds;

	/// Values for buttons described by pvrButton.
	uint32_t        HandButtons[2]; /// pressing key ORed together.
	uint32_t        HandTouches[2]; /// touching key ORed together.
	float			Trigger[2];
	float			Grip[2];
	pvrVector2f		TouchPad[2];
	pvrVector2f		JoyStick[2];

	float			GripForce[2];
	float			TouchPadForce[2];
	float           fingerIndex[2];
	float           fingerMiddle[2];
	float           fingerRing[2];
	float           fingerPinky[2];
} pvrInputState;

enum {
	PVR_MAX_SKELETAL_BONE_COUNT = 32,
};
typedef struct PVR_ALIGNAS(PVR_PTR_SIZE) pvrSkeletalData_
{
	pvrPosef boneTransforms[PVR_MAX_SKELETAL_BONE_COUNT];
	uint32_t boneCount;
}pvrSkeletalData;

enum pvrSkeletalMotionRange {
	pvrSkeletalMotionRange_WithController = 0,
	pvrSkeletalMotionRange_WithoutController = 1,
	pvrSkeletalMotionRange_Max
};

typedef enum _pvrTrackingOrigin_
{
	pvrTrackingOrigin_EyeLevel = 0,
	pvrTrackingOrigin_FloorLevel,
	pvrTrackingOrigin_Count
} pvrTrackingOrigin;

typedef void* pvrHmdHandle;

#define CONFIG_KEY_IPD			"ipd"
#define CONFIG_KEY_EYE_HEIGHT   "eye_height"          

typedef enum pvrLogLevel_ {
	pvrTrace = 1,     
	pvrDebug = 2,     
	pvrInfo = 3,      
	pvrNotice = 4,    
	pvrWarn = 5,      
	pvrErr = 6,       
	pvrCritical = 7,  
	pvrAlert = 8,     
	pvrEmerg = 9      
}pvrLogLevel;

#endif