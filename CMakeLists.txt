cmake_minimum_required (VERSION 3.0)
project (FASTCALL-BENCHMARKS)

add_compile_options(-Wall -Wextra -g)

add_library(parse-vdso STATIC parse_vdso.c)

target_compile_options(parse-vdso PRIVATE -w)

add_executable (fastcall-benchmarks benchmark.cc)

target_link_libraries(fastcall-benchmarks parse-vdso)

# use commands if you need goolge benchmark lib 
#find_package(benchmark REQUIRED)
#target_link_libraries(fastcall-benchmarks benchmark::benchmark)