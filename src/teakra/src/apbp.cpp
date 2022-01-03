#include <array>
#include <utility>
#include "apbp.h"

namespace Teakra
{
    class DataChannel
    {
    public:
        void Reset()
        {
            ready = false;
            data = 0;
        }

        void Send(u16 data)
        {
            {
                ready = true;
                this->data = data;
                if (disable_interrupt)
                    return;
            }
            if (handler)
                handler();
        }
        u16 Recv()
        {
            ready = false;
            return data;
        }
        u16 Peek() const
        {
            return data;
        }
        bool IsReady() const
        {
            return ready;
        }
        u16 GetDisableInterrupt() const
        {
            return disable_interrupt;
        }
        void SetDisableInterrupt(u16 v)
        {
            disable_interrupt = v;
        }

        std::function<void()> handler;

    private:
        bool ready = false;
        u16 data = 0;
        u16 disable_interrupt = 0;
    };

    class Apbp::Impl
    {
    public:
        std::array<DataChannel, 3> data_channels;
        u16 semaphore = 0;
        u16 semaphore_mask = 0;
        bool semaphore_master_signal = false;
        std::function<void()> semaphore_handler;

        void Reset()
        {
            for (auto &c : data_channels)
                c.Reset();
            semaphore = 0;
            semaphore_mask = 0;
            semaphore_master_signal = false;
        }
    };

    Apbp::Apbp() : impl(new Impl) {}
    Apbp::~Apbp() = default;

    void Apbp::Reset()
    {
        impl->Reset();
    }

    void Apbp::SendData(unsigned channel, u16 data)
    {
        impl->data_channels[channel].Send(data);
    }

    u16 Apbp::RecvData(unsigned channel)
    {
        return impl->data_channels[channel].Recv();
    }

    u16 Apbp::PeekData(unsigned channel) const
    {
        return impl->data_channels[channel].Peek();
    }

    bool Apbp::IsDataReady(unsigned channel) const
    {
        return impl->data_channels[channel].IsReady();
    }

    u16 Apbp::GetDisableInterrupt(unsigned channel) const
    {
        return impl->data_channels[channel].GetDisableInterrupt();
    }

    void Apbp::SetDisableInterrupt(unsigned channel, u16 v)
    {
        impl->data_channels[channel].SetDisableInterrupt(v);
    }

    void Apbp::SetDataHandler(unsigned channel, std::function<void()> handler)
    {
        impl->data_channels[channel].handler = std::move(handler);
    }

    void Apbp::SetSemaphore(u16 bits)
    {
        impl->semaphore |= bits;
        bool new_signal = (impl->semaphore & ~impl->semaphore_mask) != 0;
        if (new_signal && impl->semaphore_handler)
        {
            impl->semaphore_handler();
        }
        impl->semaphore_master_signal = impl->semaphore_master_signal || new_signal;
    }

    void Apbp::ClearSemaphore(u16 bits)
    {
        impl->semaphore &= ~bits;
        impl->semaphore_master_signal = (impl->semaphore & ~impl->semaphore_mask) != 0;
    }

    u16 Apbp::GetSemaphore() const
    {
        return impl->semaphore;
    }

    void Apbp::MaskSemaphore(u16 bits)
    {
        impl->semaphore_mask = bits;
    }

    u16 Apbp::GetSemaphoreMask() const
    {
        return impl->semaphore_mask;
    }

    void Apbp::SetSemaphoreHandler(std::function<void()> handler)
    {
        impl->semaphore_handler = std::move(handler);
    }

    bool Apbp::IsSemaphoreSignaled() const
    {
        return impl->semaphore_master_signal;
    }
} // namespace Teakra
