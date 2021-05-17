/* Throwaway code */

#ifndef CZSF_HEADERS_H
#define CZSF_HEADERS_H

#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace czsf
{

struct Sync
{
	Sync(int64_t value);
	virtual void wait() = 0;
	virtual void signal() = 0;

protected:
	std::mutex mutex;
	std::condition_variable condvar;
	int64_t value;
};

struct Barrier : Sync
{
	Barrier();
	Barrier(int64_t value);
	void wait() override;
	void signal() override;
};

struct Semaphore : Sync
{
	Semaphore();
	Semaphore(int64_t value);
	void wait() override;
	void signal() override;
};

template<class T>
void run(void (*fn)(T*), T* param, uint64_t count, czsf::Sync* sync)
{
	for (uint64_t i = 0; i < count; i++)
		std::thread([=] {
			fn(&param[i]);
			if (sync != nullptr)
				sync->signal();
		}).detach();
}

template<class T>
void run(void (*fn)(T*), T** param, uint64_t count, czsf::Sync* sync)
{
	for (uint64_t i = 0; i < count; i++)
		std::thread([=] {
			fn(param[i]);
			if (sync != nullptr)
				sync->signal();
		}).detach();
}

template<class T>
void run(void (*fn)(T*), T* param, uint64_t count) { run(fn, param, count, nullptr); }

template<class T>
void run(void (*fn)(T*), T** param, uint64_t count) { run(fn, param, count, nullptr); }

void run(void (*fn)(), czsf::Sync* sync);
void run(void (*fn)());

}

#endif // CZSF_HEADERS_H

#ifdef CZSF_IMPLEMENTATION

#ifndef CZSF_IMPLEMENTATION_GUARD_
#define CZSF_IMPLEMENTATION_GUARD_

namespace czsf
{

Sync::Sync(int64_t value) : mutex(), condvar()
{
	this->value = value;
}

Barrier::Barrier() : Sync(1) {}
Barrier::Barrier(int64_t value) : Sync(value) {}

void Barrier::wait()
{
	std::unique_lock<std::mutex> lock(mutex);
	if (value != 0)
		condvar.wait(lock, [&] { return value == 0; });
}

void Barrier::signal()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (value > 0 && --value == 0)
		condvar.notify_all();
}

Semaphore::Semaphore() : Sync(0) {}
Semaphore::Semaphore(int64_t value) : Sync(value) {}

void Semaphore::wait()
{
	std::unique_lock<std::mutex> lock(mutex);
	value--;
	if (value > 0)
		return;

	condvar.wait(lock, [&] { if (value > 0) { value--; return true; } return false; });
}

void Semaphore::signal()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (++value <= 1)
		condvar.notify_one();
}

void run(void (*fn)(), czsf::Sync* sync)
{
	std::thread([=] {
		fn();
		if (sync != nullptr)
			sync->signal();
	}).detach();
}

void run(void (*fn)()) { run(fn, nullptr); }

}

#endif // CZSF_IMPLEMENTATION_GUARD_
#endif // CZSF_IMPLEMENTATION