#pragma once

#include <condition_variable>
#include <atomic>
#include <mutex>

using std::mutex;
using std::condition_variable;
using std::atomic_bool;


// TODO: replace this with atomic_flag as soon as the wait() and so on methods are available
// TODO: why is this faster than atomic_flag?
class simple_flag {
public:
	simple_flag();
	simple_flag(const simple_flag&) = delete;
	simple_flag(simple_flag&&) noexcept;
	simple_flag& operator=(const simple_flag&) = delete;
	simple_flag& operator=(simple_flag&&) noexcept;
	~simple_flag() = default;

	// wait until flag is true
	void wait();
	// set the flag to true; notifies all waiting threads
	void set();
	// reset the flag to false
	void clear();

private:
	// atomic_flag flag;
	atomic_bool flag;
	condition_variable cv;
	mutex mtx;
};


