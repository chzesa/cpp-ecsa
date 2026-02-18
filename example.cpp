// #define CZSF_IMPL_THREADS
#define CZSS_IMPLEMENTATION
#include "czss.hpp"

#define CZSF_IMPLEMENTATION
#include <czsf.h>

#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>

using namespace czss;
using namespace std;
using namespace chrono;

volatile static bool EXITING = false;
static constexpr size_t N_PARALLEL = 4;
static constexpr size_t CACHE_PADDING = 32;

struct Resa : Resource<Resa>
{
	uint64_t parallel = 0;
	uint64_t pointer = 0;
	uint64_t iter = 0;
	uint64_t iter2 = 0;

	uint64_t parallel_ns = 0;
	uint64_t pointer_ns = 0;
	uint64_t iter_ns = 0;
	uint64_t iter2_ns = 0;
};

struct RemoveGuid : Resource<RemoveGuid>
{
	Guid guid;
};

struct A : Component<A>
{
	uint64_t value;
};

struct B : Component<B>
{
	uint64_t value;
	B* other;
};

struct Iter : Iterator<A> {};
struct Iterb : Iterator<B>{};

struct Enta : Entity<A, B> {};
struct Entb : Entity<A> {};

struct MyArch : czss::Architecture<
	MyArch,
	Resa,
	RemoveGuid,
	Enta,
	Entb
> {};

using A_run_a = Accessor<MyArch, System <
	Orchestrator<Enta>,
	Writer<RemoveGuid>
>>;

void run_a(A_run_a& arch)
{
	auto ent = arch.createEntity<Enta>();
	A* a = ent->getComponent<A>();
	a->value = rand()%10000;

	B* b = ent->getComponent<B>();
	b->value = a->value;
	b->other = b;

	ent = arch.createEntity<Enta>();
	arch.getResource<RemoveGuid>()->guid = ent->getGuid();
}

using A_run_b = Accessor<MyArch, System <
	Orchestrator<Enta>,
	Reader<RemoveGuid>
>>;

static void run_b(A_run_b& arch)
{
	Guid guid = arch.viewResource<RemoveGuid>()->guid;
	arch.destroyEntity(guid);
};

using A_run_c = Accessor<MyArch, System <
	Orchestrator<Enta>,
	Reader<Iter, Iterb>,
	Writer<Resa>
>>;

void run_c(A_run_c& arch)
{
	using namespace std::chrono;

	auto res = arch.getResource<Resa>();

	auto a = high_resolution_clock::now();

#ifdef CZSF_IMPL_THREADS
	size_t parallel = N_PARALLEL;
#else
	size_t ents = arch.countCompatibleEntities<Iter>();
	size_t parallel = ents / 8192;
	if (parallel == 0) parallel = 1;
#endif
	std::vector<uint64_t> counters(parallel * CACHE_PADDING, 0);

	arch.parallelIterate<Iter>(parallel, [&] (uint64_t index, auto& accessor)
	{
		counters[index * CACHE_PADDING] += accessor.template viewComponent<A>()->value;
	});

	for (int i = 0; i < parallel; i++)
		res->parallel += counters[i * CACHE_PADDING];

	auto b = high_resolution_clock::now();

	arch.iterate<Iterb>([&] (auto& accessor) {
		res->pointer += accessor.template viewComponent<B>()->other->value;
	});

	auto c = high_resolution_clock::now();

	for (auto& iter : arch.template iterate<Iter>())
	{
		res->iter += iter.template viewComponent<A>()->value;
	}

	auto d = high_resolution_clock::now();

	arch.iterate<Iter>([&] (auto& accessor) {
		res->iter2 += accessor.template viewComponent<A>()->value;
	});

	auto e = high_resolution_clock::now();

	res->parallel_ns += duration_cast<nanoseconds>(b - a).count();
	res->pointer_ns += duration_cast<nanoseconds>(c - b).count();
	res->iter_ns += duration_cast<nanoseconds>(d - c).count();
	res->iter2_ns += duration_cast<nanoseconds>(e - d).count();
};

// struct V : czss::Component<V> {};

// struct VirtualA : czss::Entity<V>, czss::Virtual
// {
// 	virtual void quack()
// 	{
// 		std::cout << "Quack" << std::endl;
// 	}
// };

// struct VirtualB : VirtualA
// {
// 	void quack() override
// 	{
// 		std::cout << "Woof" << std::endl;
// 	}
// };

// struct VirtualC : VirtualA
// {
// 	void quack() override
// 	{
// 		std::cout << "Meow" << std::endl;
// 	}
// };

// struct Sysd : System<Orchestrator<VirtualA>>
// {
// 	static void run(Accessor<MyArch, Sysd>& arch) { };

// 	static void initialize(Accessor<MyArch, Sysd>& arch)
// 	{
// 		std::cout << "Initialize Sysd" << std::endl;
// 		arch.createEntity<VirtualA>();
// 		arch.createEntity<VirtualB>();
// 		arch.createEntity<VirtualC>();

// 		arch.iterate2<czss::Iterator<V>>([&] (auto accessor) {
// 			accessor.getEntity()->quack();
// 		});
// 	};

// 	static void shutdown(Accessor<MyArch, Sysd>& arch)
// 	{
// 		std::cout << "Shutdown Sysd" << std::endl;
// 	};
// };

void fmain()
{
	srand(548739);

	MyArch arch;
	Resa res;
	RemoveGuid guid;
	arch.setResource(&res);
	arch.setResource(&guid);

	auto accessor = arch.accessor();

	std::cout << "Running 50000 iterations" << std::endl;
	auto start = high_resolution_clock::now();
	auto lap = high_resolution_clock::now();
	for (int i = 0; i < 50000; i++)
	{
		if (i % 1000 == 0)
			std::cout << "iter #" << i << " " << double(duration_cast<microseconds>(high_resolution_clock::now() - lap).count()) / 1000 << "ms" << std::endl;

		lap = high_resolution_clock::now();
		accessor.minirun(
			run_a,
			run_b,
			run_c
		);
	}

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	std::cout << "Total duration " << double(duration.count())/1000000 << "s" << std::endl;

	std::cout << "Results: "
		<< "\n\t For-loop:              " << res.iter
		<< "\n\t Lambda using pointers: " << res.pointer
		<< "\n\t Typed lambda (iter2):  " << res.iter
		<< "\n\t Parallel lambda:       " << res.parallel
		<< std::endl;

	std::cout << "Timing: "
		<< "\n\t For-loop:              " << std::setw(13) << res.iter_ns << "ns"
		<< "\n\t Lambda using pointers: " << std::setw(13) << res.pointer_ns << "ns"
		<< "\n\t Typed lambda (iter2):  " << std::setw(13) << res.iter2_ns << "ns"
		<< "\n\t Parallel lambda:       " << std::setw(13) << res.parallel_ns << "ns"
		<< std::endl;

	EXITING = true;
}

int main()
{
#ifdef CZSF_IMPL_THREADS
	fmain();
#else
	czsf::run(fmain);

	std::thread threads[N_PARALLEL];

	for (int i = 0; i < N_PARALLEL; i++)
		threads[i] = std::thread([] { while (!EXITING) { czsf_yield(); } });

	for (int i = 0; i < N_PARALLEL; i++)
		threads[i].join();
#endif
}
