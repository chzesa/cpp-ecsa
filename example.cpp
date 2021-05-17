#define CZSS_IMPLEMENTATION
#include "czss.hpp"

#define CZSF_IMPLEMENTATION
#include "czsf.h"

#include <iostream>
#include <chrono>
#include <thread>

using namespace czss;
using namespace std;
using namespace chrono;

void fmain();

int main()
{
	czsf::Barrier b;
	czsf::run(fmain, &b);
	b.wait();
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

struct Sysa : System <Dependency<>, Orchestrator<Enta>, Writer <Resa>>
{
	template<typename Arch>
	static void run(Arch arch)
	{
		auto ent = arch.template createEntity<Enta>();
		ent->template getComponent<A>()->value = rand()%10000;
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
};

struct MyArch : Architecture <Sysb, Sysa> {};

void fmain()
{
	srand(time(0));

	MyArch arch = {};
	Resa res = {};
	arch.setResource(&res);

	std::cout << "Running 5000 iterations" << std::endl;
	auto start = high_resolution_clock::now();
	for (int i = 0; i < 5000; i++)
		arch.run();

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	std::cout << "Total duration " << double(duration.count())/1000000 << "s" << std::endl;

	std::cout << "Result: " << res.sum << std::endl;
}