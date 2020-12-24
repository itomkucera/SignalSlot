#ifndef __ITOMSIGNAL_H__
#define __ITOMSIGNAL_H__

#include <functional>
#include <map>

// HIDDEN IMPLEMENTATION DETAILS
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


// PUBLIC API
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
    using ConnectionContainer = std::vector<std::weak_ptr<Connection>>;

public:
    template <typename... Args>
    friend class Signal;

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


    // creates a connection and returns it
    template <typename S>
    std::weak_ptr<Connection> Connect(S&& slot)
    {
        return CreateConnection(std::forward<S>(slot));
    }

    // creates a connection with a pointer to the disconnector
    // and returns this connection
    template <typename S,  typename D>
    std::weak_ptr<Connection> Connect(S&& slot, D* disconnector,
        typename std::enable_if<std::is_base_of<Disconnector, D>::value>::type* = nullptr)
    {
        if (!disconnector)
        {
            return {};
        }

        std::shared_ptr<Connection> connection;
        if constexpr (std::is_invocable<decltype(slot)>::value)
        {
            // invocable by itself
            connection = CreateConnection(std::forward<S>(slot));
        }
        else
        {
            // probably a non-static member function pointer - bind it
            connection = CreateConnection([&](Args&&... args) {
                (disconnector->*slot)(std::forward<Args>(args)...);
            });
        }

        // pass the connection to the disconnector
        disconnector->AddConnection(connection);

        return connection;
    }

    // calls all the connected slots
    template <typename... EmitArgs>
    void Emit(EmitArgs&&... args) const
    {
        for (auto&& slot : slots_)
        {
            if (slot.second)
            {
                slot.second(std::forward<EmitArgs>(args)...);
            }
        }
    }

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

    // stores the slot and returns the connection
    template <typename S>
    inline std::shared_ptr<Connection> CreateConnection(S&& slot)
    {
        // store the slot at the actual position
        slots_.emplace(actual_slot_id_, std::forward<S>(slot));

        // store the connection, increase the slot ID counter
        auto connection = std::make_shared<Connection>(actual_slot_id_++, *this);
        connections_.emplace(actual_slot_id_++, connection);

        return connection;
    }

    size_t actual_slot_id_ = 0; //< slot id counter

    ConnectionContainer connections_; //< ownership of the connections

    SlotContainer slots_; //< stored std::functions
};

} // namespace itom


#endif // __ITOMSIGNAL_H__