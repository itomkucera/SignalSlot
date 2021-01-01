#ifndef __ITOM_SIGNALS_SIGNAL_H__
#define __ITOM_SIGNALS_SIGNAL_H__

#include "connection.h"
#include "auto_terminator.h"

#define EMIT std::invoke //< syntactic suggar for emitting the signal

namespace itom
{

/*
* API wrapper class for the signal
*/
template <typename... Args>
class Signal
{
public:

    Signal() :
        impl_{ std::make_shared<detail::SignalImpl<Args...>>() }
    {

    }

    Signal(const Signal&) = delete;

    Signal(Signal&&) noexcept = delete;

    Signal& operator=(const Signal&) = delete;

    Signal& operator=(Signal&&) noexcept = delete;

    // function - invocable with Args...
    template <typename S>
    Connection Connect(S&& function)
    {
        // store the slot at the actual position
        impl_->slots_.emplace_back(
            std::make_shared<detail::Slot<Args...>>(std::forward<S>(function)));

        return Connection(impl_->slots_.back(), impl_);
    }

    // function - non-static member function of disconnector, invocable with Args...
    // disconnector - inherits Disconnector
    template <typename S, typename D>
    Connection Connect(S&& function, D* terminator,
        typename std::enable_if_t<
        std::is_base_of_v<AutoTerminator, D> &&
        std::is_invocable_v<S, D, Args...>
        >* = nullptr)
    {
        // capture by value - slot should always be a pointer
        // void(D::*f)(Args...) doesn't expect const - another overload would be needed
        return Connect([=](Args&&... args) {
            (terminator->*function)(std::forward<Args>(args)...); },
            terminator);
    }

    // function - invocable with Args...
    // disconnector - Disconnector instance
    template <typename S>
    Connection Connect(S&& function, AutoTerminator* terminator,
        typename std::enable_if_t<std::is_invocable_v<S, Args...>>* = nullptr)
    {
        if (terminator)
        {
            // store the slot at the actual position
            impl_->slots_.emplace_back(
                std::make_shared<detail::Slot<Args...>>(std::forward<S>(function)));
            terminator->EmplaceConnection(impl_->slots_.back(), impl_);

            return Connection(impl_->slots_.back(), impl_);
        }

        // return an invalid connection (empty signal, empty slot)
        return {};
    }

    // invokes all the connected slots
    template <typename... EmitArgs>
    void operator()(EmitArgs&&... args) const
    {
        for (auto&& slot : impl_->slots_)
        {
            // callable target could have been destroyed after Connect()
            if (slot->slot_ && slot->active_)
            {
                (slot->slot_)(std::forward<EmitArgs>(args)...);
            }
        }
    }

    bool GetSlotCount() const
    {
        return impl_->slots_.size();
    }

    void TerminateAll()
    {
        impl_->slots_.clear();
    }

private:

    std::shared_ptr<detail::SignalImpl<Args...>> impl_;
};

} // namespace itom

#endif // __ITOM_SIGNALS_SIGNAL_H__