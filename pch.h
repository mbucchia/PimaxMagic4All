// MIT License
//
// Copyright(c) 2023-2025 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
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
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

// MSVC intrinsics.
#include <intrin.h>

// Windows header files.
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <TlHelp32.h>
#include <traceloggingactivity.h>
#include <traceloggingprovider.h>
#include <Unknwn.h>
#include <wil/resource.h>
#include <wil/registry.h>

// OpenXR SDK.
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <dxgi.h>
#include <d3d11.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// OpenXR Utilities.
#include <SampleShared/XrInstanceContext.h>
#include <SampleShared/XrSessionContext.h>
#include <SampleShared/XrSystemContext.h>
#include <XrUtility/XrError.h>
#include <XrUtility/XrMath.h>
#include <XrUtility/XrExtensionContext.h>
#include <XrUtility/XrToString.h>

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

// Detours
#include <detours.h>
