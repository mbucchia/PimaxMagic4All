// Copyright 2020-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_WORLD_H
#define VARJO_WORLD_H

#include "Varjo_types_world.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Initialize a world.
 * One varjo_Session can have multiple worlds.
 * Each world has it own set of objects and they can overlap if
 * the worlds have the same flags. The same physical object can be represented by a different id
 * in a different varjo_World. See @ref varjo_WorldFlags for supported values for worldFlags.
 *
 * @param session Varjo session the world should be initialized for.
 * @param worldFlags Configuration flags for the world initialization.
 * @return An opaque handle to the world object.
 */
VARJO_API struct varjo_World* varjo_WorldInit(struct varjo_Session* session, varjo_WorldFlags worldFlags);

/**
 * Destroy a varjo_World instance.
 *
 * @param world Varjo world instance to destroy.
 */
VARJO_API void varjo_WorldDestroy(struct varjo_World* world);

/**
 * Sync all objects tracked in the world.
 *
 * varjo_WorldSync must be called to get up to date information about tracked
 * objects from the API. The state of the information about tracked objects
 * only changes when varjo_WorldSync is called. The information about
 * tracked objects does not changed between two consecutive calls to varjo_WorldSync.
 * Typically an application using this API calls varjo_WorldSync once per frame.
 *
 * @param world World to sync.
 */
VARJO_API void varjo_WorldSync(struct varjo_World* world);

/**
 * Get the number of objects in the world.
 *
 * The function counts world objects which have at least one of the
 * component types specified by typeMask. If the caller specifies zero
 * as typeMask, then the function returns all objects.
 * See #varjo_WorldComponentTypeMask for the list of component types.
 *
 * @param world World to get the objects from.
 * @param typeMask Mask of the desired object types. Zero if you want all of them.
 * @returns The number of objects in the world matching the mask.
 */
VARJO_API int64_t varjo_WorldGetObjectCount(struct varjo_World* world, varjo_WorldComponentTypeMask typeMask);

/**
 * Get a list of objects in the world.
 *
 * Use @ref varjo_WorldGetObjectCount with the same typeMask before calling this function
 * to get the number of objects which will be returned by this function.
 *
 * @param world World to get the objects from.
 * @param objects Output pointer for the objects.
 * @param maxObjectCount Maximum object count.
 * @param typeMask Mask of the desired object types. Zero if you want all of them.
 * @returns The number of objects in the world matching the mask.
 */
VARJO_API int64_t varjo_WorldGetObjects(
    struct varjo_World* world, struct varjo_WorldObject* objects, int64_t maxObjectCount, varjo_WorldComponentTypeMask typeMask);

/**
 * Get a pose component.
 *
 * varjo_WorldObject::typeMask should be checked before trying to get components.
 * Pose is extrapolated for the given timestamp.
 *
 * The extrapolated pose may not match the real pose if the tracking system
 * gets better data during the extrapolated time frame.
 *
 * The timestamp can be in the past to a certain degree, there is a small history kept
 * each time @ref varjo_WorldSync is called.
 *
 * Use @ref varjo_FrameGetDisplayTime to get the time the frame is going to be displayed.
 * This is going to give the best results in most cases.
 *
 * @param world World to get the component from.
 * @param id Id of the object.
 * @param component Pointer to the component to fill.
 * @param displayTime Absolute timestamp the tracking data is needed for.
 * @return True if the component was found.
 */
VARJO_API varjo_Bool varjo_WorldGetPoseComponent(
    struct varjo_World* world, varjo_WorldObjectId id, struct varjo_WorldPoseComponent* component, varjo_Nanoseconds displayTime);

/**
 * Get a object marker component.
 * varjo_WorldObject::typeMask should be checked before trying to get components.
 *
 * @param world World to get the component from.
 * @param id Id of the object.
 * @param component Pointer to the component to fill.
 * @return True if the component was found.
 */
VARJO_API varjo_Bool varjo_WorldGetObjectMarkerComponent(struct varjo_World* world, varjo_WorldObjectId id, struct varjo_WorldObjectMarkerComponent* component);

/**
 * Sets a desired lifetime duration for specified markers.
 * Markers that have not been tracked for a period of time exceeding @c duration
 * will not be returned by @ref varjo_WorldGetObjects.
 *
 * Once a marker is expired, the world object id associated with it will not be reused.
 *
 * @param world Varjo world handle.
 * @param ids Ids of the markers to use the specified timeout.
 * @param idCount Number of ids to set.
 * @param duration A time in nanoseconds for which the system will keep marker information
 * for markers that are not currently visible.
 */
VARJO_API void varjo_WorldSetObjectMarkerTimeouts(struct varjo_World* world, varjo_WorldMarkerId* ids, int64_t idCount, varjo_Nanoseconds duration);

/**
 * Sets flags for specifed markers.
 *
 * Replaces all flags set previously for the specified markers.
 * See #varjo_WorldObjectMarkerFlags.
 *
 * @param world Varjo world handle.
 * @param ids Ids of the markers to set the flags for.
 * @param idCount Number of ids to set.
 * @param flags Marker flags to set.
 */
VARJO_API void varjo_WorldSetObjectMarkerFlags(struct varjo_World* world, varjo_WorldMarkerId* ids, int64_t idCount, varjo_WorldObjectMarkerFlags flags);

#if defined __cplusplus
}
#endif

#endif  // VARJO_WORLD_H
