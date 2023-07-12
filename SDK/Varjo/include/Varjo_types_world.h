// Copyright 2020-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_TYPES_WORLD_H
#define VARJO_TYPES_WORLD_H

#include "Varjo_types.h"

#if defined __cplusplus
extern "C" {
#endif

/*
 * An opaque handle for a varjo_World instance.
 */
struct varjo_World;

/**
 * Configuration mask for varjo_WorldInit.
 *
 * Currently the only supported value is
 * #varjo_WorldFlag_UseObjectMarkers
 */
typedef uint64_t varjo_WorldFlags;
static const varjo_WorldFlags varjo_WorldFlag_UseObjectMarkers = 0x1;  //!< Initialize object marker tracking backend for the varjo_World instance.

/**
 * A unique id for an object in varjo_World.
 * Each varjo_World instance has a different set of object ids.
 */
typedef int64_t varjo_WorldObjectId;

/**
 * Bit mask for filtering a World object.
 *
 * List of world component types:
 *
 *   - #varjo_WorldComponentTypeMask_Pose
 *   - #varjo_WorldComponentTypeMask_ObjectMarker
 */
typedef uint64_t varjo_WorldComponentTypeMask;
static const varjo_WorldComponentTypeMask varjo_WorldComponentTypeMask_Pose = 0x1;          //!< Object contains a varjo_PoseComponent.
static const varjo_WorldComponentTypeMask varjo_WorldComponentTypeMask_ObjectMarker = 0x2;  //!< Object contains a varjo_ObjectMarkerComponent.

/**
 * Represents a single object in a varjo_World.
 * One object can have multiple data component that are described by the typeMask.
 */
struct varjo_WorldObject {
    varjo_WorldObjectId id;                 //!< Id of the object. Valid for the lifetime of the world.
    varjo_WorldComponentTypeMask typeMask;  //!< Mask for all contained component types.
    uint64_t reserved[2];
};

/**
 * World pose flags.
 *
 * List of flags:
 *
 *   - #varjo_WorldPoseFlags_TrackingOk
 *   - #varjo_WorldPoseFlags_TrackingLost
 *   - #varjo_WorldPoseFlags_TrackingDisconnected
 *   - #varjo_WorldPoseFlags_HasPosition
 *   - #varjo_WorldPoseFlags_HasRotation
 *   - #varjo_WorldPoseFlags_HasVelocity
 *   - #varjo_WorldPoseFlags_HasAngularVelocity
 *   - #varjo_WorldPoseFlags_HasAcceleration
 *   - #varjo_WorldPoseFlags_HasConfidence
 */
typedef uint64_t varjo_WorldPoseFlags;
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_TrackingOk = 0x1;    //!< Pose is being currently tracked
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_TrackingLost = 0x2;  //!< Pose has been tracked but currently is not detected by the tracking subsystem
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_TrackingDisconnected = 0x4;  //!< Tracking subsystem is not connected and poses cannot be acquired
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_HasPosition = 0x08;          //!< Pose has position information
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_HasRotation = 0x10;          //!< Pose has rotation information
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_HasVelocity = 0x20;          //!< Pose has velocity information
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_HasAngularVelocity = 0x40;   //!< Pose has angular velocity information
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_HasAcceleration = 0x80;      //!< Pose has acceleration information
static const varjo_WorldPoseFlags varjo_WorldPoseFlags_HasConfidence = 0x100;       //!< Pose has confidence information

/**
 * Represents a pose of an object.
 * Pose transformation is in global space.
 */
struct varjo_WorldPoseComponent {
    struct varjo_Matrix pose;               //!< Pose of the object in the global space. Translation and rotation only.
    struct varjo_Vector3D velocity;         //!< Linear velocity (m/s).
    struct varjo_Vector3D angularVelocity;  //!< Angular velocity (radians/s).
    struct varjo_Vector3D acceleration;     //!< Acceleration (m/s^2).
    varjo_WorldPoseFlags poseFlags;         //!< Bit field value describing pose. See #varjo_WorldPoseFlags.
    varjo_Nanoseconds timeStamp;            //!< Timestamp of the pose. This represents the time the pose has been extrapolated to.
    double confidence;                      //!< Tracker confidence in range [0.0, 1.0] (1.0 is accurate).
};

typedef int64_t varjo_WorldMarkerId;

/**
 * Configuration flags for markers. See #varjo_WorldObjectMarkerFlags_DoPrediction.
 */
typedef int64_t varjo_WorldObjectMarkerFlags;

/**
 * Marker to be tracked as dynamic which means more responsive pose updates according to marker's movements.
 * If not specified, the marker is treated as stationary - less responsive to marker's movements.
 */
static const varjo_WorldObjectMarkerFlags varjo_WorldObjectMarkerFlags_DoPrediction = 0x1;

/**
 * Type describing marker tracking error code.
 *
 * Marker error state is persistent and can be reset only in VarjoBase.
 * List of possible values:
 *
 *   - #varjo_WorldObjectMarkerError_None
 *   - #varjo_WorldObjectMarkerError_DuplicateID
 */
typedef int64_t varjo_WorldObjectMarkerError;
static const varjo_WorldObjectMarkerError varjo_WorldObjectMarkerError_None = 0;         //!< No error
static const varjo_WorldObjectMarkerError varjo_WorldObjectMarkerError_DuplicateID = 1;  //!< Several markers have the same ID

/**
 * Represents an object marker.
 * The pose is provided by varjo_PoseComponent.
 *
 * varjo_PoseComponent translation is the center of the marker.
 */
struct varjo_WorldObjectMarkerComponent {
    varjo_WorldMarkerId id;              //!< Unique id of the marker.
    varjo_WorldObjectMarkerFlags flags;  //!< Marker flags.
    varjo_WorldObjectMarkerError error;  //!< Marker error.
    struct varjo_Size3D size;            //!< Size of the marker in meters.
};

#if defined __cplusplus
}
#endif

#endif  // VARJO_TYPES_WORLD_H
