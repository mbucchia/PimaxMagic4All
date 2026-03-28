// MIT License
//
// Copyright(c) 2022-2026 Matthieu Bucchianeri
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

#include "pch.h"

namespace util {

    static inline std::string ToString(const vr::HmdVector3_t& vec) {
        return fmt::format("({:.3f}, {:.3f}, {:.3f})", vec.v[0], vec.v[1], vec.v[2]);
    }

    template <typename TMethod>
    void DetourDllAttach(const char* dll, const char* target, TMethod hooked, TMethod& original) {
        if (original) {
            // Already hooked.
            return;
        }

        HMODULE handle;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN, dll, &handle);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        original = (TMethod)GetProcAddress(handle, target);
        DetourAttach((PVOID*)&original, hooked);

        DetourTransactionCommit();
    }

    template <typename TMethod>
    void DetourDllDetach(const char* dll, const char* target, TMethod hooked, TMethod& original) {
        if (!original) {
            // Not hooked.
            return;
        }

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourDetach((PVOID*)&original, hooked);

        DetourTransactionCommit();

        original = nullptr;
    }

} // namespace util
