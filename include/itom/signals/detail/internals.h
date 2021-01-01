#ifndef __ITOM_SIGNALS_DETAIL_SLOT_H__
#define __ITOM_SIGNALS_DETAIL_SLOT_H__

#include <functional>
#include <algorithm>

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

} // namespace detail

} // namespace itom

#endif // __ITOM_SIGNALS_DETAIL_SLOT_H__