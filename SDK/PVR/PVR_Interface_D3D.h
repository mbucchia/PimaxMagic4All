/************************************************************************************

Filename    :   PVR_Interface_D3D.h
Content     :   d3d related Interface definitions for PVR SDK.

Copyright   :   Copyright 2017 Pimax, Inc. All Rights reserved.
************************************************************************************/
#ifndef PVR_D3D_INTERFACE_H
#define PVR_D3D_INTERFACE_H
#include "PVR_Types.h"
#include "PVRVersion.h"
#include "unknwn.h"

typedef struct _pvrD3DInterfaceV2
{
	pvrResult(*createTextureSwapChainDX)(pvrHmdHandle hmdh,
		IUnknown* d3dPtr,
		const pvrTextureSwapChainDesc* desc,
		pvrTextureSwapChain* out_TextureSwapChain);
	pvrResult(*getTextureSwapChainBufferDX)(pvrHmdHandle hmdh,
		pvrTextureSwapChain chain,
		int index,
		IID iid,
		void** out_Buffer);
	pvrResult(*createMirrorTextureDX)(pvrHmdHandle hmdh,
		IUnknown* d3dPtr,
		const pvrMirrorTextureDesc* desc,
		pvrMirrorTexture* out_MirrorTexture);
	pvrResult(*getMirrorTextureBufferDX)(pvrHmdHandle hmdh,
		pvrMirrorTexture mirrorTexture,
		IID iid,
		void** out_Buffer);
}pvrD3DInterfaceV2;

typedef pvrD3DInterfaceV2 pvrD3DInterface;

#endif
