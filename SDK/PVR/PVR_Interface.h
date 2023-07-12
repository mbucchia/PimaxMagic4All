/************************************************************************************

Filename    :   PVR_Interface.h
Content     :   Interface definitions for PVR SDK.

Copyright   :   Copyright 2017 Pimax, Inc. All Rights reserved.
************************************************************************************/
#ifndef PVR_INTERFACE_H
#define PVR_INTERFACE_H
#include "PVR_Types.h"
#include "PVRVersion.h"

typedef struct _pvrInterfaceV24
{
	pvrResult (*initialise)();
	void(*shutdown)();
	pvrResult(*createHmd)(pvrHmdHandle* phmdh);
	void (*destroyHmd)(pvrHmdHandle hmdh);
	const char* (*getVersionString)();
	double(*getTimeSeconds)();
	pvrResult(*getHmdInfo)(pvrHmdHandle hmdh, pvrHmdInfo* outInfo);
	pvrResult(*getEyeDisplayInfo)(pvrHmdHandle hmdh, pvrEyeType eye, pvrDisplayInfo* outInfo);
	pvrResult(*getEyeRenderInfo)(pvrHmdHandle hmdh, pvrEyeType eye, pvrEyeRenderInfo* outInfo);
	pvrResult(*getHmdStatus)(pvrHmdHandle hmdh, pvrHmdStatus* outStatus);

	pvrResult(*setTrackingOriginType)(pvrHmdHandle hmdh, pvrTrackingOrigin origin);
	pvrResult(*getTrackingOriginType)(pvrHmdHandle hmdh, pvrTrackingOrigin* origin);
	pvrResult(*recenterTrackingOrigin)(pvrHmdHandle hmdh);
	pvrResult(*getTrackingState)(pvrHmdHandle hmdh, double absTime, pvrTrackingState* state);
	pvrResult(*getTrackedDeviceCaps)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, uint32_t* pcap);

	pvrResult(*getInputState)(pvrHmdHandle hmdh, pvrInputState* inputState);

	float (*getFloatConfig)(pvrHmdHandle hmdh, const char* key, float def_val);
	pvrResult (*setFloatConfig)(pvrHmdHandle hmdh, const char* key, float val);
	int(*getIntConfig)(pvrHmdHandle hmdh, const char* key, int def_val);
	pvrResult(*setIntConfig)(pvrHmdHandle hmdh, const char* key, int val);
	int(*getStringConfig)(pvrHmdHandle hmdh, const char* key, char* val, int size);
	pvrResult(*setStringConfig)(pvrHmdHandle hmdh, const char* key, const char* val);

	double(*getPredictedDisplayTime)(pvrHmdHandle hmdh, long long frameIndex);
	pvrResult(*getFovTextureSize)(pvrHmdHandle hmdh, pvrEyeType eye, pvrFovPort fov, float pixelsPerDisplayPixel, pvrSizei* size);

	pvrResult(*getHmdDistortedUV)(pvrHmdHandle hmdh, pvrEyeType eye, pvrVector2f uv, pvrVector2f outUV[3]);

	pvrResult(*getTextureSwapChainLength)(pvrHmdHandle hmdh, pvrTextureSwapChain chain, int* out_Length);
	pvrResult(*getTextureSwapChainCurrentIndex)(pvrHmdHandle hmdh, pvrTextureSwapChain chain, int* out_Index);
	pvrResult(*getTextureSwapChainDesc)(pvrHmdHandle hmdh, pvrTextureSwapChain chain, pvrTextureSwapChainDesc* out_Desc);
	pvrResult(*commitTextureSwapChain)(pvrHmdHandle hmdh, pvrTextureSwapChain chain);
	void(*destroyTextureSwapChain)(pvrHmdHandle hmdh, pvrTextureSwapChain chain);
	void(*destroyMirrorTexture)(pvrHmdHandle hmdh, pvrMirrorTexture mirrorTexture);
	pvrResult(*endFrame)(pvrHmdHandle hmdh, long long frameIndex,
		pvrLayerHeader const * const * layerPtrList, unsigned int layerCount);
	pvrResult(*beginFrame)(pvrHmdHandle hmdh, long long frameIndex);
	pvrResult(*waitToBeginFrame)(pvrHmdHandle hmdh, long long frameIndex);
	pvrResult(*submitFrame)(pvrHmdHandle hmdh, long long frameIndex, pvrLayerHeader const * const * layerPtrList, unsigned int layerCount);

	void* (*getDxGlInterface)(const char* api);

	void (*Matrix4f_Projection)(pvrFovPort fov, float znear, float zfar, pvrBool right_handled, pvrMatrix4f* outMat);
	void (*Matrix4f_OrthoSubProjection)(pvrMatrix4f projection, pvrVector2f orthoScale, float orthoDistance, float hmdToEyeOffsetX, pvrMatrix4f* outMat);
	void(*calcEyePoses)(pvrPosef headPose, const pvrPosef hmdToEyePose[2], pvrPosef outEyePoses[2]);
	void(*Posef_FlipHandedness)(const pvrPosef* inPose, pvrPosef* outPose);

	pvrDispStateType(*getDisplayState)(uint32_t edid_vid, uint32_t edid_pid);
	int(*getClientCount)();
	int(*getClientPids)(uint32_t pids[], int buf_count);
	void (*logMessage)(pvrLogLevel level, const char* message);

	unsigned int (*getTrackerCount)(pvrHmdHandle hmdh);
	pvrResult (*getTrackerDesc)(pvrHmdHandle hmdh, unsigned int idx, pvrTrackerDesc* desc);
	pvrResult (*getTrackerPose)(pvrHmdHandle hmdh, unsigned int idx, pvrTrackerPose* pose);

	pvrResult (*triggerHapticPulse)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, float amplitude, float durationSeconds, float frequency);

	pvrVector3f(*getVector3fConfig)(pvrHmdHandle hmdh, const char* key, pvrVector3f def_val);
	pvrResult(*setVector3fConfig)(pvrHmdHandle hmdh, const char* key, pvrVector3f val);
	pvrQuatf(*getQuatfConfig)(pvrHmdHandle hmdh, const char* key, pvrQuatf def_val);
	pvrResult(*setQuatfConfig)(pvrHmdHandle hmdh, const char* key, pvrQuatf val);

	pvrResult(*getConnectedDevices)(pvrHmdHandle hmdh, uint32_t* pDevices);
	pvrResult(*getTrackedDevicePoseState)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, double absTime, pvrPoseStatef* state);
	float(*getTrackedDeviceFloatProperty)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, float def_val);
	int(*getTrackedDeviceIntProperty)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, int def_val);
	int(*getTrackedDeviceStringProperty)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, char* val, int size);
	pvrVector3f(*getTrackedDeviceVector3fProperty)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, pvrVector3f def_val);
	pvrQuatf(*getTrackedDeviceQuatfProperty)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, pvrQuatf def_val);

	unsigned int(*getEyeHiddenAreaMesh)(pvrHmdHandle hmdh, pvrEyeType eye, pvrVector2f* outVertexBuffer, unsigned int bufferCount);

	int64_t(*getInt64Config)(pvrHmdHandle hmdh, const char* key, int64_t def_val);
	pvrResult(*setInt64Config)(pvrHmdHandle hmdh, const char* key, int64_t val);

	pvrResult (*getSkeletalData)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrSkeletalMotionRange range, pvrSkeletalData* data);
	pvrResult (*getGripLimitSkeletalData)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrSkeletalData* data);

	int64_t(*getTrackedDeviceInt64Property)(pvrHmdHandle hmdh, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, int64_t def_val);

	pvrResult(*getEyeTrackingInfo)(pvrHmdHandle hmdh, double absTime, pvrEyeTrackingInfo* outInfo);

}pvrInterfaceV24;

typedef pvrInterfaceV24 pvrInterface;

typedef pvrInterface* (*getPvrInterface_Fn)(uint32_t major_ver, uint32_t minor_ver);

#if PVR_PTR_SIZE == 8
#define PVRCLIENT_DLL_NAME "libPVRClient64.dll"
#else
#define PVRCLIENT_DLL_NAME "libPVRClient32.dll"
#endif

#define PVR_GET_INTERFACE_FUNC_NAME "getPvrInterface"

#endif
