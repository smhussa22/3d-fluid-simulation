#ifndef BENCHMARK_HH
#define BENCHMARK_HH

#include "Graphics.hh"

// checks the GPU simulation against the CPU one, then times both at increasing particle counts and prints a table (run with --benchmark)
void run_benchmark(const Graphics& graphics);

#endif // BENCHMARK_HH
