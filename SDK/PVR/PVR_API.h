/************************************************************************************

Filename    :   PVR_API.h
Content     :   C Interface to Pimax VR devices.

Copyright   :   Copyright 2017 Pimax, Inc. All Rights reserved.
************************************************************************************/
#ifndef PVR_API_H
#define PVR_API_H

#include "PVR_Interface.h"
#include "windows.h"

typedef struct _pvrEnv
{
	pvrInterface* pvr_interface;
	void* pvr_dxgl_interface;
	HMODULE pvr_client_dll;
}pvrEnv;

typedef pvrEnv* pvrEnvHandle;

typedef struct _pvrSession
{
	pvrHmdHandle hmdh;
	pvrEnvHandle envh;
}pvrSession;

typedef pvrSession* pvrSessionHandle;

//NOTE: internal use, do not use this.
static PVR_FORCE_INLINE pvrResult __pvr_initInterface(pvrEnvHandle* pEnvHandle) {
	static pvrEnv env;
	if (env.pvr_interface == NULL) {
		HMODULE hDLL = LoadLibraryA(PVRCLIENT_DLL_NAME);
		if (hDLL == NULL) {
			return pvr_dll_failed;
		}
		getPvrInterface_Fn pGetInterface = (getPvrInterface_Fn)(void*)GetProcAddress(hDLL, PVR_GET_INTERFACE_FUNC_NAME);
		if (pGetInterface == NULL) {
			FreeLibrary(hDLL);
			return pvr_dll_wrong;
		}
		env.pvr_interface = pGetInterface(PVR_MAJOR_VERSION, PVR_MINOR_VERSION);
		if (NULL == env.pvr_interface) {
			FreeLibrary(hDLL);
			return pvr_interface_not_found;
		}
		env.pvr_client_dll = hDLL;
		*pEnvHandle = &env;
	}
	else {
		*pEnvHandle = &env;
	}
	return pvr_success;
}

//init PVR Environment.
static PVR_FORCE_INLINE pvrResult pvr_initialise(pvrEnvHandle* pEnvHandle) {
	pvrResult ret = __pvr_initInterface(pEnvHandle);
	if (ret != pvr_success) {
		return ret;
	}
	return (*pEnvHandle)->pvr_interface->initialise();
}

//shutdown PVR Environment.
static PVR_FORCE_INLINE void pvr_shutdown(pvrEnvHandle envHandle) {
	if (!envHandle) {
		return;
	}
	if (envHandle->pvr_interface)
		envHandle->pvr_interface->shutdown();
	if (envHandle->pvr_client_dll) {
		FreeLibrary(envHandle->pvr_client_dll);
		envHandle->pvr_client_dll = NULL;
	}
	envHandle->pvr_interface = NULL;
}

//get PVR SDK Version.
static PVR_FORCE_INLINE const char* pvr_getVersionString(pvrEnvHandle envHandle) {
	if (!envHandle) {
		return "";
	}
	return envHandle->pvr_interface->getVersionString();
}

//get current time in seconds.
static PVR_FORCE_INLINE double pvr_getTimeSeconds(pvrEnvHandle envHandle) {
	if (!envHandle) {
		return 0;
	}
	return envHandle->pvr_interface->getTimeSeconds();
}

//create a session to PVR Runtime.
//app can have multiple sessions.
static PVR_FORCE_INLINE pvrResult pvr_createSession(pvrEnvHandle envHandle, pvrSessionHandle* pSessionHandle) {
	if (!envHandle || !pSessionHandle) {
		return pvr_invalid_param;
	}
	*pSessionHandle = (pvrSessionHandle)malloc(sizeof(pvrSession));
	(*pSessionHandle)->envh = envHandle;
	pvrResult ret = envHandle->pvr_interface->createHmd(&((*pSessionHandle)->hmdh));
	if (ret != pvr_success) {
		free(*pSessionHandle);
		*pSessionHandle = 0;
	}
	return ret;
}
//destroy a session.
static PVR_FORCE_INLINE void pvr_destroySession(pvrSessionHandle sessionHandle) {
	if (!sessionHandle) {
		return;
	}
	sessionHandle->envh->pvr_interface->destroyHmd(sessionHandle->hmdh);
	free(sessionHandle);
}
//get render info(fov, eye offset) for eye
static PVR_FORCE_INLINE pvrResult pvr_getEyeRenderInfo(pvrSessionHandle sessionHandle, pvrEyeType eye, pvrEyeRenderInfo* outInfo){
	if (!sessionHandle || !outInfo) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getEyeRenderInfo(sessionHandle->hmdh, eye, outInfo);
}

//get tracking info for eye
static PVR_FORCE_INLINE pvrResult pvr_getEyeTrackingInfo(pvrSessionHandle sessionHandle, double absTime, pvrEyeTrackingInfo* outInfo) {
	if (!sessionHandle || !outInfo) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getEyeTrackingInfo(sessionHandle->hmdh, absTime, outInfo);
}

//get hmd descriptions.
static PVR_FORCE_INLINE pvrResult pvr_getHmdInfo(pvrSessionHandle sessionHandle, pvrHmdInfo* outInfo) {
	if (!sessionHandle || !outInfo) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getHmdInfo(sessionHandle->hmdh, outInfo);
}
//get display info for eye.
static PVR_FORCE_INLINE pvrResult pvr_getEyeDisplayInfo(pvrSessionHandle sessionHandle, pvrEyeType eye, pvrDisplayInfo* outInfo) {
	if (!sessionHandle || !outInfo) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getEyeDisplayInfo(sessionHandle->hmdh, eye, outInfo);
}

//get float config value, def_val will be used when failed to get real value.
static PVR_FORCE_INLINE float pvr_getFloatConfig(pvrSessionHandle sessionHandle, const char* key, float def_val) {
	if (!sessionHandle || !key) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getFloatConfig(sessionHandle->hmdh, key, def_val);
}

//set float config value.
static PVR_FORCE_INLINE pvrResult pvr_setFloatConfig(pvrSessionHandle sessionHandle, const char* key, float val) {
	if (!sessionHandle || !key) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->setFloatConfig(sessionHandle->hmdh, key, val);
}
//get int config value.
static PVR_FORCE_INLINE int pvr_getIntConfig(pvrSessionHandle sessionHandle, const char* key, int def_val) {
	if (!sessionHandle || !key) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getIntConfig(sessionHandle->hmdh, key, def_val);
}
//set int config value.
static PVR_FORCE_INLINE pvrResult pvr_setIntConfig(pvrSessionHandle sessionHandle, const char* key, int val) {
	if (!sessionHandle || !key) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->setIntConfig(sessionHandle->hmdh, key, val);
}
//get int64_t config value.
static PVR_FORCE_INLINE int64_t pvr_getInt64Config(pvrSessionHandle sessionHandle, const char* key, int64_t def_val) {
	if (!sessionHandle || !key) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getInt64Config(sessionHandle->hmdh, key, def_val);
}
//set int64_t config value.
static PVR_FORCE_INLINE pvrResult pvr_setInt64Config(pvrSessionHandle sessionHandle, const char* key, int64_t val) {
	if (!sessionHandle || !key) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->setInt64Config(sessionHandle->hmdh, key, val);
}
//set vector3f config value.
static PVR_FORCE_INLINE pvrResult pvr_setVector3fConfig(pvrSessionHandle sessionHandle, const char* key, pvrVector3f val) {
	if (!sessionHandle || !key) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->setVector3fConfig(sessionHandle->hmdh, key, val);
}
//get vector3f config value.
static PVR_FORCE_INLINE pvrVector3f pvr_getVector3fConfig(pvrSessionHandle sessionHandle, const char* key, pvrVector3f def_val) {
	if (!sessionHandle || !key) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getVector3fConfig(sessionHandle->hmdh, key, def_val);
}
//set vector3f config value.
static PVR_FORCE_INLINE pvrResult pvr_setQuatfConfig(pvrSessionHandle sessionHandle, const char* key, pvrQuatf val) {
	if (!sessionHandle || !key) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->setQuatfConfig(sessionHandle->hmdh, key, val);
}
//get vector3f config value.
static PVR_FORCE_INLINE pvrQuatf pvr_getQuatfConfig(pvrSessionHandle sessionHandle, const char* key, pvrQuatf def_val) {
	if (!sessionHandle || !key) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getQuatfConfig(sessionHandle->hmdh, key, def_val);
}
//get string config value.
static PVR_FORCE_INLINE int pvr_getStringConfig(pvrSessionHandle sessionHandle, const char* key, char* val, int size) {
	if (!sessionHandle || !key) {
		return 0;
	}
	return sessionHandle->envh->pvr_interface->getStringConfig(sessionHandle->hmdh, key, val, size);
}
//set string config value.
static PVR_FORCE_INLINE pvrResult pvr_setStringConfig(pvrSessionHandle sessionHandle, const char* key, const char* val) {
	if (!sessionHandle || !key) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->setStringConfig(sessionHandle->hmdh, key, val);
}
//get hmd current status.
//ServiceReady should be checked first, if true then check they others.
static PVR_FORCE_INLINE pvrResult pvr_getHmdStatus(pvrSessionHandle sessionHandle, pvrHmdStatus* outStatus) {
	if (!sessionHandle || !outStatus) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getHmdStatus(sessionHandle->hmdh, outStatus);
}

static PVR_FORCE_INLINE pvrResult pvr_getHmdDistortedUV(pvrSessionHandle sessionHandle, pvrEyeType eye, pvrVector2f uv, pvrVector2f outUV[3]) {
	if (!sessionHandle) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getHmdDistortedUV(sessionHandle->hmdh, eye, uv, outUV);
}
//set the tracking origin type.
static PVR_FORCE_INLINE pvrResult pvr_setTrackingOriginType(pvrSessionHandle sessionHandle, pvrTrackingOrigin origin) {
	if (!sessionHandle) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->setTrackingOriginType(sessionHandle->hmdh, origin);
}

//get the tracking origin type.
static PVR_FORCE_INLINE pvrResult pvr_getTrackingOriginType(pvrSessionHandle sessionHandle, pvrTrackingOrigin* origin) {
	if (!sessionHandle || !origin) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getTrackingOriginType(sessionHandle->hmdh, origin);
}

//recenter the orientation and position.
static PVR_FORCE_INLINE pvrResult pvr_recenterTrackingOrigin(pvrSessionHandle sessionHandle) {
	if (!sessionHandle) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->recenterTrackingOrigin(sessionHandle->hmdh);
}

//get pose of tracked devices(head, hands), should check StatusFlags to see if the oritation or position is valid.
static PVR_FORCE_INLINE pvrResult pvr_getTrackingState(pvrSessionHandle sessionHandle, double absTime, pvrTrackingState* state) {
	if (!sessionHandle || !state) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getTrackingState(sessionHandle->hmdh, absTime, state);
}
//get pose state of a tracked device, should check StatusFlags to see if the oritation or position is valid.
static PVR_FORCE_INLINE pvrResult pvr_getTrackedDevicePoseState(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, double absTime, pvrPoseStatef* state) {
	if (!sessionHandle || !state) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getTrackedDevicePoseState(sessionHandle->hmdh, device, absTime, state);
}

//get current connected devices.
static PVR_FORCE_INLINE pvrResult pvr_getConnectedDevices(pvrSessionHandle sessionHandle, uint32_t* pDevices) {
	if (!sessionHandle || !pDevices) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getConnectedDevices(sessionHandle->hmdh, pDevices);
}

//get device float type property.
static PVR_FORCE_INLINE float pvr_getTrackedDeviceFloatProperty(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, float def_val) {
	if (!sessionHandle) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getTrackedDeviceFloatProperty(sessionHandle->hmdh, device, prop, def_val);
}
//get device int type property.
static PVR_FORCE_INLINE int pvr_getTrackedDeviceIntProperty(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, int def_val) {
	if (!sessionHandle) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getTrackedDeviceIntProperty(sessionHandle->hmdh, device, prop, def_val);
}
//get device int64 type property.
static PVR_FORCE_INLINE int64_t pvr_getTrackedDeviceInt64Property(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, int64_t def_val) {
	if (!sessionHandle) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getTrackedDeviceInt64Property(sessionHandle->hmdh, device, prop, def_val);
}
//get device string type property.
static PVR_FORCE_INLINE int pvr_getTrackedDeviceStringProperty(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, char* val, int size) {
	if (!sessionHandle) {
		return -1;
	}
	return sessionHandle->envh->pvr_interface->getTrackedDeviceStringProperty(sessionHandle->hmdh, device, prop, val, size);
}
//get device vector3f type property.
static PVR_FORCE_INLINE pvrVector3f pvr_getTrackedDeviceVector3fProperty(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, pvrVector3f def_val) {
	if (!sessionHandle) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getTrackedDeviceVector3fProperty(sessionHandle->hmdh, device, prop, def_val);
}
//get device quatf type property.
static PVR_FORCE_INLINE pvrQuatf pvr_getTrackedDeviceQuatfProperty(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, pvrTrackedDeviceProp prop, pvrQuatf def_val) {
	if (!sessionHandle) {
		return def_val;
	}
	return sessionHandle->envh->pvr_interface->getTrackedDeviceQuatfProperty(sessionHandle->hmdh, device, prop, def_val);
}

//get the trackers count.
static PVR_FORCE_INLINE unsigned int pvr_getTrackerCount(pvrSessionHandle sessionHandle) {
	if (!sessionHandle) {
		return 0;
	}
	return sessionHandle->envh->pvr_interface->getTrackerCount(sessionHandle->hmdh);
}
//get the description of tracker.
static PVR_FORCE_INLINE pvrResult pvr_getTrackerDesc(pvrSessionHandle sessionHandle, unsigned int idx, pvrTrackerDesc* desc) {
	if (!sessionHandle || !desc) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getTrackerDesc(sessionHandle->hmdh, idx, desc);
}
//get the pose of the tracker.
static PVR_FORCE_INLINE pvrResult pvr_getTrackerPose(pvrSessionHandle sessionHandle, unsigned int idx, pvrTrackerPose* pose) {
	if (!sessionHandle || !pose) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getTrackerPose(sessionHandle->hmdh, idx, pose);
}
//get the input controller state, 
//should use pvrButton_XXX&HandButtons[hand] to check whether the button is pressing.
static PVR_FORCE_INLINE pvrResult pvr_getInputState(pvrSessionHandle sessionHandle, pvrInputState* inputState) {
	if (!sessionHandle || !inputState) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getInputState(sessionHandle->hmdh, inputState);
}

//trigger haptic pulse for device, amplitude (range from 0.0 to 1.0), frequency not used, must set to 0, reserve for future use.
static PVR_FORCE_INLINE pvrResult pvr_triggerHapticPulse(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, float amplitude, float durationSeconds, float frequency) {
	if (!sessionHandle || amplitude < 0 || durationSeconds < 0 || frequency < 0) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->triggerHapticPulse(sessionHandle->hmdh, device, amplitude, durationSeconds, frequency);
}

static PVR_FORCE_INLINE pvrResult pvr_getSkeletalData(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, pvrSkeletalMotionRange range, pvrSkeletalData* data) {
	if (!sessionHandle || !data) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getSkeletalData(sessionHandle->hmdh, device, range, data);
}

static PVR_FORCE_INLINE pvrResult pvr_getGripLimitSkeletalData(pvrSessionHandle sessionHandle, pvrTrackedDeviceType device, pvrSkeletalData* data)
{
	if (!sessionHandle || !data) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getGripLimitSkeletalData(sessionHandle->hmdh, device, data);
}

static PVR_FORCE_INLINE pvrDispStateType pvr_getDisplayState(pvrEnvHandle envHandle, uint32_t edid_vid, uint32_t edid_pid) {
	if (!envHandle) {
		return pvrDispStateType::pvrDispState_None;
	}
	return envHandle->pvr_interface->getDisplayState(edid_vid, edid_pid);
}

//get number of textures in the pvrTextureSwapChain.
static PVR_FORCE_INLINE pvrResult pvr_getTextureSwapChainLength(pvrSessionHandle sessionHandle, pvrTextureSwapChain chain, int* out_Length) {
	if (!sessionHandle || !out_Length || !chain) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getTextureSwapChainLength(sessionHandle->hmdh, chain, out_Length);
}

//get index of current using texture.
static PVR_FORCE_INLINE pvrResult pvr_getTextureSwapChainCurrentIndex(pvrSessionHandle sessionHandle, pvrTextureSwapChain chain, int* out_Index) {
	if (!sessionHandle || !out_Index || !chain) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getTextureSwapChainCurrentIndex(sessionHandle->hmdh, chain, out_Index);
}
//get description of the pvrTextureSwapChain.
static PVR_FORCE_INLINE pvrResult pvr_getTextureSwapChainDesc(pvrSessionHandle sessionHandle, pvrTextureSwapChain chain, pvrTextureSwapChainDesc* out_Desc) {
	if (!sessionHandle || !out_Desc) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getTextureSwapChainDesc(sessionHandle->hmdh, chain, out_Desc);
}
//commit the current texture of pvrTextrueSwapChain,
//the current index will be changed after this call.
static PVR_FORCE_INLINE pvrResult pvr_commitTextureSwapChain(pvrSessionHandle sessionHandle, pvrTextureSwapChain chain) {
	if (!sessionHandle || !chain) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->commitTextureSwapChain(sessionHandle->hmdh, chain);
}
//destroy the pvrTextureSwapChain.
static PVR_FORCE_INLINE void pvr_destroyTextureSwapChain(pvrSessionHandle sessionHandle, pvrTextureSwapChain chain) {
	if (!sessionHandle || !chain) {
		return;
	}
	sessionHandle->envh->pvr_interface->destroyTextureSwapChain(sessionHandle->hmdh, chain);
}
//destroy the pvrMirrorTexture.
static PVR_FORCE_INLINE void pvr_destroyMirrorTexture(pvrSessionHandle sessionHandle, pvrMirrorTexture mirrorTexture) {
	if (!sessionHandle || !mirrorTexture) {
		return;
	}
	sessionHandle->envh->pvr_interface->destroyMirrorTexture(sessionHandle->hmdh, mirrorTexture);
}

//deprecated.
//submit rendered layers to PVR Runtime, to be showned on the HMD.
static PVR_FORCE_INLINE pvrResult pvr_submitFrame(pvrSessionHandle sessionHandle, long long frameIndex,
	pvrLayerHeader const * const * layerPtrList, unsigned int layerCount) {
	if (!sessionHandle || !layerPtrList) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->submitFrame(sessionHandle->hmdh, frameIndex, layerPtrList, layerCount);
}

//submit rendered layers to PVR Runtime, to be showned on the HMD.
static PVR_FORCE_INLINE pvrResult pvr_endFrame(pvrSessionHandle sessionHandle, long long frameIndex,
	pvrLayerHeader const * const * layerPtrList, unsigned int layerCount) {
	if (!sessionHandle || !layerPtrList) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->endFrame(sessionHandle->hmdh, frameIndex, layerPtrList, layerCount);
}
//submit rendered layers to PVR Runtime, to be showned on the HMD.
static PVR_FORCE_INLINE pvrResult pvr_beginFrame(pvrSessionHandle sessionHandle, long long frameIndex) {
	if (!sessionHandle) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->beginFrame(sessionHandle->hmdh, frameIndex);
}
//wait to begin frame, must be call before beginFrame and endFrame.
static PVR_FORCE_INLINE pvrResult pvr_waitToBeginFrame(pvrSessionHandle sessionHandle, long long frameIndex) {
	if (!sessionHandle) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->waitToBeginFrame(sessionHandle->hmdh, frameIndex);
}
//get the time the frame(indicate by frameIndex) will be show on the display.
//for low latency, use this time to get the tracking state.
static PVR_FORCE_INLINE double pvr_getPredictedDisplayTime(pvrSessionHandle sessionHandle, long long frameIndex) {
	if (!sessionHandle) {
		return 0;
	}
	return sessionHandle->envh->pvr_interface->getPredictedDisplayTime(sessionHandle->hmdh, frameIndex);
}
//get recommended texture size of each eye.
static PVR_FORCE_INLINE pvrResult pvr_getFovTextureSize(pvrSessionHandle sessionHandle, pvrEyeType eye, pvrFovPort fov, float pixelsPerDisplayPixel, pvrSizei* size) {
	if (!sessionHandle || !size) {
		return pvr_invalid_param;
	}
	return sessionHandle->envh->pvr_interface->getFovTextureSize(sessionHandle->hmdh, eye, fov, pixelsPerDisplayPixel, size);
}

//get eye hidden area mesh.
//return the request vertex count.
static PVR_FORCE_INLINE unsigned int pvr_getEyeHiddenAreaMesh(pvrSessionHandle sessionHandle, pvrEyeType eye, pvrVector2f* outVertexBuffer, unsigned int bufferCount) {
	if (!sessionHandle) {
		return 0;
	}
	return sessionHandle->envh->pvr_interface->getEyeHiddenAreaMesh(sessionHandle->hmdh, eye, outVertexBuffer, bufferCount);
}

//calculate eye pose from head pose and eye offset.
static PVR_FORCE_INLINE void pvr_calcEyePoses(pvrEnvHandle envHandle, pvrPosef headPose, const pvrPosef hmdToEyePose[2], pvrPosef outEyePoses[2]) {
	if (!envHandle) {
		return;
	}
	envHandle->pvr_interface->calcEyePoses(headPose, hmdToEyePose, outEyePoses);
}
//get projection matrix from the fov.
static PVR_FORCE_INLINE void pvrMatrix4f_Projection(pvrEnvHandle envHandle, pvrFovPort fov, float znear, float zfar, pvrBool right_handled, pvrMatrix4f* outMat) {
	if (!envHandle || !outMat) {
		return;
	}
	return envHandle->pvr_interface->Matrix4f_Projection(fov, znear, zfar, right_handled, outMat);
}

static PVR_FORCE_INLINE void pvrMatrix4f_OrthoSubProjection(pvrEnvHandle envHandle, pvrMatrix4f projection, pvrVector2f orthoScale, float orthoDistance, float hmdToEyeOffsetX, pvrMatrix4f* outMat) {
	if (!envHandle || !outMat) {
		return;
	}
	return envHandle->pvr_interface->Matrix4f_OrthoSubProjection(projection, orthoScale, orthoDistance, hmdToEyeOffsetX, outMat);
}

static PVR_FORCE_INLINE void pvrPosef_FlipHandedness(pvrEnvHandle envHandle, const pvrPosef* inPose, pvrPosef* outPose) {
	if (!envHandle || outPose) {
		return;
	}
	return envHandle->pvr_interface->Posef_FlipHandedness(inPose, outPose);
}

//get client count, including current clent.
static PVR_FORCE_INLINE int pvr_getClientCount(pvrEnvHandle envHandle) {
	if (!envHandle) {
		return 0;
	}
	return envHandle->pvr_interface->getClientCount();
}

//get client pids, including current clent. return client counts.
static PVR_FORCE_INLINE int pvr_getClientPids(pvrEnvHandle envHandle, uint32_t pids[], int buf_count) {
	if (!envHandle) {
		return 0;
	}
	return envHandle->pvr_interface->getClientPids(pids, buf_count);
}
//log message.
static PVR_FORCE_INLINE void pvr_logMessage(pvrEnvHandle envHandle, pvrLogLevel level, const char* message) {
	if (!envHandle) {
		return;
	}
	return envHandle->pvr_interface->logMessage(level, message);
}

#endif
