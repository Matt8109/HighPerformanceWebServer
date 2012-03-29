#include "request_stats.hpp"

namespace base {

RequestStats::RequestStats(int num_threads) {
}

RequestStats::~RequestStats() {
}

void RequestStats::finishedRequest(int thread_num, TicksClock::Ticks now) {
}

void RequestStats::getStats(TicksClock::Ticks now,
                            uint32_t* reqsLastSec ) const {
}

} // namespace http
