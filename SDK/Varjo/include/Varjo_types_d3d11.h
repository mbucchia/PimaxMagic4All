// Copyright 2019 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_TYPES_D3D11_H
#define VARJO_TYPES_D3D11_H

#include <d3d11.h>
#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

/**
 * Device Luid.
 */
struct varjo_Luid {
    uint32_t low;
    int32_t high;
};

#if defined __cplusplus
}
#endif

#endif  // VARJO_TYPES_D3D11_H
