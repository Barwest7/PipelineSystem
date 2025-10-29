#include "consumer_producer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// After some testing I found a race condition that caused
// consumer_producer_get() to be accesed by multiple threads 
// and caused data corruption
// So I decided to use a mutex to solve this
// Initially I used a global mutex that was shared within all queues
// But I eventaully decided that each queue will have their own lock, so
// The mutex was added to the queue struct

const char* consumer_producer_init (consumer_producer_t* queue, int capacity) {
    
    // Safety check
    if (capacity <= 0) {
        return "Error, the given capacity must be positive";
    }
    
    // Another safety check
    if (queue == NULL) {
        return "Error, the given queue ptr is null";
    }
    
    // Initialize the parameters of the queue
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    
    // Allocate the memory for the items array within the queue struct
    // according to the capacity
    queue->items = (char**)malloc(sizeof(char*) * capacity);
    if (queue->items == NULL) {
        return "Error, failed to allocate memory for items array";
    }
    
    // Initialize all the monitors
    // Upon failure return error messages accordingly
    if (monitor_init(&queue->not_full_monitor) != 0) {
        free(queue->items);
        return "Error, couldnt initialize not_full_monitor";
    }
    
    // Here we also destroy the monitor that was already initialized
    if (monitor_init(&queue->not_empty_monitor) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);
        return "Error, couldnt initialize not_empty_monitor";
    }

    // Here we also destroy the monitors that were already initialized
    if (monitor_init(&queue->finished_monitor) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        monitor_destroy(&queue->not_empty_monitor);
        free(queue->items);
        return "Error, couldnt initialize finished_monitor";
    }
    
    // Here we also destroy the monitors that were already initialized
    if (pthread_mutex_init(&queue->queueLock, NULL) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        monitor_destroy(&queue->not_empty_monitor);
        monitor_destroy(&queue->finished_monitor);
        free(queue->items);
        return "Error, couldnt initialize the queue mutex";
    }
    
    // In the initialization the queue is obviously not full
    // so we signal the not_full_monitor
    monitor_signal(&queue->not_full_monitor);
    
    // If we reached here, everything was initialized successfully
    // Upon success, return null
    return NULL; 
}


void consumer_producer_destroy (consumer_producer_t* queue) {
    
    // Safety check
    if (queue == NULL) {
        return;
    }
    
    // Free all the remaining items that are in the queue
    for (int i=0; i<queue->count; i++) {
        int indexToCheck = (queue->head + i) % queue->capacity;
        if (queue->items[indexToCheck] != NULL) {
            free(queue->items[indexToCheck]);
            queue->items[indexToCheck] = NULL;
        }
    }
    
    // Free the items array (the array itself)
    if (queue->items != NULL) {
        free(queue->items);
        queue->items = NULL;
    }
    
    // Destroy all the monitors using the funciton from the monitor implementation
    monitor_destroy(&queue->not_empty_monitor);
    monitor_destroy(&queue->finished_monitor);
    monitor_destroy(&queue->not_full_monitor);

    // Destroy the mutex
    pthread_mutex_destroy(&queue->queueLock);
    
    // Reset queue state to the beginning (everything set to 0)
    queue->capacity = 0;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
}

const char* consumer_producer_put (consumer_producer_t* queue, const char* item) {
    
    // Safety check
    if (item == NULL) {
        return "Passed a null item pointer";
    }
    
    // Yet another safety check
    if (queue == NULL) {
        return "Passed a null queue pointer";
    }

    // Lock, so that no other thread will be able to modify queue
    pthread_mutex_lock(&queue->queueLock);

    
    // Wait until queue is not full
    while (queue->count >= queue->capacity) {
        
        // I unlock the mutex before access to monitor because consumer
        // threads now need to access the queue in order to remove itms
        pthread_mutex_unlock(&queue->queueLock);

        // Upon failure return failure message
        if (monitor_wait(&queue->not_full_monitor) != 0) {
            return "Error, failed to wait on not_full_monitor";
        }

        // After waking up from the monitor wait we reacquire the lock
        pthread_mutex_lock(&queue->queueLock);

    }
    
    // Allocate memory and make a copy of the string
    char* copiedItem = malloc(strlen(item) + 1);
    if (copiedItem == NULL) {
        
        // Release the lock before leaving the function (because we have return)
        pthread_mutex_unlock(&queue->queueLock);
        return "Error, failed to allocate memory for th item copy";
    }
    strcpy(copiedItem, item);
    
    // Add the item to the queue
    queue->items[queue->tail] = copiedItem;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    // Signal, using the not empty montiro, that queue is not empty 
    monitor_signal(&queue->not_empty_monitor);
    
    // If the queue is now full, reset the not full monitor
    if (queue->count >= queue->capacity) {
        monitor_reset(&queue->not_full_monitor);
    }

    // Unlock. We are done with the queue so now other threads are free to use it
    pthread_mutex_unlock(&queue->queueLock);
    
    // Upon succes we reach here and return null
    return NULL;
}

char* consumer_producer_get (consumer_producer_t* queue) {
    
    // Safety check
    if (queue == NULL) {
        return NULL;
    }

    // Lock so no othe threads will be able to reach the queue and chagne it
    pthread_mutex_lock(&queue->queueLock);
    
    // Wait until the queue is not empty, using the 
    // not empty monitor
    while (queue->count<=0) {

        // Unlock to allow producers access to the queue
        // and signal the not empty monitor 
        pthread_mutex_unlock(&queue->queueLock);
        if (monitor_wait(&queue->not_empty_monitor) != 0) {
            return NULL;
        }

        // Lock again so that no other threads will be able to reach the queue
        pthread_mutex_lock(&queue->queueLock);
    }
    
    // Get the item from the queue
    char* item = queue->items[queue->head];
    
    // Clear the slot where the item was in the queue
    // while maintianing the circular sturcture of the queue
    queue->items[queue->head] = NULL;

    // Update the head pointer
    queue->head = (queue->head + 1) % queue->capacity;

    // Decrease the amount of itemms in the queue
    queue->count--;
    
    // Signal that queue is not full (we removed an item from it)
    // this is for waiting producers
    monitor_signal(&queue->not_full_monitor);
    
    // If the queue is now empty, reset the not_empty_monitor
    // so that consumers coming in the future will wait until
    // the queue has items
    if (queue->count == 0) {
        monitor_reset(&queue->not_empty_monitor);
    }

    // We are done so we can now unlock and allow others to reach the queue
    pthread_mutex_unlock(&queue->queueLock);
    
    // Return the item to the caller
    // the caller must free it
    return item;
}

void consumer_producer_signal_finished (consumer_producer_t* queue) {
    
    // Safety check
    if (queue == NULL) {
        return;
    }
    
    // Call montor signal on the finished monitor of the queue (broadcast that done)
    monitor_signal(&queue->finished_monitor);
}

int consumer_producer_wait_finished (consumer_producer_t* queue) {
    
    // Safety check
    if (queue == NULL) {
        return -1;
    }
    
    // Call monitor wait on the inished monitor of the queue
    // this will block the caller until some thread calls signal finished (the function before)
    // and will return 0 (return from monitor_wait) upon success or -1 (again from monitor wait)
    // upon failure
    int result = monitor_wait(&queue->finished_monitor);
    return result;
}