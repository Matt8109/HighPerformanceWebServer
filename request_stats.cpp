#include "request_stats.hpp"

namespace base {

RequestStats::RequestStats(int num_threads)
    : num_threads_(num_threads),
      stat_buffers_(new ServerStatBuffer*[num_threads]),
      ticks_per_slot_(TicksClock::ticksPerSecond() / SLOTS) {
  for (int i = 0; i < num_threads_; i++)
    stat_buffers_[i] = new ServerStatBuffer(SLOTS);
}

RequestStats::~RequestStats() {
  for (int i = 0; i < num_threads_; i++)
    delete stat_buffers_[i];

  delete[] stat_buffers_;
}

void RequestStats::finishedRequest(int thread_num, TicksClock::Ticks now) {
  stat_buffers_[thread_num]->hit(now);
}

void RequestStats::getStats(TicksClock::Ticks now,
                            uint32_t* reqsLastSec ) const {
  if (reqsLastSec == NULL)
    reqsLastSec = new uint32_t;

  *reqsLastSec = 0;

  for (int i = 0; i < num_threads_; i++) 
    *reqsLastSec += stat_buffers_[i]->getHits(now);
}

} // namespace http
