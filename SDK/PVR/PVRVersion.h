/************************************************************************************

Filename    :   PVRVersion.h
Content     :   pvr sdk version definitions.

Copyright   :   Copyright 2017 Pimax, Inc. All Rights reserved.
************************************************************************************/
#ifndef _PVR_VERSION_H
#define _PVR_VERSION_H

#if !defined(PVR_STRINGIZE)
#define PVR_STRINGIZEIMPL(x) #x
#define PVR_STRINGIZE(x)     PVR_STRINGIZEIMPL(x)
#endif

#define PVR_MAJOR_VERSION 1
#define PVR_MINOR_VERSION 24
#define PVR_PATCH_VERSION 1
#define PVR_BUILD_NUMBER  1

/// "Major.Minor.Patch"
#define PVR_VERSION_STRING  PVR_STRINGIZE(PVR_MAJOR_VERSION.PVR_MINOR_VERSION.PVR_PATCH_VERSION)

/// "Major.Minor.Patch.Build"
#define PVR_DETAILED_VERSION_STRING PVR_STRINGIZE(PVR_MAJOR_VERSION.PVR_MINOR_VERSION.PVR_PATCH_VERSION.PVR_BUILD_NUMBER)


#if defined(_DEBUG)
#define PVR_FILE_DESCRIPTION_STRING "dev build debug"
#else
#define PVR_FILE_DESCRIPTION_STRING "dev build"
#endif

#endif
