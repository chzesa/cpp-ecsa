#define CZSF_IMPL_THREADS
#define CZSS_IMPLEMENTATION
#include "czss.hpp"

#define CZSF_IMPLEMENTATION
#include <czsf.h>

#include <iostream>
#include <chrono>

using namespace czss;
using namespace std;
using namespace chrono;

volatile static bool EXITING = false;

struct Resa : Resource<Resa>
{
	uint64_t parallel = 0;
	uint64_t pointer = 0;
	uint64_t iter = 0;
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
	auto res = arch.getResource<Resa>();

	arch.parallelIterate<Iter>(8, [&] (uint64_t index, auto& accessor)
	{
		res->parallel += accessor.template viewComponent<A>()->value;
	});

	arch.iterate<Iterb>([&] (auto& accessor) {
		res->pointer += accessor.template viewComponent<B>()->other->value;
	});

	for (auto& iter : arch.template iterate<Iter>())
	{
		res->iter += iter.template viewComponent<A>()->value;
	}
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

int main(int argc, char** argv)
{
	srand(time(0));

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
		<< "\n\t Lambda using pointers: " << res.pointer
		<< "\n\t Parallel lambda: " << res.parallel
		<< "\n\t For-loop: " << res.iter
		<< std::endl;

	EXITING = true;
}