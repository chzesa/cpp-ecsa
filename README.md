# cpp-ecsa

Facilitates easy building of a coarsely parallelized ECS-architecture by defining read and write access, as well as dependencies between systems, using inheritance.

## Building the example

Linux:

```sh
g++ -I external/c-fiber/ example.cpp -pthread
```

## Usage

Add the following to a __single__ `.cpp` file (Note: usage requires [c-fiber](https://github.com/chzesa/c-fiber), which is included as a submodule)

```c++
#define CZSS_IMPLEMENTATION
#include "czss.hpp"
```

### Systems and controllers

The library provides means to create `System`s which are executed in parallel, whenever possible, by the `System`'s `Controller`. The `Controller` is either the static default `Controller`, or one passed to the `System` in its constructor.

### Defining systems

To define a `System` inherit from `System<T>` as follows:

```c++
struct SomeSystem : czss::System<SomeSystem>
{
	void run() const override
	{
		// This function is called by the controller
	}
	/* ... */
}
```

Then, to run all of the `System`s once:

```c++
struct A : czss::System<A> { /* ... */ };
struct B : czss::System<B> { /* ... */ };
struct C : czss::System<C> { /* ... */ };

/* ... */

	// Instantiate systems to make the controller aware of them
	A a = {};
	B b = {};
	C c = {};

/* ... */

	Controller* ctrl = czsf::defaultController();
	ctrl->run(); // Calls a->run(), b->run(), c->run() once
```

With `System`s defined as follows:

```c++
struct A : czss::System<A>
{
	A(Controller* ctrl) : System<A>(ctrl) { /* ... */ }
	/* ... */
};

struct B : czss::System<B>
{
	B(Controller* ctrl) : System<B>(ctrl) { /* ... */ }
	/* ... */
};

struct C : czss::System<C> { /* ... */ }
```

They can be used with different `Controller`s:

```c++
	Controller first = {};
	Controller second = {};
	A a(&first);
	A s(&second);
	B b(first);
	C c = {};
	
	first.run() // Calls a->run(), b->run()
	second.run() // Calls s->run()
	defaultController()->run() // calls c->run()
```

### System dependencies

To control access to shared data, `System`s can define read and write accesses other `System`s controlled by the same `Controller` by inheriting from `Reader<T>` or `Writer<T>`.

The object provided by a `Reader<T>` or `Writer<T>` must have been instantiated, and controlled by the same `Controller` as the reader or writer.

```c++
struct D : czss::System<D> { /* ... */ };
struct E : czss::System<E> { /* ... */ };

struct A : czss::System<A>, czss::Reader<D>
{
	void run() const override
	{
		const D* d = Reader<D>::get();
	}
};

struct B : czss::System<B>, czss::Writer<E>
{
	void run() const override
	{
		E* e = Reader<E>::get();
	}
};

```

If `System`s `A` and `B` attempt to read and write, or both write, to the same `System`, then there must exist an implicit or explicit dependency between `A` and `B`. To declare dependencies, inherit from `Dependency<T>`:

```c++
struct D : czss::System<D>;
struct A : czss::System<A>, czss::Writer<D>;

struct B : czss::System<B>, czss::Writer<D>, czss::Dependency<A>;
// B depends on A

struct C : czss::System<C>, czss::Writer<D>, czsss::Dependency<B>;
// Since C depends on B, and B depends on A, C implicitly depends on A
```

The following will cause a `std::logic_error` due to missing dependencies:

```c++
struct D : czss::System<D>;
struct A : czss::System<A>, czss::Reader<D>;
struct B : czss::System<B>, czss::Writer<D>;
```

__Note: The system object must inherit from `System<T>` before `Reader<T>`, `Writer<T>`, or `Dependency<T>`__

### Guarantees and parallelism

Please see the [c-fiber example](https://github.com/chzesa/c-fiber/blob/master/example.cpp) and [header documentation](https://github.com/chzesa/c-fiber/blob/master/czsf.h) first.

When `controller->run()` is called, the validity of the definitions and dependencies is checked once, and then again if another `System` is added to the controller (i.e. instantiated). Each `System` is run once per invocation of `controller->run()` as its own fiber task.

When `System` `A`'s run function is called, all of its dependencies have finished executing and none of its dependencies begin execution until `A` is finished. Furthermore, no `System` `B` that reads from a `System` that `A` writes to, or writes to a `System` that `A` reads from or writes to, executes in parallel with `A`.

`System` `A` must only guarantee that it has finished reading and writing to all other `System`s by the time its `run` function returns. Thus, further parallelism can be achieved by diving the `System`'s work into their own fiber tasks if feasible.