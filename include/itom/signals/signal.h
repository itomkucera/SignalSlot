#ifndef __ITOM_SIGNALS_SIGNAL_H__
#define __ITOM_SIGNALS_SIGNAL_H__

#include "auto_terminator.h"

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
        return { impl_->Connect(std::forward<S>(function)), impl_ };
    }

    // function - non-static member function of disconnector, invocable with Args...
    // disconnector - inherits Disconnector
    template <typename S, typename D>
    Connection Connect(S&& function, D* terminator,
        typename std::enable_if_t<
        std::is_base_of_v<AutoTerminator, D> &&
        std::is_invocable_v<S, D, Args...>>* = nullptr)
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
            // TODO: terminator could be destroyed here by another thread
            auto slot = impl_->Connect(std::forward<S>(function));
            terminator->AddConnection(Connection{ slot, impl_ });
            return { std::move(slot), impl_ };
        }
        return {};
    }

    // invokes all the connected slots
    template <typename... EmitArgs>
    void Emit(EmitArgs&&... args) const
    {
        static_assert(
            (std::is_convertible_v<EmitArgs, Args> && ...),
            "Parameter types don't match.");

        impl_->Emit(std::forward<EmitArgs>(args)...);
    }

    bool GetSlotCount() const
    {
        return impl_->GetSlotCount();
    }

    void TerminateAll()
    {
        impl_->TerminateAll();
    }

private:

    std::shared_ptr<detail::SignalImpl<Args...>> impl_;
};

} // namespace itom

#endif // __ITOM_SIGNALS_SIGNAL_H__