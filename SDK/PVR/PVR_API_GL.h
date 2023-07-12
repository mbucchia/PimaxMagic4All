/************************************************************************************

Filename    :   PVR_API_D3D.h
Content     :   OPENGL related support for PVR SDK.

Copyright   :   Copyright 2017 Pimax, Inc. All Rights reserved.
************************************************************************************/
#ifndef PVR_API_GL_H
#define PVR_API_GL_H

#include "PVR_API.h"
#include "PVR_Interface_GL.h"

//NOTE: internal use, do not use this.
static PVR_FORCE_INLINE pvrResult __pvr_initGLInterface(pvrSessionHandle sessionHandle)
{
	if (sessionHandle->envh->pvr_dxgl_interface == nullptr) {
		sessionHandle->envh->pvr_dxgl_interface = sessionHandle->envh->pvr_interface->getDxGlInterface("gl");
	}
	if (sessionHandle->envh->pvr_dxgl_interface == nullptr) {
		return pvr_interface_not_found;
	}
	return pvr_success;
}

#define GL_INTERFACE(sessionHandle) ((pvrGLInterface*)sessionHandle->envh->pvr_dxgl_interface)

// create pvrTextureSwapChain for OPENGL, app should render scenes to pvrTextureSwapChain, and submit to PVR HMD.
static PVR_FORCE_INLINE pvrResult pvr_createTextureSwapChainGL(pvrSessionHandle sessionHandle,
	const pvrTextureSwapChainDesc* desc,
	pvrTextureSwapChain* out_TextureSwapChain) {
	if (!sessionHandle || !desc || !out_TextureSwapChain) {
		return pvr_invalid_param;
	}
	auto ret = __pvr_initGLInterface(sessionHandle);
	if (ret != pvr_success) {
		return ret;
	}
	return GL_INTERFACE(sessionHandle)->createTextureSwapChainGL(sessionHandle->hmdh, desc, out_TextureSwapChain);
}

// get the gl tex id of index from pvrTextureSwapChain
static PVR_FORCE_INLINE pvrResult pvr_getTextureSwapChainBufferGL(pvrSessionHandle sessionHandle,
	pvrTextureSwapChain chain,
	int index,
	unsigned int* out_TexId) {
	if (!sessionHandle || !out_TexId || !chain || index < 0) {
		return pvr_invalid_param;
	}
	return GL_INTERFACE(sessionHandle)->getTextureSwapChainBufferGL(sessionHandle->hmdh, chain, index, out_TexId);
}

// create pvrMirrorTexture for app to show on the window.
static PVR_FORCE_INLINE pvrResult pvr_createMirrorTextureGL(pvrSessionHandle sessionHandle,
	const pvrMirrorTextureDesc* desc,
	pvrMirrorTexture* out_MirrorTexture) {
	if (!sessionHandle || !out_MirrorTexture || !desc) {
		return pvr_invalid_param;
	}
	auto ret = __pvr_initGLInterface(sessionHandle);
	if (ret != pvr_success) {
		return ret;
	}
	return GL_INTERFACE(sessionHandle)->createMirrorTextureGL(sessionHandle->hmdh, desc, out_MirrorTexture);
}

// get gl tex id for pvrMirrorTexture.
static PVR_FORCE_INLINE pvrResult pvr_getMirrorTextureBufferGL(pvrSessionHandle sessionHandle,
	pvrMirrorTexture mirrorTexture,
	unsigned int* out_TexId) {
	if (!sessionHandle || !mirrorTexture || !out_TexId) {
		return pvr_invalid_param;
	}
	return GL_INTERFACE(sessionHandle)->getMirrorTextureBufferGL(sessionHandle->hmdh, mirrorTexture, out_TexId);
}

#endif
