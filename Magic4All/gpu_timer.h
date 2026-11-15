// MIT License
//
// Copyright(c) 2026 Matthieu Bucchianeri
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

#include <wil/com.h>

namespace util {

    // An asynchronous GPU timer for Direct3D 11 (adapted from VirtualDesktop-OpenXR).
    // query() never blocks: it returns 0 if the result is not available (yet).
    class D3D11GpuTimer {
      public:
        // Returns nullptr if the queries cannot be created.
        static std::unique_ptr<D3D11GpuTimer> Create(ID3D11Device* device) {
            auto timer = std::unique_ptr<D3D11GpuTimer>(new D3D11GpuTimer());
            device->GetImmediateContext(timer->m_context.put());

            D3D11_QUERY_DESC queryDesc{};
            queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
            if (FAILED(device->CreateQuery(&queryDesc, timer->m_timeStampDis.put()))) {
                return {};
            }
            queryDesc.Query = D3D11_QUERY_TIMESTAMP;
            if (FAILED(device->CreateQuery(&queryDesc, timer->m_timeStampStart.put())) ||
                FAILED(device->CreateQuery(&queryDesc, timer->m_timeStampEnd.put()))) {
                return {};
            }
            return timer;
        }

        void start() {
            m_context->Begin(m_timeStampDis.get());
            m_context->End(m_timeStampStart.get());
        }

        void stop() {
            m_context->End(m_timeStampEnd.get());
            m_context->End(m_timeStampDis.get());
            m_valid = true;
        }

        // Duration in microseconds, or 0.
        uint64_t query() {
            uint64_t duration = 0;
            if (m_valid) {
                UINT64 startTime = 0, endTime = 0;
                D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disData{};
                if (m_context->GetData(m_timeStampStart.get(), &startTime, sizeof(startTime), 0) == S_OK &&
                    m_context->GetData(m_timeStampEnd.get(), &endTime, sizeof(endTime), 0) == S_OK &&
                    m_context->GetData(m_timeStampDis.get(), &disData, sizeof(disData), 0) == S_OK &&
                    !disData.Disjoint && disData.Frequency && endTime >= startTime) {
                    duration = static_cast<uint64_t>(((endTime - startTime) * 1e6) / disData.Frequency);
                }
                m_valid = false;
            }
            return duration;
        }

      private:
        D3D11GpuTimer() = default;

        wil::com_ptr<ID3D11DeviceContext> m_context;
        wil::com_ptr<ID3D11Query> m_timeStampDis;
        wil::com_ptr<ID3D11Query> m_timeStampStart;
        wil::com_ptr<ID3D11Query> m_timeStampEnd;

        // Can the timer be queried (it might still only read 0).
        bool m_valid = false;
    };

} // namespace util
