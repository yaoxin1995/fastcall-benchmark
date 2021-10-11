# FAST CALL BENCHMARK
Benchmarks for the comparison of the fastcall mechanism with system calls, ioctl etc. These benchmarks use the RDTSC/RDTSCP.
## BUILD
```
$ mkdir build

$cd build

$cmake ../build

$sudo taskset -c 1 ./fastcall-benchmarks

```