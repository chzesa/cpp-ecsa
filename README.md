# cpp-ecsa

Facilitates easy building of a coarsely parallelized ECS-architecture by defining read and write access to components, as well as dependencies between systems, using inheritance.

## Building the example

Linux:

```sh
clang++ -O1 example.cpp -pthread
```