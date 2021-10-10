cmake_minimum_required (VERSION 3.0)
project (FASTCALL-BENCHMARKS)

add_compile_options(-Wall -Wextra)


add_executable (fastcall-benchmarks benchmark.cc)

# use commands if you need goolge benchmark lib 
#find_package(benchmark REQUIRED)
#target_link_libraries(fastcall-benchmarks benchmark::benchmark)