#define CZSS_IMPLEMENTATION
#include "czss.hpp"

#define CZSF_IMPLEMENTATION
#include <czsf.h>

#include <thread>
#include <iostream>
#include <chrono>

using namespace std;

static bool EXITING = false;

void print(string s)
{
	static czsf::Semaphore sem(1);
	sem.wait();
	cout << s << endl;
	sem.signal();
}

struct Entities
	: czss::Component<Entities>
{ };

struct PhysicsComponents
	: czss::Component<PhysicsComponents>
{ };

struct AnimationComponents
	: czss::Component<AnimationComponents>
{ };

struct AnimationData
	: czss::Component<AnimationData>
{ };

struct Input
	: czss::Component<Input>
{
	char pressed_button;
};

struct RenderComponent
	: czss::Component<RenderComponent>
{ };

// Behaviour systems

struct InputReader
	: czss::System<InputReader>
	, czss::Writer<Input>
{
	void run() const override
	{
		print("Reading OS input");
		Input* input = czss::Writer<Input>::get();
		input->pressed_button = 'A';
		this_thread::sleep_for(1s);
		print("OS input written");
	}
};

struct GameLoop
	: czss::System<GameLoop>
	, czss::Reader<Input>
	, czss::Writer<AnimationComponents, Entities, PhysicsComponents, RenderComponent>
	, czss::Dependency<InputReader>
{
	void run() const override
	{
		print("Running game loop");

		const Input* input = Reader<Input>::get();
		string s = "Player pressed button ";
		s += input->pressed_button;
		print(s);
		this_thread::sleep_for(1s);
		print("Game update finished");
	}
};

struct PhysicsUpdate
	: czss::System<PhysicsUpdate>
	, czss::Writer<PhysicsComponents>
	, czss::Dependency<GameLoop>
{
	void run() const override
	{
		print("Calculating physics");
		this_thread::sleep_for(3s);
		print("Finished physics update");
	}
};

struct Animator
	: czss::System<Animator>
	, czss::Reader<AnimationComponents, Entities>
	, czss::Writer<AnimationData>
	, czss::Dependency<GameLoop>
{
	void run() const override
	{
		print("Updating animations");
		this_thread::sleep_for(1s);
		print("Animation update finished");
	}
};

struct Renderer
	: czss::System<Renderer>
	, czss::Reader<AnimationData>
	, czss::Dependency<Animator>
{
	void run() const override
	{
		print("Rendering");
		this_thread::sleep_for(1s);
		print("Rendering finished");
	}
};

void fmain()
{
	// Instantiation order of systems doesn't matter

	// Instantiate data containers
	Entities ents = {};
	PhysicsComponents physComp = {};
	AnimationComponents animComp = {};
	AnimationData animData = {};
	Input inputData = {};
	RenderComponent rendComp = {};

	// Instantiate behaviour systems
	GameLoop gloop = {};
	InputReader input = {};
	PhysicsUpdate physics = {};
	Animator anim = {};
	Renderer rend = {};

	// Get pointer to default system controller and
	// run all associated systems once.
	// Note: Controller->run() must be called from a
	// fiber, see https://github.com/chzesa/c-fiber
	czss::defaultController()->run();
	EXITING = true;
}

int main(int argc, char** argv)
{
	czsf::run(fmain);

	thread t1 ([] { while(!EXITING) { czsf_yield(); } });
	thread t2 ([] { while(!EXITING) { czsf_yield(); } });
	while(!EXITING) { czsf_yield(); }

	t1.join();
	t2.join();
}

/* Sample output:
	Reading OS input
	OS input written
	Running game loop
	Player pressed button A
	Game update finished
	Calculating physics
	Updating animations
	Animation update finished
	Rendering
	Rendering finished
	Finished physics update
*/