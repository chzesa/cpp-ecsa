#define CZSS_IMPLEMENTATION
#include "czss.hpp"

#define CZSF_IMPLEMENTATION
#include "external/c-fiber/czsf.h"

#include <iostream>
#include <chrono>
#include <thread>

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

struct A : Component<A>
{
	uint64_t value;
};

struct B : Component<B> {};
struct Iter : Iterator<A> {};
struct Enta : Entity<A, B> {};
struct Entb : Entity<A> {};

struct MyArch;

struct Sysa : System <Dependency<>, Orchestrator<Enta>, Writer <Resa>>
{
	static void run(Accessor<MyArch, Sysa> arch)
	{
		auto ent = arch.createEntity<Enta>();
		ent->getComponent<A>()->value = rand()%10000;
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

struct Sysb : System <Dependency<Sysa>, Reader<Iter, Entb>, Writer<Resa>>
{
	template<typename Arch>
	static void run(Arch arch)
	{
		auto res = arch.template getResource<Resa>();
		for (auto& iter : arch.template iterate<Iter>())
		{
			res->sum += iter.template view<A>()->value;
		}
	}

	template<typename Arch>
	static void initialize(Arch arch)
	{
		std::cout << "Initialize Sysb" << std::endl;
	};

	template<typename Arch>
	static void shutdown(Arch arch)
	{
		std::cout << "Shutdown Sysb" << std::endl;
	};
};

struct MyArch : Architecture <MyArch, Sysb, Sysa> {};

void fmain()
{
	srand(time(0));

	MyArch arch = {};
	Resa res = {};
	arch.setResource(&res);

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