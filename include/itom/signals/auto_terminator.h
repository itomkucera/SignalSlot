#ifndef __ITOM_SIGNALS_AUTOTERMINATOR_H__
#define __ITOM_SIGNALS_AUTOTERMINATOR_H__

#include  <list>
#include "connection.h"

namespace itom
{

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

} // namespace itom

#endif // __ITOM_SIGNALS_AUTOTERMINATOR_H__