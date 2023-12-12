// MIT License
//
// Copyright(c) 2023 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

// Standard library.
#define _CRT_SECURE_NO_WARNINGS
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

// MSVC intrinsics.
#include <intrin.h>

// Windows header files.
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <traceloggingactivity.h>
#include <traceloggingprovider.h>
#include <wil/resource.h>
#include <wil/registry.h>

// We don't use OpenXR, however we import code as-is from the OpenXR-Eye-Trackers project, therefore we need definitions
// for OpenXR basic types (such as XrPosef or XrVector3f).
#define XR_NO_PROTOTYPES
#include <openxr/openxr.h>
#include <XrError.h>
#include <XrMath.h>
#include <XrStereoView.h>
#include <XrToString.h>

// We also need this definition to keep the compiler happy (unused code).
class OpenXrApi {};

// FMT formatter.
#define FMT_HEADER_ONLY
#include <fmt/format.h>

// Pimax SDK
#include <PVR.h>
#include <PVR_Interface.h>

// OpenVR
#include <openvr.h>

// Varjo SDK
#include <Varjo.h>

// HP Omnicept SDK
#include <omnicept/Glia.h>

// Detours
#include <detours.h>
