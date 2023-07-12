// Copyright 2019-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_TYPES_DATASTREAM_H
#define VARJO_TYPES_DATASTREAM_H

#include "Varjo_types.h"
#include "Varjo_types_mr.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Bitfield for different data stream channels.
 */
typedef uint64_t varjo_ChannelFlag;
static const varjo_ChannelFlag varjo_ChannelFlag_None = 0;         //!< Disables data buffers and only provides metadata.
static const varjo_ChannelFlag varjo_ChannelFlag_First = 1 << 0;   //!< First channel.
static const varjo_ChannelFlag varjo_ChannelFlag_Second = 1 << 1;  //!< Second channel.

/**
 * Helper aliases and aggregates for varjo_ChannelFlag.
 */
static const varjo_ChannelFlag varjo_ChannelFlag_Left = 1 << 0;   //!< Left camera channel. Alias for varjo_ChannelFlag_First.
static const varjo_ChannelFlag varjo_ChannelFlag_Right = 1 << 1;  //!< Right camera channel. Alias for varjo_ChannelFlag_Second.
static const varjo_ChannelFlag varjo_ChannelFlag_All = ~0ull;     //!< Aggregate of every available channel.

/**
 * Values for indexing buffers and settings of data streams.
 */
typedef int64_t varjo_ChannelIndex;
static const varjo_ChannelIndex varjo_ChannelIndex_First = 0;   //!< First channel.
static const varjo_ChannelIndex varjo_ChannelIndex_Second = 1;  //!< Second channel.

/**
 * Helper aliases for varjo_ChannelIndex.
 */
static const varjo_ChannelIndex varjo_ChannelIndex_Left = 0;   //!< Left camera channel. Alias for varjo_ChannelIndex_First.
static const varjo_ChannelIndex varjo_ChannelIndex_Right = 1;  //!< Right camera channel. Alias for varjo_ChannelIndex_Second.

/**
 * Camera intrinsics model type.
 */
typedef int64_t varjo_IntrinsicsModel;
static const varjo_IntrinsicsModel varjo_IntrinsicsModel_Omnidir = 1;  //!< OpenCV compatible Omnidir calibration model.

/**
 * Memory buffer type.
 */
typedef int64_t varjo_BufferType;
static const varjo_BufferType varjo_BufferType_CPU = 1;  //!< Data is available in a CPU memory buffer.
static const varjo_BufferType varjo_BufferType_GPU = 2;  //!< Data is available in a GPU memory buffer.

/**
 * Frame data flag buffer type.
 */
typedef uint64_t varjo_DataFlag;
static const varjo_DataFlag varjo_DataFlag_Buffer = 1 << 0;
static const varjo_DataFlag varjo_DataFlag_Intrinsics = 1 << 1;
static const varjo_DataFlag varjo_DataFlag_Extrinsics = 1 << 2;

/**
 * Data stream types.
 */
typedef int64_t varjo_StreamType;

/**
 * Distorted (i.e. uncorrected) color data stream from visible light RGB camera.
 *
 * Color data frames contain the following additional data for each channel:
 * - Buffers (access with #varjo_GetBufferId)
 * - Intrinsics (access with #varjo_GetCameraIntrinsics)
 * - Extrinsics (access with #varjo_GetCameraExtrinsics)
 */
static const varjo_StreamType varjo_StreamType_DistortedColor = 1;

/**
 * HDR lighting estimate stream as a cubemap. Values are linear, RGB=(1, 1, 1) corresponds to a luminance of 100 cd/m^2. Alpha channel indicates cubemap
 * coverage.
 *
 * Contains a single frame with the following additional data:
 * - Buffer (access with #varjo_GetBufferId).
 *
 * Cubemap layout:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *        | top    |
 * | left | front  | right | back |
 *        | bottom |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * CPU buffer contains the faces packed into a single column:
 * ~~~~~~~~~~
 * | right  |
 * | left   |
 * | top    |
 * | bottom |
 * | front  |
 * | back   |
 * ~~~~~~~~~~
 */
static const varjo_StreamType varjo_StreamType_EnvironmentCubemap = 2;

/**
 * Distorted (i.e. uncorrected) luminance data stream from eye tracking monochrome camera.
 *
 * Frames contain the following additional data for each channel:
 * - Buffers (access with #varjo_GetBufferId)
 */
static const varjo_StreamType varjo_StreamType_EyeCamera = 3;

/**
 * Stream ID to identify a data stream configuration.
 */
typedef int64_t varjo_StreamId;

/**
 * Buffer ID to lock and use a data stream buffer.
 */
typedef int64_t varjo_BufferId;

/**
 * Invalid id for any ID type (e.g. StreamId, BufferId).
 */
static const int64_t varjo_InvalidId = -1;

/**
 * Data stream configuration.
 */
struct varjo_StreamConfig {
    varjo_StreamId streamId;              //!< Id.
    varjo_ChannelFlag channelFlags;       //!< Bitfield of provided camera channels.
    varjo_StreamType streamType;          //!< Stream type.
    varjo_BufferType bufferType;          //!< Buffer type: CPU or GPU memory buffer.
    varjo_TextureFormat format;           //!< Texture format.
    struct varjo_Matrix streamTransform;  //!< Transform from HMD pose center to stream origin in view coordinates.
    int32_t frameRate;                    //!< Frame rate in frames per second
    int32_t width;                        //!< Texture width.
    int32_t height;                       //!< Texture height;
    int32_t rowStride;                    //!< Buffer row stride in bytes.
};

/**
 * Camera intrinsics parameter for undistorting a distorted camera frame.
 */
struct varjo_CameraIntrinsics {
    varjo_IntrinsicsModel model;       //!< Intrisics calibration model.
    double principalPointX;            //!< Camera principal point X.
    double principalPointY;            //!< Camera principal point Y.
    double focalLengthX;               //!< Camera focal length X.
    double focalLengthY;               //!< Camera focal length Y.
    double distortionCoefficients[6];  //!< Intrinsics model coefficients. For omnidir: 2 radial, skew, xi, 2 tangential.
};

/**
 * Buffer for data streams.
 */
struct varjo_BufferMetadata {
    varjo_TextureFormat format;  //!< Texture format.
    varjo_BufferType type;       //!< CPU or GPU.
    int32_t byteSize;            //!< Buffer size in bytes.
    int32_t rowStride;           //!< Buffer row stride in bytes.
    int32_t width;               //!< Image width.
    int32_t height;              //!< Image height.
};

/**
 *  White balance normalization data for color matching VR content with camera image.
 *
 *  It is assumed that VR content is rendered as if it uses a fixed 6500K white balance. Color conversion to camera
 *  white balance color temperature is done with the following formula:
 *
 *  finalColor = saturate(color * invCCM * diag(whiteBalanceColorGains)) * ccm
 *
 *  where saturate(x) means clamping out-of-bounds values to [0, 2^B-1] range assuming B-bit image and diag(x) is a
 *  diagonal matrix with x values on the diagonal.
 */
struct varjo_WBNormalizationData {
    double whiteBalanceColorGains[3];  //!< White balance RGB gains to convert from 6500K to VST color temperature.
    struct varjo_Matrix3x3 invCCM;     //!< Inverse CCM for 6500K color temperature.
    struct varjo_Matrix3x3 ccm;        //!< CCM for VST color temperature.
};

/**
 * Metadata for distorted color stream
 *
 * This metadata is available in #varjo_StreamFrameMetadata structure
 * when #varjo_StreamFrame::type equals #varjo_StreamType_DistortedColor.
 */
struct varjo_DistortedColorFrameMetadata {
    varjo_Nanoseconds timestamp;                           //!< Timestamp at the middle of frame exposure.
    double ev;                                             //!< EV (exposure value) at ISO100.
    double exposureTime;                                   //!< Exposure time in seconds.
    double whiteBalanceTemperature;                        //!< Camera white balance color temperature in Kelvin degrees.
    struct varjo_WBNormalizationData wbNormalizationData;  //!< White balance normalization data.
    double cameraCalibrationConstant;                      //!< The luminance (in cd/m^2) which saturates a pixel is equal to 2^ev * cameraCalibrationConstant.
};

/**
 * Metadata for environment cubemap stream.
 *
 * This metadata is available in #varjo_StreamFrameMetadata structure
 * when #varjo_StreamFrame::type equals #varjo_StreamType_EnvironmentCubemap.
 */
struct varjo_EnvironmentCubemapFrameMetadata {
    varjo_Nanoseconds timestamp;                           //!< Timestamp when the cubemap was last updated.
    varjo_EnvironmentCubemapMode mode;                     //!< Cubemap mode which defines the color and brightness of the cubemap data.
    double whiteBalanceTemperature;                        //!< Cubemap white balance color temperature in Kelvin degrees.
    double brightnessNormalizationGain;                    //!< Gain to convert cubemap to VST image brightness.
    struct varjo_WBNormalizationData wbNormalizationData;  //!< White balance normalization data.
};

/**
 * Metadata for eye camera stream.
 *
 * This metadata is available in #varjo_StreamFrameMetadata structure
 * when #varjo_StreamFrame::type equals #varjo_StreamType_EyeCamera.
 *
 * Glint LEDs are numbered in clockwise direction for left eye and counter clockwise for right
 * eye starting from middle nose side glint LED. Refer to developer documentation for more
 * information.
 */
struct varjo_EyeCameraFrameMetadata {
    varjo_Nanoseconds timestamp;  //!< Timestamp when the frame was captured (start of frame).
    uint32_t glintMaskLeft;       //!< Bit mask of enabled glint LEDs of left chamber
    uint32_t glintMaskRight;      //!< Bit mask of enabled glint LEDs of right chamber
};

/**
 * Wrapper union for different types of streams.
 */
union varjo_StreamFrameMetadata {
    struct varjo_DistortedColorFrameMetadata distortedColor;
    struct varjo_EnvironmentCubemapFrameMetadata environmentCubemap;
    struct varjo_EyeCameraFrameMetadata eyeCamera;
    int64_t reserved[64];
};

/**
 * A single frame that will arrive to FrameListener callback after the user has subscribed to a stream.
 */
struct varjo_StreamFrame {
    varjo_StreamType type;                     //!< Type of the stream.
    varjo_StreamId id;                         //!< Id of the stream.
    int64_t frameNumber;                       //!< Monotonically increasing frame number.
    varjo_ChannelFlag channels;                //!< Channels that this frame contains.
    varjo_DataFlag dataFlags;                  //!< Data that this frame contains.
    struct varjo_Matrix hmdPose;               //!< HMD world pose. Invert the pose, if you need the HMD center view matrix.
    union varjo_StreamFrameMetadata metadata;  //!< Frame data. Use 'type' to determine which element to access.
};

/**
 * Camera frame stream callback function.
 *
 * @param frame Incoming frame.
 * @param session Varjo session pointer for this callback.
 * @param userData User controlled pointer that can be supplied when subscribing to a data stream.
 */
typedef void(varjo_FrameListener)(const struct varjo_StreamFrame* frame, struct varjo_Session* session, void* userData);

#if defined __cplusplus
}
#endif
#endif  // VARJO_TYPES_DATASTREAM_H
