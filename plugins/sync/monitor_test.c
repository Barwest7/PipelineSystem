#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include "monitor.h"

// Global monitor
static monitor_t* testMonitor;

// A generic thread that just signals the monitor and then exits
// it gets delay_ptr which is a void pointer - as required by pthread
// if its null then we use 0 delay. otherwise, the value pointed to
void* signaler_thread (void* delay_ptr) {

    // Dereference the pointer an cast to int to get the number of delay
    // If null, then do not delay (set to 0)
    int delay = delay_ptr ? *(int*)delay_ptr : 0;
    
    // Delay for as long as given
    if (delay > 0) {
        usleep(delay * 1000);
    }
  
    // Signal the monitor and exit
    monitor_signal(testMonitor);
    return NULL;
}

// Another gneric thread. this one waits on the monitor
// and retunrs success or failure through the return value
void* waiter_thread (void* unused) {
    
    // This is to make the warning about unused to disappear
    (void)unused;
    
    // Call wait on the test monitor and return the result
    int result = monitor_wait(testMonitor);
    
    // The result is int, convert it to void pointer and return it
    // for pthread return
    return (void*)(long)result;
}

// Test 1: Signal before wait
// this should return right away ("remember" that signal was called)
void testSignalBeforeWait () {
    printf("Test 1: Signal before wait: ");
    
    monitor_t monitor;

    // Test monitor creation succesful
    assert(monitor_init(&monitor) == 0);
    
    // Signal
    monitor_signal(&monitor);
    
    // Wait should return right away. Upon failure - assert will let us know
    assert(monitor_wait(&monitor) == 0);
    monitor_destroy(&monitor);
    
    // Test passed if reached here
    printf("pass\n");
}

// Test 2: Basic threads signal wait
// Real scenario: Consumer arrives, waits. Producer does something (simulated by sleeping)
// producer signals when its done, then the consumer wakes up and proceeds
void testSignalWait () {
    printf("Test 2: Basic signal wait: ");
    
    // Yet again create a monitor and initialize it, make sure it was initalized ok
    monitor_t monitor;
    assert(monitor_init(&monitor) == 0);

    // Assign the address of the monitor I just created to the global test monitor
    // so that signal thread will know which monitor he should signal
    // this will expose the monitor to the signaling thread through the global ptr
    testMonitor = &monitor;
    
    // 50ms delay
    int delay = 50;
    
    // Create thread that will signal after the specified delay
    pthread_t thread;
    pthread_create(&thread, NULL, signaler_thread, &delay);
    
    // Wait for signal (on any error assert will stop and throw an error)
    assert(monitor_wait(&monitor) == 0);
    
    // Join makes the test wait until the signaling thread finishes
    pthread_join(thread, NULL);

    // Clean up and report
    monitor_destroy(&monitor);
    printf("pass\n");
}

// Test 3: Test that reset clears the signaled state
void testReset () {
    printf("Test 3: Reset: ");
    
    // Create monitor and initialize it...
    monitor_t monitor;
    assert(monitor_init(&monitor) == 0);
    
    // Signal and then reset (signaled = 1 and then signaled = 0)
    monitor_signal(&monitor);
    monitor_reset(&monitor);
    
    // Assign the address of the monitor I just created to the global test monitor
    // this will expose the monitor to the signaling thread through the global ptr
    testMonitor = &monitor;

    // Create a delay of 50ms
    int delay = 50;
    
    // Create a new thread, it will sleep for 50ms and then signal
    // it should block until new signal
    pthread_t thread;
    pthread_create(&thread, NULL, signaler_thread, &delay);
    
    // Main thread will now wait for the monitor. as stated before
    // should block until new signal
    assert(monitor_wait(&monitor) == 0);
    
    pthread_join(thread, NULL);
    monitor_destroy(&monitor);
    printf("pass\n");
}

// Test 4: Multiple waiters where only one should wake up
void testMultiWaiters () {
    printf("Test 4: Multiple waiters with one signal: ");
    
    // Like in previous tests
    monitor_t monitor;
    assert(monitor_init(&monitor) == 0);
    testMonitor = &monitor;
    
    // Create miultiple threads that will be waiting
    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, waiter_thread, NULL);
    pthread_create(&t2, NULL, waiter_thread, NULL);
    pthread_create(&t3, NULL, waiter_thread, NULL);
    
    // Wait for them to start and block the monitor
    usleep(50000);
    
    // Signal once, only one of the waiting thread should wake up
    monitor_signal(&monitor);
    usleep(50000); // Let one thread wake up
        
    // Count how many threads have completed
    // I use tryjoin to see if they exited without blocking main
    void* result1, *result2, *result3;
    int awake = 0;
    if (pthread_tryjoin_np(t1, &result1) == 0) awake++;
    if (pthread_tryjoin_np(t2, &result2) == 0) awake++;
    if (pthread_tryjoin_np(t3, &result3) == 0) awake++;
    
    // Exactly one should have completed
    assert(awake == 1);
    
    // Wake up the others by signaling
    monitor_signal(&monitor);
    monitor_signal(&monitor);
    
    // Clean up all the threads
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    
    monitor_destroy(&monitor);
    printf("pass\n");
}

// Test 5: NULL pointer
void testNullPtr () {
    printf("Test 5: null pointer:  ");
    
    // Initalize with null
    assert(monitor_init(NULL) == -1);
    assert(monitor_wait(NULL) == -1);
    
    // Shouldnt crash
    monitor_signal(NULL);
    monitor_reset(NULL);
    monitor_destroy(NULL);
    
    printf("pass\n");
}

// Test 6: Double initialization
void testTwoInit () {
    printf("Test 6: Initialize twice: ");
    
    monitor_t monitor;
    assert(monitor_init(&monitor) == 0);

    // Should succeed (second initialization)
    assert(monitor_init(&monitor) == 0);
    
    monitor_destroy(&monitor);
    printf("pass\n");
}

int main () {
    printf("Monitor Unit Test\n\n");
    
    testSignalBeforeWait();
    testSignalWait();
    testReset();
    testMultiWaiters();
    testNullPtr();
    testTwoInit();
    
    printf("\nAll the tests passed\n");
    return 0;
}