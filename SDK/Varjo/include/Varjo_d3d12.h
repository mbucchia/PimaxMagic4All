// Copyright 2020-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_D3D12_H
#define VARJO_D3D12_H

#include <d3d12.h>

#include "Varjo.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Converts a Direct3D 12 texture to #varjo_Texture.
 */
VARJO_API struct varjo_Texture varjo_FromD3D12Texture(struct ID3D12Resource* texture);

/**
 * Converts #varjo_Texture to Direct3D 12 texture.
 */
VARJO_API struct ID3D12Resource* varjo_ToD3D12Texture(struct varjo_Texture texture);

#if defined __cplusplus
}
#endif

#endif