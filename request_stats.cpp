#include "request_stats.hpp"

namespace base {

RequestStats::RequestStats(int num_threads)
    : num_threads_(num_threads),
      ticks_per_slot_(TicksClock::ticksPerSecond() / SLOTS) {
}

RequestStats::~RequestStats() {
}

void RequestStats::finishedRequest(int thread_num, TicksClock::Ticks now) {
}

void RequestStats::getStats(TicksClock::Ticks now,
                            uint32_t* reqsLastSec ) const {
}

} // namespace http
