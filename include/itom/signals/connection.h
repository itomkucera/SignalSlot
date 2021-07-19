#ifndef __ITOM_SIGNALS_CONNECTION_H__
#define __ITOM_SIGNALS_CONNECTION_H__

#include "detail/internals.h"

namespace itom {

/*
 * Represents the signal-slot connection
 */

class Connection {
    template <typename... Args>
    friend class Signal;

    friend class AutoTerminator;

    using SlotType = std::weak_ptr<detail::ISlot>;
    using SignalType = std::weak_ptr<detail::ISignalImpl>;

   public:
    Connection() = default;

    // terminates the connection, removes the slot from the signal
    // false if the signal was destroyed or the slot has already been removed
    void Terminate() {
        if (auto shared_slot = slot_.lock()) {
            if (auto shared_signal = signal_.lock()) {
                shared_signal->Terminate(shared_slot.get());
            }
        }
    }

    // checks if the slot is still stored inside the living signal instance
    bool IsTerminated() const { return slot_.expired(); }

    void Activate(bool activate = true) {
        if (auto shared_slot = slot_.lock()) {
            shared_slot->active_ = activate;
        }
    }

    // checks if the slot will be invoked when signal gets emitted
    bool IsActive() const {
        if (auto shared_slot = slot_.lock()) {
            return shared_slot->active_;
        }
        return false;
    }

   private:
    Connection(SlotType&& slot, SignalType&& signal)
        : slot_{std::move(slot)}, signal_{std::move(signal)} {}

    SlotType slot_;
    SignalType signal_;
};

}  // namespace itom

#endif  // __ITOM_SIGNALS_CONNECTION_H__
