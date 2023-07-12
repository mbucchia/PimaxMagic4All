/************************************************************************************

Filename    :   PVR_API_D3D.h
Content     :   d3d related support for PVR SDK.

Copyright   :   Copyright 2017 Pimax, Inc. All Rights reserved.
************************************************************************************/
#ifndef PVR_API_D3D_H
#define PVR_API_D3D_H

#include "PVR_API.h"
#include "PVR_Interface_D3D.h"

//NOTE: internal use, do not use this.
static PVR_FORCE_INLINE pvrResult __pvr_initD3DInterface(pvrSessionHandle sessionHandle)
{
	if (sessionHandle->envh->pvr_dxgl_interface == nullptr) {
		sessionHandle->envh->pvr_dxgl_interface = sessionHandle->envh->pvr_interface->getDxGlInterface("dx");
	}
	if (sessionHandle->envh->pvr_dxgl_interface == nullptr) {
		return pvr_interface_not_found;
	}
	return pvr_success;
}

#define D3D_INTERFACE(sessionHandle) ((pvrD3DInterface*)sessionHandle->envh->pvr_dxgl_interface)

// create pvrTextureSwapChain for D3D, app should render scenes to pvrTextureSwapChain, and submit to PVR HMD.
// currently only d3d11 supported.
static PVR_FORCE_INLINE pvrResult pvr_createTextureSwapChainDX(pvrSessionHandle sessionHandle,
	IUnknown* d3dPtr,
	const pvrTextureSwapChainDesc* desc,
	pvrTextureSwapChain* out_TextureSwapChain) {
	if (!sessionHandle || !d3dPtr || !desc || !out_TextureSwapChain) {
		return pvr_invalid_param;
	}
	auto ret = __pvr_initD3DInterface(sessionHandle);
	if (ret != pvr_success) {
		return ret;
	}
	return D3D_INTERFACE(sessionHandle)->createTextureSwapChainDX(sessionHandle->hmdh, d3dPtr, desc, out_TextureSwapChain);
}

// get the d3d texture resource of index from pvrTextureSwapChain
static PVR_FORCE_INLINE pvrResult pvr_getTextureSwapChainBufferDX(pvrSessionHandle sessionHandle,
	pvrTextureSwapChain chain,
	int index,
	IID iid,
	void** out_Buffer) {
	if (!sessionHandle || !chain || !out_Buffer || index < 0) {
		return pvr_invalid_param;
	}
	return D3D_INTERFACE(sessionHandle)->getTextureSwapChainBufferDX(sessionHandle->hmdh, chain, index, iid, out_Buffer);
}

// create pvrMirrorTexture for app to show on the window.
static PVR_FORCE_INLINE pvrResult pvr_createMirrorTextureDX(pvrSessionHandle sessionHandle,
	IUnknown* d3dPtr,
	const pvrMirrorTextureDesc* desc,
	pvrMirrorTexture* out_MirrorTexture) {
	if (!sessionHandle || !d3dPtr || !desc || !out_MirrorTexture) {
		return pvr_invalid_param;
	}
	auto ret = __pvr_initD3DInterface(sessionHandle);
	if (ret != pvr_success) {
		return ret;
	}
	return D3D_INTERFACE(sessionHandle)->createMirrorTextureDX(sessionHandle->hmdh, d3dPtr, desc, out_MirrorTexture);
}

// get d3d texture resource for pvrMirrorTexture.
static PVR_FORCE_INLINE pvrResult pvr_getMirrorTextureBufferDX(pvrSessionHandle sessionHandle,
	pvrMirrorTexture mirrorTexture,
	IID iid,
	void** out_Buffer) {
	if (!sessionHandle || !mirrorTexture || !out_Buffer) {
		return pvr_invalid_param;
	}
	return D3D_INTERFACE(sessionHandle)->getMirrorTextureBufferDX(sessionHandle->hmdh, mirrorTexture, iid, out_Buffer);
}

#endif
