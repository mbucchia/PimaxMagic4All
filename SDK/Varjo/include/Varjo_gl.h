// Copyright 2019-2023 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_GL_H
#define VARJO_GL_H

#include "Varjo.h"
#include "Varjo_types_gl.h"

#if defined __cplusplus
extern "C" {
#endif

/**
 * Converts an OpenGL texture to #varjo_Texture.
 */
VARJO_API struct varjo_Texture varjo_FromGLTexture(GLuint texture);

/**
 * Converts #varjo_Texture to OpenGL texture.
 */
VARJO_API GLuint varjo_ToGLTexture(struct varjo_Texture texture);

#if defined __cplusplus
}
#endif

#endif  // VARJO_GL_H
