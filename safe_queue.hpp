#ifndef MCP_SAFE_QUEUE_HEADER
#define MCP_SAFE_QUEUE_HEADER

#include <queue>

#include "lock.hpp"

namespace base {
using base::Mutex;
using std::queue;

// A simple wrapper around the queue class that ensures 
// thready safety.
template <typename T>
class SafeQueue 
{
public:
	SafeQueue();
	~SafeQueue();

	void push(T value);
	T pop();
	int size();

private:
	mutable Mutex sync_root;
  queue<T> internal_queue;
};

template <typename T>
SafeQueue<T>::SafeQueue() { }

template <typename T>
SafeQueue<T>::~SafeQueue() { }

template <typename T>
void SafeQueue<T>::push(T value) {
	sync_root.lock();
	internal_queue.push(value);
	sync_root.unlock();
}

template <typename T>
T SafeQueue<T>::pop() {
	T return_value;

	return_value = internal_queue.front(); 
	internal_queue.pop();

	return return_value;
}

template <typename T>
int SafeQueue<T>::size() {
	int size;

	sync_root.lock();
	size = internal_queue.size();
	sync_root.unlock();

	if (size < 0)
		size = 0;

	return size;
}

}

#endif // MCP_SAFE_QUEUE_HEADER
