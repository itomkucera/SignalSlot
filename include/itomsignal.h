#ifndef __ITOMSIGNAL_H__
#define __ITOMSIGNAL_H__

#include <functional>
#include <map>

/*************** IMPLEMENTATION DETAILS ***************/

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

} // namespace itom::detail


/*************** PUBLIC API ***************/

// TODO: helpers for overloaded slot functors

#define EMIT //< makes emitting the signal more readable

namespace itom
{

/*
* Represents the signal-slot connection
*/
class Connection final
{
public:

    Connection(size_t slot_id, detail::ISignal& signal) :
        slot_id_{ slot_id },
        signal_{ signal }
    {

    }

    // terminates the connection, removes the slot from the signal
    void Disconnect()
    {
        signal_.Disconnect(slot_id_);
    }

private:

    size_t slot_id_;

    detail::ISignal& signal_;
};

/*
* Extend this class to automatically terminate all the connections
* that depend on the instance of this class (this object was declared
* as a disconnector for the connection)
*/
class Disconnector
{
    template <typename... Args>
    friend class Signal;

    using ConnectionContainer = std::vector<std::weak_ptr<Connection>>;

public:

    Disconnector() = default;

    virtual ~Disconnector()
    {
        // terminate all the connections
        for (auto&& weak_connection : connections_)
        {
            // that are still active - prevents from a crash
            if (auto shared_connection = weak_connection.lock())
            {
                shared_connection->Disconnect();
            }
        }
    }

private:

    template <typename WPTRC>
    void AddConnection(WPTRC&& connection)
    {
        connections_.push_back(std::forward<WPTRC>(connection));
    }

    // weak_ptr to connections - they may already be terminated
    // due to the signal desctruction
    ConnectionContainer connections_;
};



template <typename... Args>
class Signal final : public detail::ISignal
{
    using SlotContainer = std::map<size_t, std::function<void(Args...)>>;
    using ConnectionContainer = std::unordered_map<size_t, std::shared_ptr<Connection>>;

public:

    Signal() = default;

    Signal(const Signal&) = delete;

    Signal(Signal&&) noexcept = delete;

    Signal& operator=(const Signal&) = delete;

    Signal& operator=(Signal&&) noexcept = delete;

    // creates a connection
    // slot - invocable with Args...
    template <typename S>
    std::weak_ptr<Connection> Connect(S&& slot)
    {
        // store the slot at the actual position
        slots_.emplace(actual_slot_id_, std::forward<S>(slot));

        // store the connection
        auto connection = std::make_shared<Connection>(actual_slot_id_, *this);
        connections_.emplace(actual_slot_id_, connection);

        actual_slot_id_++;

        return connection;
    }

    // slot - non-static member function of disconnector, invocable with Args...
    // disconnector - inherits Disconnector
    template <typename S, typename D>
    std::weak_ptr<Connection> Connect(S&& slot, D* disconnector,
        typename std::enable_if_t<
            std::is_base_of_v<Disconnector, D> &&
            std::is_invocable_v<S, D, Args...>
        >* = nullptr)
    {
        return Connect([&](Args&&... args)
            { (disconnector->*slot)(std::forward<Args>(args)...); },
            disconnector);
    }

    // slot - invocable with Args...
    // disconnector - inherits Disconnector
    template <typename S>
    std::weak_ptr<Connection> Connect(S&& slot, Disconnector* disconnector,
        typename std::enable_if_t<std::is_invocable_v<S, Args...>>* = nullptr)
    {
        // terminate before the connection is created
        if (!disconnector)
        {
            return {};
        }

        auto connection = Connect(std::forward<S>(slot));
        disconnector->AddConnection(connection);

        return connection;
    }

    // calls all the connected slots
    template <typename... EmitArgs>
    void operator()(EmitArgs&&... args) const
    {
        for (auto&& slot : slots_)
        {
            // callable target could have been destroyed after Connect()
            if (slot.second)
            {
                slot.second(std::forward<EmitArgs>(args)...);
            }
        }
    }

    // terminates all the connections, removes all the slots
    void DisconnectAll()
    {
        slots_.clear();
        connections_.clear();
        actual_slot_id_ = 0; // optionally reset the counter
    }

private:

    // terminates the connection with specific id
    void Disconnect(size_t slot_id) override
    {
        slots_.erase(slot_id);
        connections_.erase(slot_id);
    }

    size_t actual_slot_id_ = 0; //< slot id counter

    ConnectionContainer connections_; //< ownership of the connections

    SlotContainer slots_; //< stored std::functions
};

} // namespace itom


#endif // __ITOMSIGNAL_H__