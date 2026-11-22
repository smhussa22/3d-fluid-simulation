#ifndef BENCHMARK_HH
#define BENCHMARK_HH

#include "Graphics.hh"

// checks the GPU simulation against the CPU one, then times both at increasing particle counts and prints a table (run with --benchmark)
void run_benchmark(const Graphics& graphics);

// drops a cube of about `particles` particles into its tank and prints, every so often, how long each GPU pass takes and how crowded the water is (run with --profile N)
void run_profile(const Graphics& graphics, int particles);

#endif // BENCHMARK_HH
