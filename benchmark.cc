#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include "benchmark.h"
#include <numeric>
#include <cmath>
#include <tuple>


struct mesg {
		unsigned long yellow_address;
		unsigned long purple_address;
		unsigned long green_address;
};




static inline uint64_t time_before(void)
{
    uint64_t rax, rdx;

    asm volatile ( 
        "cpuid\n\t"
        :::"%rax", "%rbx", "%rcx", "%rdx");

    asm volatile ( 
        "rdtsc\n\t"
        : "=a" (rax), "=d" (rdx)  ::);
    return (rdx << 32) + rax;
}

static inline uint64_t time_after(void)
{
    uint64_t rax, rdx;
    
    asm volatile ( 
        "rdtscp\n\t" 
        : "=a" (rax), "=d" (rdx) ::);
    
    asm volatile ( 
        "cpuid\n\t"
        :::"%rax", "%rbx", "%rcx", "%rdx");
    return (rdx << 32) + rax;
}



void write_to_file(std::string f_name, const std::vector<uint64_t> &vec) 
{


  std::ofstream myfile (f_name);
  if (myfile.is_open()) {
    std::cout << "file opened" << std::endl;
    myfile << "This is a line.\n";
    myfile << "This is another line.\n";
    for (int count = 0; count < SIZE_OF_SAMPLE; count ++) {
        if(!count%20)
            myfile <<"\n";
        myfile << vec[count] << " " ;
    }
    myfile.close();
  }
  else std::cout << "Unable to open file" << std::endl;

}

uint64_t median(std::vector<uint64_t> &vec)
{
    size_t n = vec.size()/ 2;
    std::nth_element(vec.begin(), vec.begin()+n, vec.end());
    return vec[n];

}


/*
* calculate mean, variance, standard deviation
*/
template<typename T>
std::tuple<T,T,T> cal_variance(const std::vector<T> &vec) 
{
    const size_t sz = vec.size();
    if (sz == 1) 
        return {0,0,0};
    

    // Calculate the mean
    T mean = std::accumulate(vec.begin(), vec.end(), 0.0) / sz;

    // Now calculate the variance
    auto variance_func = [&mean, &sz](T accumulator, const T& val) {
        return accumulator + ((val - mean)*(val - mean) / (sz-1));
    };

    T variance = std::accumulate(vec.begin(), vec.end(), 0.0, variance_func);

    T std_deviation = sqrt(variance);


    return {mean, variance, std_deviation};
}



void syscall_noop() 
{
    uint64_t before = 0, after = 0;
    std::vector<uint64_t> vec_time;


    for(int i = 0; i < SIZE_OF_SAMPLE; i++) {

        before = time_before();
        long res = syscall(NR_SYS_NOOP, 150);
        after = time_after();

        uint64_t time = after - before;
        if(after - before > 5000 && i >= 1)
            time = vec_time[i-1];
        vec_time.push_back(time);
    }

    auto triple= cal_variance(vec_time);

    auto mean = std::get<0>(triple);
    auto variance = std::get<1>(triple);
    auto std_deviation = std::get<2>(triple);

    write_to_file("syscall_noop.txt", vec_time);
    std::cout << "syscall_noop mean:"<< mean << std::endl;
    std::cout << "syscall_noop std_deviation:"<< std_deviation << std::endl;
    std::cout << "syscall_noop median:"<< median(vec_time) << std::endl;

}


void ioctl_noop() 
{
    uint64_t before = 0, after = 0;
    struct mesg *message = (struct mesg *)malloc(sizeof(struct mesg));
    std::vector<uint64_t> vec_time;


    int fd = open(FCE_DEVICE_FILE, O_RDONLY);
	if (fd < 0) 
	    std::cout << "Failed to open device driver!" << std::endl;
	

    for(int i = 0; i < SIZE_OF_SAMPLE; i++) {
        

        before = time_before();
        int ret = ioctl(fd, 2, message);
        after = time_after();

        uint64_t time = after - before;
        if(time > 5000 && i >= 1)
            time = vec_time[i-1];
        vec_time.push_back(time);
    }


    auto triple= cal_variance(vec_time);

    auto mean = std::get<0>(triple);
    auto variance = std::get<1>(triple);
    auto std_deviation = std::get<2>(triple);


    write_to_file("ioctl_noop.txt", vec_time);
    std::cout << "ioctl_noop mean:"<< mean << std::endl;
    std::cout << "ioctl_noop std_deviation:"<< std_deviation << std::endl;
    std::cout << "ioctl_noop median:"<< median(vec_time) << std::endl;

    close(fd);

}


int main() 
{
    ioctl_noop();

    syscall_noop();
}
