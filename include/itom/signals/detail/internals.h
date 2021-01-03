#ifndef __ITOM_SIGNALS_DETAIL_SLOT_H__
#define __ITOM_SIGNALS_DETAIL_SLOT_H__

#include <functional>
#include <algorithm>

namespace itom
{

template <typename... Args>
class Signal;

class Connection;
class AutoTerminator;

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
    using SlotContainer = std::list<std::shared_ptr<Slot<Args...>>>;

public:

    // function - invocable with Args...
    template <typename S>
    std::weak_ptr<detail::Slot<Args...>> Connect(S&& function)
    {
        // store the slot at the actual position
        return slots_.emplace_back(
            std::make_shared<detail::Slot<Args...>>(std::forward<S>(function)));
    }

    // invokes all the connected slots
    template <typename... EmitArgs>
    void Emit(EmitArgs&&... args) const
    {
        for (auto&& slot : slots_)
        {
            // callable target could have been destroyed after Connect()
            if (slot->slot_ && slot->active_)
            {
                (slot->slot_)(std::forward<EmitArgs>(args)...);
            }
        }
    }

    void Terminate(ISlot* slot) override
    {
        slots_.erase(std::remove_if(slots_.begin(), slots_.end(),
            [slot](const auto& stored) { return stored.get() == slot; }),
            slots_.end());
    }

    bool GetSlotCount() const
    {
        return slots_.size();
    }

    void TerminateAll()
    {
        slots_.clear();
    }

private:

    // slots must be stored here - wrapper could be destroyed by another thread
    // immediatelly after calling lock() inside the Connection -> possible crash
    SlotContainer slots_;
};

} // namespace detail

} // namespace itom

#endif // __ITOM_SIGNALS_DETAIL_SLOT_H__