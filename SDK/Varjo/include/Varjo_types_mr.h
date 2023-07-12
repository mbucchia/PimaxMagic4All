// Copyright 2019-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_TYPES_MR_H
#define VARJO_TYPES_MR_H

#include "Varjo_types.h"

#if defined __cplusplus
extern "C" {
#endif

// Mixed reality lock API

/**
 * Mixed reality lock types.
 */
typedef int64_t varjo_LockType;
static const varjo_LockType varjo_LockType_Camera = 1;              //!< Lock for camera properties.
static const varjo_LockType varjo_LockType_ChromaKey = 2;           //!< Lock for chroma key configuration.
static const varjo_LockType varjo_LockType_EnvironmentCubemap = 3;  //!< Lock for environment cubemap configuration.

// Chroma keying API

/**
 * Chroma keying config types for different color matchers.
 */
typedef int64_t varjo_ChromaKeyType;
static const varjo_ChromaKeyType varjo_ChromaKeyType_Disabled = 0;  //!< Chroma key matching disabled.
static const varjo_ChromaKeyType varjo_ChromaKeyType_HSV = 1;       //!< Chroma key matcher in HSV colorspace.

/**
 * Chroma keying parameters for HSV colorspace chroma key matcher
 */
struct varjo_ChromaKeyParams_HSV {
    double targetColor[3];  //!< Target color in HSV colorspace (range 0.0 .. 1.0).
    double tolerance[3];    //!< Tolerances for HSV components (range 0.0 .. 1.0).
    double falloff[3];      //!< Tolerance falloffs for HSV components (range 0.0 .. 1.0).
};

/**
 * Wrapper for different chroma key matcher parameters
 */
union varjo_ChromaKeyParams {
    struct varjo_ChromaKeyParams_HSV hsv;  //!< Parameters for varjo_ChromaKeyType_HSV type chroma key matcher.
    int64_t reserved[32];                  //!< Reserved for future use.
};

/**
 * Chroma key config structure including type info and actual parameters.
 */
struct varjo_ChromaKeyConfig {
    varjo_ChromaKeyType type;            //!< Chroma key type.
    union varjo_ChromaKeyParams params;  //!< Parameters union. Determine from 'type' which element to access.
};

// Environment cubemap configuration API

typedef int64_t varjo_EnvironmentCubemapMode;

//! Cubemap uses fixed 6500K color temperature and brightness is expressed in EV100.
static const varjo_EnvironmentCubemapMode varjo_EnvironmentCubemapMode_Fixed6500K = 0;

//! Cubemap color temperature and brightness are automatically adapted to match video pass through image.
static const varjo_EnvironmentCubemapMode varjo_EnvironmentCubemapMode_AutoAdapt = 1;

/**
 * Environment cubemap config structure.
 */
struct varjo_EnvironmentCubemapConfig {
    varjo_EnvironmentCubemapMode mode;  //!< Cubemap mode.
    int64_t reserved[32];
};


// Camera property API

/**
 * Camera property config type.
 */
typedef int64_t varjo_CameraPropertyConfigType;
static const varjo_CameraPropertyConfigType varjo_CameraPropertyConfigType_List = 1;   //!< List of discrete values.
static const varjo_CameraPropertyConfigType varjo_CameraPropertyConfigType_Range = 2;  //!< Range (min, max).

/**
 * Camera property data types.
 */
typedef int64_t varjo_CameraPropertyDataType;
static const varjo_CameraPropertyDataType varjo_CameraPropertyDataType_Int = 1;     //!< Integer.
static const varjo_CameraPropertyDataType varjo_CameraPropertyDataType_Double = 2;  //!< Floating point.
static const varjo_CameraPropertyDataType varjo_CameraPropertyDataType_Bool = 3;    //!< Boolean.

/**
 * Camera property types that can be accessed to control video pass through cameras.
 *
 * See #varjo_MRGetCameraPropertyModes and #varjo_MRGetCameraPropertyValues documentation for details.
 */
typedef int64_t varjo_CameraPropertyType;
static const varjo_CameraPropertyType varjo_CameraPropertyType_ExposureTime = 1;         //!< Exposure time.
static const varjo_CameraPropertyType varjo_CameraPropertyType_ISOValue = 2;             //!< ISO value.
static const varjo_CameraPropertyType varjo_CameraPropertyType_WhiteBalance = 3;         //!< White balance.
static const varjo_CameraPropertyType varjo_CameraPropertyType_FlickerCompensation = 4;  //!< Flicker compensation (50Hz and 60Hz).
static const varjo_CameraPropertyType varjo_CameraPropertyType_Sharpness = 5;            //!< Sharpness filter mode.
static const varjo_CameraPropertyType varjo_CameraPropertyType_EyeReprojection =
    6;  //!< Reproject the image to correspond the physical position of user's eyes.

/**
 * Camera property modes that can be set to control video pass through cameras.
 *
 * See #varjo_MRGetCameraPropertyModes documentation for details.
 */
typedef int64_t varjo_CameraPropertyMode;
static const varjo_CameraPropertyMode varjo_CameraPropertyMode_Off = 0;     //!< Off mode.
static const varjo_CameraPropertyMode varjo_CameraPropertyMode_Auto = 1;    //!< Automatic mode.
static const varjo_CameraPropertyMode varjo_CameraPropertyMode_Manual = 2;  //!< Manual value mode.

/**
 * Wrapper for different camera property values.
 */
union varjo_CameraValue {
    double doubleValue;    //!< Floating point value.
    int64_t intValue;      //!< Integer value.
    varjo_Bool boolValue;  //!< Boolean value.
};

/**
 * Camera property value structure including type info and actual value.
 */
struct varjo_CameraPropertyValue {
    varjo_CameraPropertyDataType type;  //!< Data type for this value.
    union varjo_CameraValue value;      //!< Value union. Determine from 'type' which element to access.
};

#if defined __cplusplus
}
#endif
#endif  // VARJO_TYPES_MR_H
