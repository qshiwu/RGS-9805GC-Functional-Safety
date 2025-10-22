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

volatile uint32_t *mmio_ptr;  // Pointer to MMIO register

// Timer handler function that writes to the MMIO register every 500us
void timer_handler(int signum) {
    // Write a value to the MMIO register
    *mmio_ptr = *mmio_ptr ^ 0x00400000;		//toggle bit 22
    uint32_t value = *mmio_ptr;
   // printf("Readvalue : 0x%08X\n", value & 0x10000);
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
    mmio_ptr = (volatile uint32_t *)(mapped_base + 0x204);		//0xFED81510, SYS_HEARTBEAT (CPU_AGPIO4) to MCU_GPI_5 (MCU GPI[2]/ FLEXIO_D4)
    
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
