// Copyright 2019-2023 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_D3D11_H
#define VARJO_D3D11_H

#include "Varjo.h"
#include "Varjo_types_d3d11.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Converts a Direct3D 11 texture to #varjo_Texture.
 */
VARJO_API struct varjo_Texture varjo_FromD3D11Texture(struct ID3D11Texture2D* texture);

/**
 * Converts #varjo_Texture to Direct3D 11 texture.
 * This is a deprecated version of #varjo_ToD3D11Texture.
 */
VARJO_DEPRECATED_API struct ID3D11Texture2D* varjo_ToD3D11texture(struct varjo_Texture texture);

/**
 * Converts #varjo_Texture to Direct3D 11 texture.
 */
VARJO_API struct ID3D11Texture2D* varjo_ToD3D11Texture(struct varjo_Texture texture);

/**
 * Retrieves LUID of the device which is used by compositor.
 *
 * Application has to use the same device to work correctly.
 *
 * @param session Varjo session handle.
 * @return LUID of DirectX device which compositor is using.
 */
VARJO_API struct varjo_Luid varjo_D3D11GetLuid(struct varjo_Session* session);

#if defined __cplusplus
}
#endif

#if defined DIRECTX_MATH_VERSION
/**
 * Convert Varjo double matrix to DirectX XMMATRIX.
 */
static DirectX::XMMATRIX varjo_DoubleArrayToXMMatrix(double* m)
{
    return DirectX::XMMATRIX{
        (float)m[0],
        (float)m[1],
        (float)m[2],
        (float)m[3],
        (float)m[4],
        (float)m[5],
        (float)m[6],
        (float)m[7],
        (float)m[8],
        (float)m[9],
        (float)m[10],
        (float)m[11],
        (float)m[12],
        (float)m[13],
        (float)m[14],
        (float)m[15],
    };
}

/**
 * Convert Varjo 3x3 double matrix to DirectX XMMATRIX.
 */
static DirectX::XMMATRIX varjo_DoubleArray3x3ToXMMatrix(double* m)
{
    return DirectX::XMMATRIX{
        (float)m[0],
        (float)m[1],
        (float)m[2],
        0.0f,
        (float)m[3],
        (float)m[4],
        (float)m[5],
        0.0f,
        (float)m[6],
        (float)m[7],
        (float)m[8],
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
    };
}
#endif

#endif
