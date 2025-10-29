#include "monitor.h"

// All functions functionalities are described in detail 
// in the header file

int monitor_init (monitor_t* monitor) {
    
    // Avoid segmentation fault in case there is no monitor
    if (!monitor) { return -1; }

    // Initialize the mutex lock
    if (pthread_mutex_init(&monitor->mutex, NULL) != 0) {
        return -1;
    }
    
    // Initialize the condition variable
    if (pthread_cond_init(&monitor->condition, NULL) != 0) {
        pthread_mutex_destroy(&monitor->mutex);
        return -1;
    }
    
    // Initialize the monitor's signaled state to be false
    monitor->signaled = 0;
    
    // We reached here so nothing went wrong so we return 0
    return 0;
}

void monitor_destroy (monitor_t* monitor) {
    
    // Avoid segmentation fault in case there is no monitor
    if (!monitor) { return; }
    
    // Free all the resources by destroing them using their own functions   
    pthread_cond_destroy(&monitor->condition);
    pthread_mutex_destroy(&monitor->mutex);
    monitor->signaled = 0;
}

void monitor_signal (monitor_t* monitor) {
    
    // Avoid segmentation fault in case there is no monitor
    if (!monitor) {
        return;
    }
    
    // Acquire the lock, Set the monitor signaled state to be 1 
    // Wake up one blocked thread and then set the lock free
    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 1;
    pthread_cond_signal(&monitor->condition);
    pthread_mutex_unlock(&monitor->mutex);
}

void monitor_reset (monitor_t* monitor) {
    
    // Avoid segmentation fault in case there is no monitor
    if (!monitor) { return; }
    
    // Acquire the lock, set monitor signaled state to be 0 
    // and release the lock.
    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 0;
    pthread_mutex_unlock(&monitor->mutex);
}

int monitor_wait (monitor_t* monitor) {

    // Avoid segmentation fault in case there is no monitor
    if (!monitor) { return -1; }
    
    // Acqquire the lock
    pthread_mutex_lock(&monitor->mutex);
    
    // Wait while not signaled (means that the condition hasnt been triggered yet)
    // Loop is for spurious wake ups (cond_wait return wout a signal)
    // or maybe if a different thread tries to consume the signal before
    while (!monitor->signaled) {

        // Release the lock and wait for the cond signal
        // Lock is reacquired when cond_wait returns
        if (pthread_cond_wait(&monitor->condition, &monitor->mutex) != 0) {
            
            // Upon fail, releas the lock and return -1
            pthread_mutex_unlock(&monitor->mutex);
            return -1;
        }
    }
    
    // Upon success, release the lock and return 0
    pthread_mutex_unlock(&monitor->mutex);
    return 0;
}