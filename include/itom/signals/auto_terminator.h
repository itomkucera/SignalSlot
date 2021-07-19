#ifndef __ITOM_SIGNALS_AUTOTERMINATOR_H__
#define __ITOM_SIGNALS_AUTOTERMINATOR_H__

#include "connection.h"

namespace itom {

/*
 * Extend this class to automatically terminate all the connections
 * that depend on the instance of this class (this object was declared
 * as a disconnector for the connection)
 */
class AutoTerminator {
    template <typename... Args>
    friend class Signal;

    using ConnectionContainer = std::vector<Connection>;

   public:
    AutoTerminator() = default;

    virtual ~AutoTerminator() { TerminateAll(); }

    void TerminateAll() {
        for (auto&& connection : connections_) {
            connection.Terminate();
        }
    }

    size_t GetConnectionCount() { return connections_.size(); }

   private:
    template <typename C>
    void AddConnection(C&& connection) {
        connections_.push_back(std::forward<C>(connection));
    }

    ConnectionContainer connections_;
};

}  // namespace itom

#endif  // __ITOM_SIGNALS_AUTOTERMINATOR_H__
