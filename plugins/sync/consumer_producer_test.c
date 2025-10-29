#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "consumer_producer.h"

// Configuration
#define QueueSize 5
#define NumThreads 4
#define ItemCount 20

// Global queue
consumer_producer_t* test_queue;

// Producer thread, should add items to the queue
void* producer (void* arg) {
    int num = *(int*)arg;
    char item[32];
    
    for (int i=0; i<num; i++) {
        snprintf(item, sizeof(item), "item-%d", i);
        consumer_producer_put(test_queue, item);
    }
    return NULL;
}

// Consumer thread, should take items from the queue
void* consumer (void* arg) {
    int num = *(int*)arg;
    
    for (int i=0; i<num; i++) {
        char* item = consumer_producer_get(test_queue);
        if (item) free(item);
    }
    return NULL;
}

// Test 1
// Put and get
int testPutGet () {
    printf("Test 1: Put and get: ");
    
    consumer_producer_t queue;
    if (consumer_producer_init(&queue, 3) != NULL) {
        printf("Failed to initialzie \n");
        return 0;
    }
    
    // Put the item
    if (consumer_producer_put(&queue, "test") != NULL) {
        printf("Failed to put \n");
        consumer_producer_destroy(&queue);
        return 0;
    }
    
    // Get the item
    char* item = consumer_producer_get(&queue);
    if (!item || strcmp(item, "test") != 0) {
        printf("Failed to get \n");
        if (item) free(item);
        consumer_producer_destroy(&queue);
        return 0;
    }
    
    free(item);
    consumer_producer_destroy(&queue);
    printf("Pass\n");
    return 1;
}

// Test 2
// Fill the queue completely
int testFillQueue () {
    printf("Test 2: Full queue: ");
    
    consumer_producer_t queue;
    if (consumer_producer_init(&queue, 2) != NULL) {
        printf("Failed to initialzie \n");
        return 0;
    }
    
    // Fill the queue
    consumer_producer_put(&queue, "first in");
    consumer_producer_put(&queue, "second in");
    
    // Get the items back
    char* itemOne = consumer_producer_get(&queue);
    char* itemTwo = consumer_producer_get(&queue);
    
    int toReturn = itemOne && itemTwo && 
             strcmp(itemOne, "first in") == 0 && 
             strcmp(itemTwo, "second in") == 0;
    
    if (itemOne) free(itemOne);
    if (itemTwo) free(itemTwo);
    consumer_producer_destroy(&queue);
    
    printf(toReturn ? "Pass\n" : "Fail\n");
    return toReturn;
}

// Test 3
// Invalid parameters
int testInvalidParams () {
    printf("Test 3: Errors handling: ");
    
    consumer_producer_t queue;
    
    // This should fail
    if (consumer_producer_init(NULL, 10) == NULL ||
        consumer_producer_init(&queue, 0) == NULL ||
        consumer_producer_init(&queue, -1) == NULL) {
        printf("Fail: should reject non valid paramaters\n");
        return 0;
    }
    
    printf("Pass\n");
    return 1;
}

// Test 4
// Multiple threads
int tstMuiltiThreads () {
    printf("Test 4: Multiple threads: ");
    
    consumer_producer_t queue;
    if (consumer_producer_init(&queue, QueueSize) != NULL) {
        printf("Failed to initialzie \n");
        return 0;
    }
    
    test_queue = &queue;
    pthread_t threads[NumThreads];
    
    // Make sure total items produced is the same as total items consumed
    int producers = NumThreads / 2;
    int consumers = NumThreads - producers;
    int itemsPerProducer = ItemCount / producers;
    int itemsPerConsumer = ItemCount / consumers;
    
    // Start the producers first
    for (int i = 0; i < producers; i++) {
        pthread_create(&threads[i], NULL, producer, &itemsPerProducer);
    }
    
    // Then start the consumers
    for (int i = producers; i < NumThreads; i++) {
        pthread_create(&threads[i], NULL, consumer, &itemsPerConsumer);
    }
    
    // Wait for all the threads
    for (int i=0; i<NumThreads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    consumer_producer_destroy(&queue);
    printf("Pass\n");
    return 1;
}

// Test 5
// Check that the finish signal works fine
int testFinishSignal () {
    printf("Test 5: Finish signal: ");
    
    consumer_producer_t queue;
    if (consumer_producer_init(&queue, 3) != NULL) {
        printf("Failed to initialzie \n");
        return 0;
    }
    
    consumer_producer_signal_finished(&queue);
    int result = consumer_producer_wait_finished(&queue);
    
    consumer_producer_destroy(&queue);
    
    if (result != 0) {
        printf("Fail, wait failed\n");
        return 0;
    }
    
    printf("Pass \n");
    return 1;
}

int main () {
    printf("Consumer-Producer Unit Test \n");
    printf("Configuration: queue=%d, threads=%d, items=%d\n\n", 
           QueueSize, NumThreads, ItemCount);
    
    int passed = 0;
    passed += testPutGet();
    passed += testFillQueue();
    passed += testInvalidParams();
    passed += tstMuiltiThreads();
    passed += testFinishSignal();
    
    printf("\n%d/5 tests passed\n", passed);
    return (passed == 5) ? 0 : 1;
}