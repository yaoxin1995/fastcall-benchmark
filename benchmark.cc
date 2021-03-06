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
#include <sys/auxv.h>
#include <sys/wait.h>
#include <cstring>
struct mesg {
		unsigned long fce_region_address;
		unsigned long secret_region;
		unsigned long hidden_region[10];
};


extern "C" void vdso_init_from_sysinfo_ehdr(uintptr_t base);
extern "C" void *vdso_sym(const char *version, const char *name);




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

static inline void warm_up()
{
    time_before();
    time_after;
}

void write_to_file(std::string f_name, const std::array<uint64_t,SIZE_OF_SAMPLE>  &vec,
            uint64_t mean, uint64_t std_deviation, uint64_t median) 
{


  std::ofstream myfile (f_name);
  if (myfile.is_open()) {
    std::cout << "file: " << f_name << "  saved" << std::endl;
    myfile << "file : " << f_name << " mean: " << mean << std::endl;
    myfile << "file : " << f_name << " std_deviation: " << std_deviation << std::endl;
    myfile << "file : " << f_name << " median: " << median << std::endl;
    for (int count = 0; count < SIZE_OF_SAMPLE; count ++) {
        if(!count%20)
            myfile <<"\n";
        myfile << vec[count] << " " ;
    }
    myfile.close();
  }
  else std::cout << "Unable to open file" << std::endl;

}

uint64_t median(std::array<uint64_t,SIZE_OF_SAMPLE>  &vec)
{
    size_t n = vec.size()/ 2;
    std::nth_element(vec.begin(), vec.begin()+n, vec.end());
    return vec[n];

}


/*
* calculate mean, variance, standard deviation
*/
template<typename T>
std::tuple<T,T,T> cal_variance(std::array<T,SIZE_OF_SAMPLE> &vec) 
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
    std::array<uint64_t,SIZE_OF_SAMPLE>  vec_time;


    for(int i = 0; i < SIZE_OF_SAMPLE; i++) {

        before = time_before();
        long res = syscall(NR_SYS_NOOP, 150);
        after = time_after();

        uint64_t time = after - before;
        if(after - before > 5000 && i >= 1)
            time = vec_time[i-1];
        vec_time[i] = time;
    }

    auto triple= cal_variance(vec_time);

    auto mean = std::get<0>(triple);
    auto std_deviation = std::get<2>(triple);
    auto media = median(vec_time);

    write_to_file("syscall_noop.txt", vec_time, mean, std_deviation, media);
    std::cout << "syscall_noop mean:"<< mean << std::endl;
    std::cout << "syscall_noop std_deviation:"<< std_deviation << std::endl;
    std::cout << "syscall_noop median:"<< media<< std::endl;

}


void ioctl_noop() 
{
    uint64_t before = 0, after = 0;
    struct mesg *message = (struct mesg *)malloc(sizeof(struct mesg));
    std::array<uint64_t,SIZE_OF_SAMPLE> vec_time;


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
        vec_time[i] = time;
    }


    auto triple= cal_variance(vec_time);

    auto mean = std::get<0>(triple);
    auto std_deviation = std::get<2>(triple);


    write_to_file("ioctl_noop.txt", vec_time, mean, std_deviation, median(vec_time));
    std::cout << "ioctl_noop mean:"<< mean << std::endl;
    std::cout << "ioctl_noop std_deviation:"<< std_deviation << std::endl;
    std::cout << "ioctl_noop median:"<< median(vec_time) << std::endl;

    close(fd);

}

void fc_registration()
{
    uint64_t before = 0, after = 0;
    std::array<uint64_t,SIZE_OF_SAMPLE>  vec_reg_time, vec_dreg_time;
    struct mesg *message = (struct mesg *)malloc(sizeof(struct mesg));

    int fd = open(FCE_DEVICE_FILE, O_RDONLY);
	if (fd < 0) 
	    std::cout << "Failed to open device driver!" << std::endl;

    for(int i = 0; i < SIZE_OF_SAMPLE; i++) {
        
        before = time_before();
        int ret = ioctl(fd, IOCTL_REGISTRATION, message);
        after = time_after();

        uint64_t time = after - before;
        if(time > 5000 && i >= 1)
            time = vec_reg_time[i-1];
        vec_reg_time[i] = time;

        before = time_before();
        ret = ioctl(fd, IOCTL_DEREGISTRATION, message);
        after = time_after();

        time = after - before;
        if(time > 5000 && i >= 1)
            time = vec_dreg_time[i-1];
        vec_dreg_time[i] = time;
        
    }
	
    auto reg_triple= cal_variance(vec_reg_time);
    auto reg_mean = std::get<0>(reg_triple);
    auto reg_std_deviation = std::get<2>(reg_triple);

    write_to_file("fc_registration.txt", vec_reg_time, reg_mean, reg_std_deviation, median(vec_reg_time));
    std::cout << "fc_registration mean:"<< reg_mean << std::endl;
    std::cout << "fc_registration std_deviation:"<< reg_std_deviation << std::endl;
    std::cout << "fc_registration median:"<< median(vec_reg_time) << std::endl;


    auto dreg_triple= cal_variance(vec_dreg_time);
    auto dreg_mean = std::get<0>(dreg_triple);
    auto dreg_std_deviation = std::get<2>(dreg_triple);

    write_to_file("fc_deregistration.txt", vec_dreg_time, dreg_mean, dreg_std_deviation, median(vec_dreg_time));
    std::cout << "fc_deregistration mean:"<< dreg_mean << std::endl;
    std::cout << "fc_deregistration std_deviation:"<< dreg_std_deviation << std::endl;
    std::cout << "fc_deregistration median:"<< median(vec_dreg_time) << std::endl;


    close(fd);
    free(message);
    
}

void fc_noop() 
{
    uint64_t before = 0, after = 0;
    std::array<uint64_t,SIZE_OF_SAMPLE>  vec_time;
    struct mesg *message = (struct mesg *)malloc(sizeof(struct mesg));

    int fd = open(FCE_DEVICE_FILE, O_RDONLY);
	if (fd < 0) 
	    std::cout << "Failed to open device driver!" << std::endl;
    int ret = ioctl(fd, IOCTL_REGISTRATION, message);
    fc_ptr fc_noop = (fc_ptr)message->fce_region_address;

    warm_up();
    for(int i = 0; i < SIZE_OF_SAMPLE; i++) {
        
        before = time_before();
        int fce_ret = fc_noop();
        after = time_after();

        uint64_t time = after - before;
        if(time > 70 && i >= 1)
            time = vec_time[i-1];
        vec_time[i] = time;
        
    }
	
    auto triple= cal_variance(vec_time);
    auto mean = std::get<0>(triple);
    auto std_deviation = std::get<2>(triple);

    write_to_file("fc_noop.txt", vec_time, mean, std_deviation, median(vec_time));
    std::cout << "fc_noop mean:"<< mean << std::endl;
    std::cout << "fc_noop std_deviation:"<< std_deviation << std::endl;
    std::cout << "fc_regisfc_nooptration median:"<< median(vec_time) << std::endl;



    ret = ioctl(fd, IOCTL_DEREGISTRATION, message);
    close(fd);
    free(message);
    
}


void vdso_noop() 
{
    const char *VDSO_VERSION = "LINUX_2.6";
    const char *name = "__vdso_getcounter";
    uint64_t before = 0, after = 0;
    std::array<uint64_t,SIZE_OF_SAMPLE>  vec_time;

    //vDSO setup 
    vdso_init_from_sysinfo_ehdr(getauxval(AT_SYSINFO_EHDR));

    getcounter_t func = (getcounter_t)(vdso_sym(VDSO_VERSION, name));
    if (!func) {
		printf("Could not find %s\n", name);
		return;
	}

    for(int i = 0; i < SIZE_OF_SAMPLE; i++) {
        before = time_before();
        int ret = func(VDSO_MEG);
        after = time_after();

        uint64_t time = after - before;
        if(time > 100 && i >= 1)
            time = vec_time[i-1];
        vec_time[i] = time;
    }

    auto triple= cal_variance(vec_time);
    auto mean = std::get<0>(triple);
    auto std_deviation = std::get<2>(triple);


    write_to_file("vdso_noop.txt", vec_time, mean, std_deviation, median(vec_time));
    std::cout << "vdso_noop mean:"<< mean << std::endl;
    std::cout << "vdso_noop std_deviation:"<< std_deviation << std::endl;
    std::cout << "vdso_noop median:"<< median(vec_time) << std::endl;

}


void fork_simple()
{
    uint64_t before, after;
    int i = 0;
    std::array<uint64_t,SIZE_OF_SAMPLE>  vec_time;

    while (i < SIZE_OF_SAMPLE) {
        before = time_before();
        int pid = fork();
        if (pid < 0) {
            std::cout << "fork failed: " << std::strerror(errno) << '\n';
            return;
        } else if (pid == 0)
        exit(0);
        after = time_after();

        if (waitpid(pid, nullptr, 0) < 0) {
            std::cout << "waiting for child failed: " << std::strerror(errno) << '\n';
            return;
        }
        vec_time[i] = after - before;
        ++i;
    }

    auto triple= cal_variance(vec_time);
    auto mean = std::get<0>(triple);
    auto std_deviation = std::get<2>(triple);


    write_to_file("fork_simple.txt", vec_time, mean, std_deviation, median(vec_time));
    std::cout << "fork_simple mean:"<< mean << std::endl;
    std::cout << "fork_simple std_deviation:"<< std_deviation << std::endl;
    std::cout << "fork_simple median:"<< median(vec_time) << std::endl;

}

void fc_fork()
{
    uint64_t before, after;
    int i = 0;
    std::array<uint64_t,SIZE_OF_SAMPLE>  vec_time;

    struct mesg *message = new struct mesg[NR_FC_FORK];

    int fd = open(FCE_DEVICE_FILE, O_RDONLY);
	if (fd < 0) 
	    std::cout << "Failed to open device driver!" << std::endl;
    int ret = ioctl(fd, IOCTL_REGISTRATION, message);

    for(int j = 0; j < NR_FC_FORK; j++) {
        int ret = ioctl(fd, IOCTL_REGISTRATION, message[j]);
    }


    while (i < SIZE_OF_SAMPLE) {
        before = time_before();
        int pid = fork();
        if (pid < 0) {
            std::cout << "fork failed: " << std::strerror(errno) << '\n';
            return;
        } else if (pid == 0)
        exit(0);
        after = time_after();

        if (waitpid(pid, nullptr, 0) < 0) {
            std::cout << "waiting for child failed: " << std::strerror(errno) << '\n';
            return;
        }
        vec_time[i] = after - before;
        ++i;
    }



    auto triple= cal_variance(vec_time);
    auto mean = std::get<0>(triple);
    auto std_deviation = std::get<2>(triple);


    write_to_file("fc_fork.txt", vec_time, mean, std_deviation, median(vec_time));
    std::cout << "fc_fork mean:"<< mean << std::endl;
    std::cout << "fc_fork std_deviation:"<< std_deviation << std::endl;
    std::cout << "fc_fork median:"<< median(vec_time) << std::endl;

    close(fd);
    delete[] message;
}


int main() 
{
    //ioctl_noop();

    //syscall_noop();

    //vdso_noop();

    //fc_registration();

    //fc_noop(); 

    //fork_simple();

    fc_fork();
}