#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "plugin_common.h"

// Global plugin context
// each plugin shared object will have its own instance
// Initailized with all struct members to 0
static plugin_context_t g_plugin_context = {0};

void* plugin_consumer_thread (void* arg) {
    plugin_context_t* pluginContext = (plugin_context_t*)arg;
    
    // Contine to procces items the queue unitl <END>
    while (1) {
        
        // Get item from the queue 
        // blocks if empty
        char* itemFromQueue = consumer_producer_get(pluginContext->queue);
        
        // This shouldnt happen so its a safety check
        if (itemFromQueue == NULL) {
            log_error(pluginContext, "Received NULL item from queue");
            break;
        }
        
        // Check if recieved <END>
        if (strcmp(itemFromQueue, "<END>") == 0) {
            
            // Process the <END> signal first
            if (pluginContext->next_place_work != NULL) {
            
                // Pass end signal (<END>) to the next plugin
                const char* error = pluginContext->next_place_work(itemFromQueue);
                if (error != NULL) {
                    log_error(pluginContext, error);
                }
            }
            
            // Free the <END> string
            free(itemFromQueue);
            
            // Set the finished flag to 1
            // Also signal completion
            pluginContext->finished = 1;
            consumer_producer_signal_finished(pluginContext->queue);
            break;
        }
        
        // Now we reached here so its not the end string
        // Process the string using the required plugin function
        const char* proccessedString = pluginContext->process_function(itemFromQueue);
        
        // Free the original item because we are done with it
        free(itemFromQueue);
        
        // In case there is a next plugin we send it the string that we processed
        if (pluginContext->next_place_work != NULL) {
            const char* error = pluginContext->next_place_work(proccessedString);
            if (error != NULL) {
                log_error(pluginContext, error);
                
                // Free the processed string in case it was not able to be sent
                if (proccessedString != NULL) {
                    free((char*)proccessedString);
                }
            }

        } 
        
        else {
            
            // This is the lest plugin, we free it
            if (proccessedString != NULL) {
                free((char*)proccessedString);
            }
        }
    }
    
    return NULL;
}

// Print according to the given format from the .h file
void log_error (plugin_context_t* context, const char* message) {
    fprintf(stderr, "[ERROR][%s] - %s\n", context->name, message);
}

// Print according to the given format from the .h file
void log_info (plugin_context_t* context, const char* message) {
    fprintf(stdout, "[INFO][%s] - %s\n", context->name, message);
}

// .h file states specifically not to modify or free plugin's name
const char* plugin_get_name (void) {
    if (g_plugin_context.initialized && g_plugin_context.name) {
        return g_plugin_context.name;
    }

    // In case not initalized
    return "Unknown plugin";
}

const char* common_plugin_init (const char* (*process_function)(const char*), const char* name, int queueSize) {
    
    // Safety check (prevent double initializaiton)
    if (g_plugin_context.initialized) {
        return "Already initialized plugin";
    }
    
    // Another safety check
    if (name == NULL) {
        return "The name of the plugin cant be NULL";
    }
    
    // Another safety check
    if (queueSize <= 0) {
        return "The size of the queue size must be positive";
    }
    
    // And yet, another safety check
    if (process_function == NULL) {
        return "The procces function cant be NULL";
    }
    
    // Initialize context struct
    memset(&g_plugin_context, 0, sizeof(plugin_context_t));
    g_plugin_context.name = name;
    g_plugin_context.process_function = process_function;
    g_plugin_context.next_place_work = NULL;
    g_plugin_context.finished = 0;
    
    // Allocate memory and initialize the queue
    g_plugin_context.queue = malloc(sizeof(consumer_producer_t));
    if (g_plugin_context.queue == NULL) {
        return "Error, couldnt allocate memory for the queue";
    }
    

    const char* queueError = consumer_producer_init(g_plugin_context.queue, queueSize);
    if (queueError != NULL) {
        free(g_plugin_context.queue);
        g_plugin_context.queue = NULL;
        return queueError;
    }
    
    // Create a thread for the consumer
    // this thread will work and proccess items from the queue
    int threadResult = pthread_create(&g_plugin_context.consumer_thread, NULL, plugin_consumer_thread, &g_plugin_context);
    if (threadResult != 0) {
        consumer_producer_destroy(g_plugin_context.queue);
        free(g_plugin_context.queue);
        g_plugin_context.queue = NULL;
        return "Error, coludnt create consumer thread";
    }
    
    // Put a mark on the plugins, that they were intialized successfully
    g_plugin_context.initialized = 1;
    
    // Upon success
    return NULL;
}

const char* plugin_fini (void) {

    // Safety check
    if (!g_plugin_context.initialized) {
        return "Error, the plugin was not initialized";
    }
    
    // Wait for the consumer thread to finish proccessing to ensure
    // that there is no work left that we might lose during shutdown
    int joinResult = pthread_join(g_plugin_context.consumer_thread, NULL);
    if (joinResult != 0) {
        return "Error, failed to join the consumer thread";
    }
    
    // Clean up the queue
    if (g_plugin_context.queue != NULL) {
        consumer_producer_destroy(g_plugin_context.queue);
        free(g_plugin_context.queue);
        g_plugin_context.queue = NULL;
    }
    
    // Reset context (for clean state as required)
    memset(&g_plugin_context, 0, sizeof(plugin_context_t));
    
    // Upon success
    return NULL;
}

const char* plugin_place_work (const char* str) {
    
    // Safety check
    if (!g_plugin_context.initialized) {
        return "Error, the plugin was not initialized";
    }
    
    if (str == NULL) {
        return "Error, the input string cant be NULL";
    }
    
    // Create a copy of the string for the queue operations
    // consumer thread should free this once done proccessing
    char* strCopy = strdup(str);
    if (strCopy == NULL) {
        return "Error, failed memory allocation for string copy";
    }
    
    // Put the string into the queue (should block if queue is a t full capacity)
    const char* putError = consumer_producer_put(g_plugin_context.queue, strCopy);
    if (putError != NULL) {
        free(strCopy);
        return putError;
    }
    
    // Upon success
    return NULL;
}

void plugin_attach (const char* (*next_place_work)(const char*)) {
    if (g_plugin_context.initialized) {
        g_plugin_context.next_place_work = next_place_work;
    }
}

const char* plugin_wait_finished (void) {
    
    // Safety check
    if (!g_plugin_context.initialized) {
        return "Error, the plugin was not initialized";
    }
    
    // Wait for the plugin to finish
    int waitForResult = consumer_producer_wait_finished(g_plugin_context.queue);
    if (waitForResult != 0) {
        return "Failed to wait for plugin to finish";
    }
    
    // Upon success
    return NULL;
}