#ifndef __ITOMSIGNAL_H__
#define __ITOMSIGNAL_H__

#include <functional>
#include <algorithm>

/*************** IMPLEMENTATION DETAILS ***************/
namespace itom
{

template <typename... Args>
class Signal;

namespace detail
{

class ISlot
{
public:

    bool active_ = true;

};

template <typename... Args>
class Slot : public ISlot
{
public:

    template <typename F>
    Slot(F&& slot) :
        slot_{ std::forward<F>(slot) }
    {

    }

    std::function<void(Args...)> slot_;
};

/*
* SignalImpl's interface used as a non-templated "forward declaration"
*/
class ISignalImpl
{
public:

    virtual void Terminate(ISlot* slot) = 0;
};

/*
* Signal's internal implementation - useful for managing the instances of
* a single signal across other classes (Connection, Disconnector...)
*/
template <typename... Args>
class SignalImpl final : public ISignalImpl
{
    template <typename... Args>
    friend class Signal;
    
    using SlotContainer = std::list<std::shared_ptr<Slot<Args...>>>;

public:

    void Terminate(ISlot* slot) override
    {
        slots_.erase(std::remove_if(slots_.begin(), slots_.end(),
            [slot](const auto& stored) { return stored.get() == slot; }),
            slots_.end());
    }

    /*bool SlotExists(ISlot* slot) const override
    {
        return std::any_of(slots_.begin(), slots_.end(),
            [slot](const auto& shared_slot) { return shared_slot.get() == slot; });
    }*/

private:

    // slots must be stored here - wrapper could be destroyed by another thread
    // immediatelly after calling lock() inside the Connection -> possible crash
    SlotContainer slots_;
};

} // namespace itom::detail


/*************** PUBLIC API ***************/

// TODO: helpers for overloaded slot functors

#define EMIT std::invoke //< syntactic suggar for emitting the signal


/*
* Represents the signal-slot connection
*/

class Connection// : public detail::IConnection
{
public:

    Connection() = default;

    Connection(std::weak_ptr<detail::ISlot>&& slot, std::weak_ptr<detail::ISignalImpl>&& signal) :
        slot_{ std::move(slot) },
        signal_{ std::move(signal) }
    {
    
    }

    // terminates the connection, removes the slot from the signal
    // false if the signal was destroyed or the slot has already been removed
    void Terminate() 
    {
        if (auto shared_slot = slot_.lock())
        {
            if (auto shared_signal = signal_.lock())
            {
                shared_signal->Terminate(shared_slot.get());
            }
        }
    }

    // checks if the slot is still stored inside the living signal instance
    bool IsTerminated() const
    {
        return slot_.expired();
    }

    void Activate(bool activate = true)
    {
        if (auto shared_slot = slot_.lock())
        {
            shared_slot->active_ = activate;
        }
    }

    // checks if the slot will be invoked when signal gets emitted
    bool IsActive() const
    {
        if (auto shared_slot = slot_.lock())
        {
            return shared_slot->active_;
        }
        return false;
    }

private:

    std::weak_ptr<detail::ISlot> slot_;
    std::weak_ptr<detail::ISignalImpl> signal_;
};

/*
* Extend this class to automatically terminate all the connections
* that depend on the instance of this class (this object was declared
* as a disconnector for the connection)
*/
class AutoTerminator
{
    template <typename... Args>
    friend class Signal;

    using ConnectionContainer = std::list<Connection>;

public:

    AutoTerminator() = default;

    virtual ~AutoTerminator()
    {
        TerminateAll();
    }

    void TerminateAll()
    {
        for (auto&& connection : connections_)
        {
            connection.Terminate();
        }
    }

    size_t GetConnectionCount()
    {
        return connections_.size();
    }

private:

    template <typename... Args>
    void EmplaceConnection(Args&&... args)
    {
        connections_.emplace_back(std::forward<Args>(args)...);
    }

    ConnectionContainer connections_;
};


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


#endif // __ITOMSIGNAL_H__