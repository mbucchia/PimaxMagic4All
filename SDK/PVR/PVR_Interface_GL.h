/************************************************************************************

Filename    :   PVR_Interface_D3D.h
Content     :   d3d related Interface definitions for PVR SDK.

Copyright   :   Copyright 2017 Pimax, Inc. All Rights reserved.
************************************************************************************/
#ifndef PVR_GL_INTERFACE_H
#define PVR_GL_INTERFACE_H
#include "PVR_Types.h"
#include "PVRVersion.h"

typedef struct _pvrGLInterfaceV2
{
	pvrResult(*createTextureSwapChainGL)(pvrHmdHandle hmdh,
		const pvrTextureSwapChainDesc* desc,
		pvrTextureSwapChain* out_TextureSwapChain);
	pvrResult(*getTextureSwapChainBufferGL)(pvrHmdHandle hmdh,
		pvrTextureSwapChain chain,
		int index,
		unsigned int* out_TexId);
	pvrResult(*createMirrorTextureGL)(pvrHmdHandle hmdh,
		const pvrMirrorTextureDesc* desc,
		pvrMirrorTexture* out_MirrorTexture);
	pvrResult(*getMirrorTextureBufferGL)(pvrHmdHandle hmdh,
		pvrMirrorTexture mirrorTexture,
		unsigned int* out_TexId);
}pvrGLInterfaceV2;

typedef pvrGLInterfaceV2 pvrGLInterface;

#endif
