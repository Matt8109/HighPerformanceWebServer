#ifndef MCP_REQUEST_STATS_HEADER
#define MCP_REQUEST_STATS_HEADER

#include <inttypes.h>
#include <string>

#include "ticks_clock.hpp"

namespace base {

using base::TicksClock;
using std::string;

// This class keeps enough state to compute the number of request
// served per second instantaneously. That is, when the stats is
// requested, the second finishing _roughly_ at the request time is
// used.
//
// Thread Safety:
//
//   We assume that finishedRequest(i,...) is only called by the i-th
//   thread, according to ThreadPoolFast::ME(). Calls to
//   finishRequest(j,...)  can be done concurrently with the former.
//
//   getStats() will be called at any time by some unknow thread. For
//   the lab4's purposes, we'll assume it can read the internal state
//   without synchronization (we'll reason about what could happen to
//   accuracy of the results).

class RequestStats {
public:
  explicit RequestStats(int num_threads);
  ~RequestStats();

  // Records that one request completed 'now'.  The thread calling
  // this should use it's identifier (returned by
  // ThreadPoolFast::ME())
  void finishedRequest(int thread_num, TicksClock::Ticks now);

  // Writes the current req/s stats for the second finishing 'now' in
  // 'reqsLastSec'.
  void getStats(TicksClock::Ticks now, uint32_t* reqsLastSec) const;

  // Accessors (for testing)

  uint64_t ticksPerSlot() const { return 0; }

private:

  // Non-copyable, non-assignable
  RequestStats(const RequestStats&);
  RequestStats& operator=(const RequestStats&);
};

}  // namespace base

#endif // MCP_REQUEST_STATS_HEADER
