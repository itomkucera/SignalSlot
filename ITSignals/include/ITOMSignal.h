#ifndef __ITOMSIGNAL_H__
#define __ITOMSIGNAL_H__

#include <functional>
#include <map>
#include <memory>
#include <utility>

namespace itom
{

// hide the implementation details in the detail namespace
namespace detail
{

struct SignalBase
{
    virtual void Disconnect(size_t slot_id) = 0;
};

struct Connection
{
    Connection(size_t slot_id, SignalBase& signal) :
        slot_id_{ slot_id },
        signal_{ signal }
    {

    }

    size_t slot_id_;
    SignalBase& signal_;
};

} // namespace detail


/*
* If any class extends from Disconnector, all the connections
* that depend on the instance of this class (this object was declared
* as a disconnector for the connection) will be terminated (slots
* will be removed from the associated signal).
*/
class Disconnector
{
    using WeakPtrConnection = std::weak_ptr<detail::Connection>;
    using WeakPtrConnectionContainer = std::vector<WeakPtrConnection>;

public:
    template <typename... Args>
    friend class Signal;

    Disconnector() = default;

    virtual ~Disconnector()
    {
        // terminate all the connections
        for (auto&& weak_con : connections_)
        {
            // that are still active - prevents from a crash
            if (auto shared_con = weak_con.lock())
            {
                shared_con->signal_.Disconnect(shared_con->slot_id_);
            }
        }
    }

private:

    void AddConnection(const WeakPtrConnection& connection)
    {
        connections_.push_back(connection);
    }

    // weak_ptr to connections - they may already be terminated
    // due to the signal desctruction
    WeakPtrConnectionContainer connections_;
};



template <typename... Args>
class Signal : public detail::SignalBase
{
    using SlotType = std::function<void(Args...)>;
    using SlotContainer = std::map<size_t, SlotType>;

    using SharedPtrConnection = std::shared_ptr<detail::Connection>;
    using SharedPtrConnectionContainer = std::vector<SharedPtrConnection>;

public:

    Signal() = default;

    Signal(const Signal&) = delete;

    Signal(Signal&&) noexcept = delete;

    Signal& operator=(const Signal&) = delete;

    Signal& operator=(Signal&&) noexcept = delete;

    void Disconnect(size_t slot_id) override
    {
        slots_.erase(slot_id);
    }

    // create a connection without a disconnector
    template <typename S>
    size_t Connect(S&& slot)
    {
        slots_.emplace(actual_slot_id_, std::forward<S>(slot));
        return actual_slot_id_++;
    }

    // create a connection with a pointer to the disconnector
    template <typename S>
    size_t Connect(S&& slot, Disconnector* disconnector)
    {
        connections_.emplace_back(
            std::make_shared<detail::Connection>(actual_slot_id_, *this));
        disconnector->AddConnection(connections_.back());

        return Connect(std::forward<S>(slot));
    }

    // create a connection with a reference to the disconnector
    template <typename S>
    size_t Connect(S&& slot, Disconnector& disconnector)
    {
        return Connect(std::forward<S>(slot), std::addressof(disconnector));
    }

    // emit the signal - call all the connected slots
    template <typename... EmitArgs>
    void Emit(EmitArgs&&... args)
    {
        for (auto&& slot : slots_)
        {
            // TODO: empty function ???
            slot.second(std::forward<EmitArgs>(args)...);
        }
    }


private:
    // slot id counter
    size_t actual_slot_id_ = 0;

    // keep track of the connections to prevent the disconnector
    // from trying to terminate a non-existing connection
    SharedPtrConnectionContainer connections_;

    // connected slots
    SlotContainer slots_;
};

} // namespace itom


#endif // __ITOMSIGNAL_H__