// Copyright 2019-2020 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_EXPORT_H
#define VARJO_EXPORT_H

#if defined __cplusplus
extern "C" {
#endif

#ifndef VARJORUNTIME_STATIC
#ifdef VARJORUNTIME_EXPORTS
#define VARJORUNTIME_EXPORT __declspec(dllexport)
#else
#define VARJORUNTIME_EXPORT __declspec(dllimport)
#endif
#else
#define VARJORUNTIME_EXPORT
#endif

#ifndef VARJORUNTIME_NO_EXPORT
#define VARJORUNTIME_NO_EXPORT
#endif

#ifndef VARJORUNTIME_DEPRECATED
#define VARJORUNTIME_DEPRECATED __declspec(deprecated)
#endif

#define VARJO_API VARJORUNTIME_EXPORT
#define VARJO_EXPERIMENTAL_API VARJORUNTIME_EXPORT
#define VARJO_DEPRECATED_API VARJORUNTIME_EXPORT VARJORUNTIME_DEPRECATED

#if defined __cplusplus
}
#endif

#endif  // VARJO_EXPORT_H
