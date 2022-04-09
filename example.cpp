#define CZSS_IMPLEMENTATION
#include "czss.hpp"

#define CZSF_IMPLEMENTATION
#include "external/c-fiber/czsf.h"

#include <iostream>
#include <chrono>

using namespace czss;
using namespace std;
using namespace chrono;

volatile static bool EXITING = false;

void fmain();

int main()
{
	czsf::run(fmain);
	while (!EXITING) { czsf_yield(); }

}

struct Resa : Resource<Resa>
{
	uint64_t sum = 0;
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

template <>
void czss::managePointer<B, B>(B& component, const Repair<B>& rep)
{
	rep(component.other);
}

struct Iter : Iterator<A> {};
struct Iterb : Iterator<B>{};

struct Enta : Entity<A, B> {};
struct Entb : Entity<A> {};

CZSS_NAME(Enta, "Enta")
CZSS_NAME(Entb, "Entb")

struct MyArch;

struct Sysa : System <Dependency<>, Orchestrator<Enta>, Writer<RemoveGuid>>
{
	static void run(Accessor<MyArch, Sysa> arch)
	{
		auto ent = arch.createEntity<Enta>();
		A* a = ent->getComponent<A>();
		a->value = rand()%10000;

		B* b = ent->getComponent<B>();
		b->value = rand()%123;
		b->other = b;

		ent = arch.createEntity<Enta>();
		arch.getResource<RemoveGuid>()->guid = ent->getGuid();
	};

	static void initialize(Accessor<MyArch, Sysa> arch)
	{
		std::cout << "Initialize Sysa" << std::endl;
	};

	static void shutdown(Accessor<MyArch, Sysa> arch)
	{
		std::cout << "Shutdown Sysa" << std::endl;
	};
}; 

struct Sysb : System<Dependency<Sysa>, Orchestrator<Enta>, Reader<RemoveGuid>>
{
	static void run(Accessor<MyArch, Sysb> arch)
	{
		Guid guid = arch.viewResource<RemoveGuid>()->guid;
		arch.destroyEntity(guid);
	};

	static void initialize(Accessor<MyArch, Sysb> arch)
	{
		std::cout << "Initialize Sysb" << std::endl;
	};

	static void shutdown(Accessor<MyArch, Sysb> arch)
	{
		std::cout << "Shutdown Sysb" << std::endl;
	};
};

struct Sysc : System<Dependency<Sysb>, Orchestrator<Enta>, Reader<Iter, Iterb>, Writer<Resa>>
{
	static void run(Accessor<MyArch, Sysc> arch)
	{
		auto res = arch.template getResource<Resa>();

		arch.parallelIterate<Iter>(8, [&] (uint64_t index, IteratorAccessor<Iter, MyArch, Sysc>& accessor)
		{
			res->sum += accessor.view<A>()->value;
		});

		arch.iterate<Iterb>([&] (IteratorAccessor<Iterb, MyArch, Sysc>& accessor) {
			res->sum += accessor.view<B>()->other->value;
		});
	};

	static void initialize(Accessor<MyArch, Sysc> arch)
	{
		std::cout << "Initialize Sysc" << std::endl;
	};

	static void shutdown(Accessor<MyArch, Sysc> arch)
	{
		std::cout << "Shutdown Sysc" << std::endl;
	};
};

struct MyArch : Architecture <MyArch, Sysb, Sysa, Sysc> {};

void fmain()
{
	srand(time(0));

	MyArch arch;
	Resa res;
	RemoveGuid guid;
	arch.setResource(&res);
	arch.setResource(&guid);

	arch.initialize();

	std::cout << "Running 50000 iterations" << std::endl;
	auto start = high_resolution_clock::now();
	for (int i = 0; i < 50000; i++)
	{
		if (i % 1000 == 0)
			std::cout << "iter #" << i << std::endl;
		arch.run();
	}

	arch.shutdown();

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	std::cout << "Total duration " << double(duration.count())/1000000 << "s" << std::endl;

	std::cout << "Result: " << res.sum << std::endl;
	EXITING = true;
}