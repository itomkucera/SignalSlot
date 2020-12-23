#ifndef __ITOMSIGNAL_H__
#define __ITOMSIGNAL_H__

#include <functional>
#include <map>
#include <memory>
#include <utility>

// IMPLEMENTATION DETAILS
namespace itom::detail
{

/*
* Signal's interface used as a "forward declaration"
*/
class ISignal
{
public:

    virtual void Disconnect(size_t slot_id) = 0;
};

/* 
* Connection class carries information about the signal and
* slot's position inside the signal
*/ 
class Connection final
{
public:

    Connection(size_t slot_id, ISignal& signal) :
        slot_id_{ slot_id },
        signal_{ signal }
    {
        
    }

    size_t GetSlotID() const
    {
        return slot_id_;
    }

    ISignal& GetSignal() const
    {
        return signal_;
    }

private:

    size_t slot_id_;
    ISignal& signal_;
};

} // namespace itom::detail


// PUBLIC API
namespace itom
{
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
                shared_con->GetSignal().Disconnect(shared_con->GetSlotID());
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
class Signal : public detail::ISignal
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

    // stores the slot inside the signal
    template <typename S>
    size_t Connect(S&& slot)
    {
        // store the slot at the actual position
        slots_.emplace(actual_slot_id_, std::forward<S>(slot));
        // return this  position and increse the counter
        return actual_slot_id_++;
    }

    // creates a connection with a pointer to the disconnector
    // and stores the slot inside the signal
    template <typename S>
    size_t Connect(S&& slot, Disconnector* disconnector)
    {
        // create a connection and pass it to the disconnector
        connections_.emplace_back(
            std::make_shared<detail::Connection>(actual_slot_id_, *this));
        disconnector->AddConnection(connections_.back());

        // store the slot and return its position
        return Connect(std::forward<S>(slot));
    }

    // creates a connection with a reference to the disconnector
    // and stores the slot inside the signal    
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