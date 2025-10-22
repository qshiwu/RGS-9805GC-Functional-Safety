#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#define MMIO_BASE_ADDRESS 0xFED81000  // Replace with your MMIO address
#define MMIO_SIZE 0x1000  // 4KB mapping, adjust if needed

volatile uint32_t *HandShake_R;  // Pointer to MMIO register
volatile uint32_t *HandShake_W;  // Pointer to MMIO register

void HandShakeOut(uint32_t input)
{
    if(input == 0x10000)
    {
        *HandShake_W = *HandShake_W | 0x00400000;   //high
    }
    else
    {
        *HandShake_W = *HandShake_W & ~0x00400000;  //Low
    }
}

// Timer handler function that writes to the MMIO register every 500us
void timer_handler(int signum) {
    // Write a value to the MMIO register
    uint32_t value = *HandShake_R;
    value &= 0x10000;   //check bit 16 1:high 0:low
    HandShakeOut(value);
}

int main() {
    int fd;
    void *mapped_base;
    struct itimerval timer;

    // Open /dev/mem to access physical memory
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("Error opening /dev/mem");
        return -1;
    }

    // Map the MMIO address into user space
    mapped_base = mmap(0, MMIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, MMIO_BASE_ADDRESS);
    if (mapped_base == MAP_FAILED) {
        perror("Error mapping MMIO address");
        close(fd);
        return -1;
    }

    // Get the pointer to the desired MMIO register
    HandShake_R = (volatile uint32_t *)(mapped_base + 0x540);		//0xFED81540, CPU GPIO16, MCU PTD16
    HandShake_W = (volatile uint32_t *)(mapped_base + 0x514);		//0xFED81514, CPU GPIO5, MCU PTA12
    
    // Set up the signal handler for the timer
    signal(SIGALRM, timer_handler);

    // Configure the timer to trigger every 500us
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 500;  // First trigger after 500us
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 500;  // Repeat every 500us

    // Start the timer
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("Error setting timer");
        return -1;
    }

    // Wait for timer events (the timer handler will be called every 1ms)
    while (1) {
        pause();  // Wait for signals
    }

    // Clean up (this code won't be reached in this example)
    if (munmap(mapped_base, MMIO_SIZE) == -1) {
        perror("Error unmapping memory");
    }
    close(fd);

    return 0;
}
