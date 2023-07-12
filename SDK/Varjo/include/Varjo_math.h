// Copyright 2019-2021 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_MATH_H
#define VARJO_MATH_H

#include "Varjo_export.h"
#include "Varjo_types.h"
#include <math.h>
#include <string.h>

#if defined __cplusplus
extern "C" {
#endif

/**
 * Extracts the euler angles in radians from the given matrix.
 *
 * Default convention is right handed counter clockwise rotation, where
 * X-axis is negative and Y and Z axes are positive.
 *
 * Rotations are global, to get local rotations flip the axes. Eg:
 *   Global rotation XYZ -> local rotation ZYX.
 *
 * @param matrix Matrix to extract the angles from.
 * @param order Euler axis order.
 * @param direction Rotation direction.
 * @param handedness Rotation Handedness.
 */
VARJO_API struct varjo_Vector3D varjo_GetEulerAngles(
    struct varjo_Matrix* matrix, varjo_EulerOrder order, varjo_RotationDirection direction, varjo_Handedness handedness);

/**
 * Gets the translation part from a transformation matrix.
 *
 * @param matrix Matrix to extract the translation from.
 */
VARJO_API struct varjo_Vector3D varjo_GetPosition(struct varjo_Matrix* matrix);

/**
 * Updates the near and far clip plane distances in the projection matrix.
 *
 * @param projectionMatrix Projection matrix to modify.
 * @param clipRange Clip space range (0..1 or -1..1).
 * @param nearZ Positive distance in meters to the near plane. To indicate depth values reversed, can be greater than farZ.
 * @param farZ Positive distance in meters to the far plane. To indicate depth values reversed, can be smaller than nearZ.
 */
static void varjo_UpdateNearFarPlanes(double* projectionMatrix, varjo_ClipRange clipRange, double nearZ, double farZ)
{
    if (projectionMatrix == NULL) {
        return;
    }

    if (clipRange == varjo_ClipRangeZeroToOne) {
        if (farZ == HUGE_VAL) {
            // Normal Z-range, infinite far clip plane, D3D/Vulkan convention, OpenGL with glClipControl(GL_ZERO_TO_ONE)
            projectionMatrix[10] = -1.;
            projectionMatrix[14] = -nearZ;
        } else if (nearZ == HUGE_VAL) {
            // Inverted Z-range, infinite near clip plane, D3D/Vulkan convention, OpenGL with glClipControl(GL_ZERO_TO_ONE)
            projectionMatrix[10] = 0.;
            projectionMatrix[14] = farZ;
        } else {
            // Note: Following formula handles both cases: when farZ > nearZ and when nearZ > farZ
            projectionMatrix[10] = farZ / (nearZ - farZ);
            projectionMatrix[14] = (nearZ * farZ) / (nearZ - farZ);
        }
    } else {
        if (farZ == HUGE_VAL) {
            // Normal Z-range, infinite far clip plane, OpenGL with default clip range
            projectionMatrix[10] = -1.;
            projectionMatrix[14] = -2. * nearZ;
        } else if (nearZ == HUGE_VAL) {
            // Inverted Z-range, infinite near clip plane, OpenGL with default clip range
            projectionMatrix[10] = 1.;
            projectionMatrix[14] = 2. * farZ;
        } else {
            // Handles both normal and inverted Z-ranges in default OpenGL convention
            projectionMatrix[10] = (nearZ + farZ) / (nearZ - farZ);
            projectionMatrix[14] = (2. * nearZ * farZ) / (nearZ - farZ);
        }

    }
}
/**
 * Creates a projection matrix from given FOV tangents.
 *
 * @param tangents Tangents for a view.
 * @return projection Projection matrix with default depth transform.
 *
 * @see varjo_UpdateNearFarPlanes() To update the depth transform in the projection matrix.
 */
static struct varjo_Matrix varjo_GetProjectionMatrix(struct varjo_FovTangents* tangents)
{
    struct varjo_Matrix varjoMatrix;
    memset(&varjoMatrix, 0, sizeof(struct varjo_Matrix));

    if (tangents == NULL) {
        varjoMatrix.value[0] = 1.0f;
        varjoMatrix.value[5] = 1.0f;
        varjoMatrix.value[10] = 1.0f;
        varjoMatrix.value[15] = 1.0f;
    } else {
        const double invSizeX = 1.0 / (tangents->right - tangents->left);
        const double invSizeY = 1.0 / (tangents->top - tangents->bottom);
        const double offsetX = (tangents->left + tangents->right) * invSizeX;
        const double offsetY = (tangents->bottom + tangents->top) * invSizeY;

        varjoMatrix.value[0] = 2.0 * invSizeX;
        varjoMatrix.value[5] = 2.0 * invSizeY;
        varjoMatrix.value[8] = offsetX;
        varjoMatrix.value[9] = offsetY;
        varjoMatrix.value[10] = -1.0;
        varjoMatrix.value[11] = -1.0;
        varjoMatrix.value[14] = -0.1;
    }

    return varjoMatrix;
}


#if defined __cplusplus
}
#endif

#endif  // VARJO_MATH_H
