// Copyright 2019-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_DATASTREAM_H
#define VARJO_DATASTREAM_H

#include "Varjo_export.h"
#include "Varjo_types.h"
#include "Varjo_types_datastream.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Get number of available stream configurations.
 *
 * Possible errors: #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 */
VARJO_API int32_t varjo_GetDataStreamConfigCount(struct varjo_Session* session);

/**
 * Get available data stream configurations.
 *
 * The needed size for the 'configs' output parameter can be obtained by calling #varjo_GetDataStreamConfigCount.
 *
 * Possible errors: #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable
 *
 * @param session Varjo session handle.
 * @param configs Fills this array with the available data stream configs.
 *                Can return multiple different configurations for the same stream:
 *                e.g. RawColor could delivered either to CPU or GPU memory. These will be returned as
 *                separate #varjo_StreamConfig objects.
 * @param maxSize Maximum amount of values that fit into 'configs'. If this is < #varjo_GetDataStreamConfigCount, the resulting
 *                list will be truncated.
 */
VARJO_API void varjo_GetDataStreamConfigs(struct varjo_Session* session, struct varjo_StreamConfig* configs, int32_t maxSize);

/**
 * Start frame streaming into the provided callback.
 *
 * Frames are not queued. If the previous callback has not been
 * finished when new frames arrive, the new frames are dropped.
 *
 * Callback will arrive from a worker thread.
 *
 * Note: the callback is not quaranteed to always arrive from the same worker thread.
 * Note: provided callback and user data instances must be valid and operational at
 *       all times until the data stream is stopped by calling #varjo_StopDataStream.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamInvalidCallback, #varjo_Error_DataStreamInvalidId, #varjo_Error_DataStreamAlreadyInUse
 *
 * @param session Varjo session handle.
 * @param id Stream to subscribe to. Stream ID can be found from #varjo_StreamConfig.
 * @param channels Channels to subscribe to. Available channels can be found from #varjo_StreamConfig.
 * @param callback Callback which receives the new frames.
 * @param userData Optional user controlled parameter that will be passed as-is as the second parameter for 'callback'.
 *                 Varjo stack does not access the data. Can be nullptr.
 */
VARJO_API void varjo_StartDataStream(
    struct varjo_Session* session, varjo_StreamId id, varjo_ChannelFlag channels, varjo_FrameListener* callback, void* userData);

/**
 * Stop frame streaming.
 *
 * There might be calls into the callback while executing this function, but after the function
 * has returned the callback is guaranteed not to be called anymore.
 *
 * Stopping the frame stream releases and invalidates all data related to the stream.
 * e.g. buffers that are currently locked will get implicitly released and should not be used after stopping.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamInvalidId, #varjo_Error_DataStreamNotInUse
 *
 * @param session Varjo session handle.
 * @param id Stream ID.
 */
VARJO_API void varjo_StopDataStream(struct varjo_Session* session, varjo_StreamId id);

/**
 * Get camera intrinsics for the current frame.
 *
 * Note: This data is only available for certain streams. The availability can be checked from
 *       #varjo_StreamFrame's dataFlags.
 * Note: It is only valid to call this function from the frame callback. Otherwise it will fail.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamInvalidId, #varjo_Error_DataStreamFrameExpired, #varjo_Error_DataStreamDataNotAvailable
 *
 * @param session Varjo session handle.
 * @param id Stream ID.
 * @param frameNumber Frame number that is specified in varjo_StreamFrame.
 * @param index Channel index for the data.
 * @return camera intrinsics for the frame or unspecified value if the frame does not have intrinsics.
 */
VARJO_API struct varjo_CameraIntrinsics varjo_GetCameraIntrinsics(
    struct varjo_Session* session, varjo_StreamId id, int64_t frameNumber, varjo_ChannelIndex index);

/**
 * Get camera extrinsics for the current frame.
 *
 * Note: This data is only available for certain streams. The availability can be checked from
 *       #varjo_StreamFrame's dataFlags.
 * Note: It is only valid to call this function from the frame callback. Otherwise it will fail.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamInvalidId, #varjo_Error_DataStreamFrameExpired, #varjo_Error_DataStreamDataNotAvailable
 *
 * @param session Varjo session handle.
 * @param id Stream ID.
 * @param frameNumber Frame number that is specified in varjo_StreamFrame.
 * @param index Channel index for the data.
 * @return camera extrinsics for the frame or unspecified value if the frame does not have extrinsics.
 */
VARJO_API struct varjo_Matrix varjo_GetCameraExtrinsics(struct varjo_Session* session, varjo_StreamId id, int64_t frameNumber, varjo_ChannelIndex index);

/**
 * Get bufferId for the current frame. This Id can be used to obtain metadata (#varjo_GetBufferMetadata),
 * CPU data buffer (#varjo_GetBufferCPUData) and GPU data buffer (#varjo_GetBufferGPUData).
 *
 * Note: This data is only available for certain streams. The availability can be checked from
 *       #varjo_StreamFrame's dataFlags.
 * Note: It is only valid to call this function from the frame callback. Otherwise it will fail.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamInvalidId, #varjo_Error_DataStreamFrameExpired, #varjo_Error_DataStreamDataNotAvailable
 *
 * @param session Varjo session handle.
 * @param id Stream ID.
 * @param frameNumber Frame number that is specified in varjo_StreamFrame.
 * @param index Channel index for the data.
 * @return buffer id for the frame or varjo_InvalidId value if the frame does not have a buffer.
 */
VARJO_API varjo_BufferId varjo_GetBufferId(struct varjo_Session* session, varjo_StreamId id, int64_t frameNumber, varjo_ChannelIndex index);

/**
 * Lock a data stream buffer for reading.
 *
 * The buffer must be locked from inside the frame callback.
 *
 * Prefer releasing the lock as fast as possible as new frames stop from arriving when a buffer
 * lock is being held. This applies as follows: as long as there is one or more (e.g. left or right channel)
 * buffers locked for a certain frame, no new frames for that particular stream will be provided in the callback.
 * In other words holding the lock for a long time will cause frames to drop.
 *
 * When the buffer is locked it can be consumed from any thread (e.g. render thread for GPU textures).
 *
 * Note: A locked buffer will become invalid after calling varjo_StopStream for the corresponding stream.
 *       In other words: the buffers do not outlive the streams lifetime.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamBufferAlreadyLocked, varjo_Error_DataStreamBufferInvalidId
 *
 * @param session Varjo session handle.
 * @param id Buffer ID.
 */
VARJO_API void varjo_LockDataStreamBuffer(struct varjo_Session* session, varjo_BufferId id);

/*
 * Unlock a previously locked data stream buffer.
 *
 * Unlock can be called from any thread (i.e. not necessarily from the frame callback thread).
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamBufferNotLocked, #varjo_Error_DataStreamBufferInvalidId
 *
 * @param session Varjo session handle.
 * @param id Buffer ID.
 */
VARJO_API void varjo_UnlockDataStreamBuffer(struct varjo_Session* session, varjo_BufferId id);

/**
 * Get buffer related metadata.
 *
 * The buffer must be locked (#varjo_LockDataStreamBuffer) before calling this function. After that it is
 * possible to call this function from any thread.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamBufferNotLocked, #varjo_Error_DataStreamBufferInvalidId
 *
 * @param session Varjo session handle.
 * @param id Buffer ID.
 * @return Buffer metadata or unspecified value in case the buffer does not exist.
 */
VARJO_API struct varjo_BufferMetadata varjo_GetBufferMetadata(struct varjo_Session* session, varjo_BufferId id);

/**
 * Get data buffer if it resides in the GPU memory (can be checked with #varjo_BufferMetadata::type).
 *
 * The buffer must be locked (#varjo_LockDataStreamBuffer) before calling this function. After that it is
 * possible to call this function from any thread.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamBufferNotLocked, #varjo_Error_DataStreamBufferInvalidId
 *
 * @param session Varjo session handle.
 * @param id Buffer ID.
 * @return GPU buffer handle or filled with zeroes in case the buffer does not exist.
 */
VARJO_API struct varjo_Texture varjo_GetBufferGPUData(struct varjo_Session* session, varjo_BufferId id);

/**
 * Get data buffer if it resides in the CPU memory (can be checked with #varjo_BufferMetadata::type).
 *
 * The buffer must be locked (#varjo_LockDataStreamBuffer) before calling this function. After that it is
 * possible to call this function from any thread.
 *
 * Possible errors:
 *     #varjo_Error_OperationFailed, #varjo_Error_NotAvailable, #varjo_Error_CapabilityNotAvailable,
 *     #varjo_Error_DataStreamBufferNotLocked, #varjo_Error_DataStreamBufferInvalidId
 *
 * @param session Varjo session handle.
 * @param id Buffer ID.
 * @return CPU buffer or nullptr if in case the buffer does not exist.
 */
VARJO_API void* varjo_GetBufferCPUData(struct varjo_Session* session, varjo_BufferId id);

#if defined __cplusplus
}
#endif
#endif  // VARJO_DATASTREAM_H
