#include "simple_flag.h"

using std::unique_lock;
using std::scoped_lock;

simple_flag::simple_flag() {
	flag.store(false);
}

simple_flag::simple_flag(simple_flag&& s) noexcept {
	flag.store(s.flag.load());
}
simple_flag& simple_flag::operator=(simple_flag&& s) noexcept {
	flag.store(s.flag.load());
    return *this;
}

void simple_flag::wait() {
	auto lock = unique_lock(mtx);
	// if another thread calls set AFTER load but BEFORE wait, this thread misses the wakeup
    // need unique_lock as we might need to re-acquire several times
	while (!flag.load()) {
		cv.wait(lock);
	}
}
void simple_flag::set() {
	// lock needed here because if another thread is in wait() in between load and wait it will miss the wakeup
    // scoped_lock is fine as we only need to acquire once
	auto lock = scoped_lock(mtx);
	flag.store(true);
	cv.notify_all();
}
void simple_flag::clear() {
	flag.store(false);
}

