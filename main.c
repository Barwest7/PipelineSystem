#define _GNU_SOURCE
#include "plugins/plugin_sdk.h"
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

// Assumption from assignment:
// No input line exceeds 1024 characters
// Thus: 1024 chars + 1 newline + 1 null terminator = 1026
// this is for step 5: reading input
#define MaximalLineLength 1026

// Type definitions for the struct introduced in the assignment
typedef const char* (*plugin_init_func_t)(int);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*plugin_place_work_func_t)(const char*);
typedef void (*plugin_attach_func_t)(const char* (*)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);
typedef const char* (*plugin_get_name_func_t)(void);

// Plugin data sruct from assignment
typedef struct {
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    plugin_place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    char* name;
    void* handle;
} plugin_handle_t;

// Globals:
static plugin_handle_t* plugins = NULL;
static int numPlugins = 0;
static int sizeQueue = 0;

// Print usage
void PrintUsageMessage () {
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  queue_size    Maximum number of items in each plugin's queue\n");
    printf("  plugin1..N    Names of plugins to load (without .so extension)\n");
    printf("\n");
    printf("Available plugins:\n");
    printf("  logger        - Logs all strings that pass through\n");
    printf("  typewriter    - Simulates typewriter effect with delays\n");
    printf("  uppercaser    - Converts strings to uppercase\n");
    printf("  rotator       - Move every character to the right. Last character moves to \n");
    printf("the beginning.\n");
    printf("  flipper       - Reverses the order of characters\n");
    printf("  expander      - Expands each character with spaces\n");
    printf("\n");
    printf("Example:\n");
    printf("  ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo 'hello' | ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo '<END>' | ./analyzer 20 uppercaser rotator logger\n");
}

// Step 1:
void ParseCommandLineArgs (int argc, char* argv[]) {
    
    // Check number of arguments in the input
    if (argc < 3) {

        // Print error
        fprintf(stderr, "Error, less than 3 args ");

        // Print usage
        PrintUsageMessage();

        // Exit code 1
        exit(1);
    }

    // Check queue size is positive integer
    char *endpointer;
    long tempSizeQueue = strtol(argv[1], &endpointer, 10);

    if (*endpointer != '\0' || tempSizeQueue <= 0) {
        fprintf(stderr, "Error, queue size cant be 0 or lower ");

        // Print usage
        PrintUsageMessage();

        // Exit code 1
        exit(1);
    }

    sizeQueue = (int)tempSizeQueue;
    
    // Now we save the plugins names from the std input:
    // If the amount of pluginss exceeds the allowd amount, log an error
    numPlugins = argc - 2;
    plugins = calloc(numPlugins, sizeof(plugin_handle_t));
    if (!plugins) {
        fprintf(stderr, "Error: plugins, memory allocation failed for array");
        
        // Print usage
        PrintUsageMessage();

        // Exit code 1
        exit(1);
    }

    // Save the names into the array that we created in globels section
    for (int i=0; i<numPlugins; i++) {
        plugins[i].name = strdup(argv[i+2]);
        if (!plugins[i].name) {
            fprintf(stderr, "Error: plugins, memory allocaton failed for name");
                    
            // Print usage
            PrintUsageMessage();

            // Exit code 1
            exit(1);
        }
    }
}

// Step 2 (preprocess for step 2):
void LoadSinglePluginSO (int index) {
    
    // Create buffer
    char fileName[256];

    // Format filename string
    snprintf(fileName, sizeof(fileName), "./output/%s.so", plugins[index].name);

    // Now we load the shared object:
    // I used dlopen but that did not support multiple plugins of the same type, because
    // the dynamic loader reused the same memory and all static/global variables are shared 
    // for each .so file
    // To solve that I changed to dlmopen with the flag LM_ID_NEWLM, which should load each instance
    // into its own namespace, that should give independent static (global) variables.
    // Using this flag is fine since it is not a compilation flag but a runtime flag,
    // (in the Piazza we were instructed not to add compilation flags).
    plugins[index].handle = dlmopen(LM_ID_NEWLM, fileName, RTLD_NOW | RTLD_LOCAL);

    // In case loading the shared object failed
    if (!plugins[index].handle) {
        fprintf(stderr, "Error, couldnt load shared object ");
                    
        // Print usage
        PrintUsageMessage();

        // Exit code 1
        exit(1);
    }

    // Clear any prior errors
    dlerror();

    // Load all the required functions from the shared object
    plugins[index].init = (plugin_init_func_t)dlsym(plugins[index].handle, "plugin_init");
    plugins[index].fini = (plugin_fini_func_t)dlsym(plugins[index].handle, "plugin_fini");
    plugins[index].place_work = (plugin_place_work_func_t)dlsym(plugins[index].handle, "plugin_place_work");
    plugins[index].attach = (plugin_attach_func_t)dlsym(plugins[index].handle, "plugin_attach");
    plugins[index].wait_finished = (plugin_wait_finished_func_t)dlsym(plugins[index].handle, "plugin_wait_finished");
    
    // Check for dlsym errors
    char* error = dlerror();

    // In case the error is not empty it means that there are errors
    if (error != NULL) {

        fprintf(stderr, "Error, couldnt load plugin %s: %s\n", plugins[index].name, error);
        dlclose(plugins[index].handle);
        
        // Print usage
        PrintUsageMessage();

        // Exit code 1
        exit(1);
    }

    
    if (!plugins[index].init || !plugins[index].fini ||
    !plugins[index].place_work || !plugins[index].attach || !plugins[index].wait_finished) {
        
        fprintf(stderr, "Error, %s plugin is missing functions\n", plugins[index].name);
        
        dlclose(plugins[index].handle);

        // Print usage
        PrintUsageMessage();

        // Exit code 1
        exit(1);
    }
}

// Step 2 (the step itself)
void LoadPlugins () {
        
    // We itearate over all the plugins and do the loading for each of them    
    for (int i=0; i<numPlugins; i++) {
        LoadSinglePluginSO(i);
    }
}

// Step 3
void InitializePlugins () {

    // Iterate over all the plugins and initialize them
    for (int i=0; i<numPlugins; i++) {
        const char* error = plugins[i].init(sizeQueue);
        if (error != NULL) {
            
            fprintf(stderr, "Error: cant initialize plugin %s: %s\n", plugins[i].name, error);

            // Exit code 2
            exit(2);
        }
    }
}

// Step 4
void AttachPluginsTogether () {

    // Attach all plugins except the last one
    for (int i=0; i<numPlugins-1; i++) {
        plugins[i].attach(plugins[i+1].place_work);
    }
    
    // Dont do anything for the last plugin
}

// Step 5 
void ReadInputFromSTDIn () {

    // Create a buffer to store each line
    char line[MaximalLineLength];
    
    // Keep reading lines until end of file (End signal check comes later)
    while (fgets(line, sizeof(line), stdin)) {
        
        // Make sure theres no trailing \n by removing it (replace with null terminator)
        size_t currLineLength = strlen(line);
        if (currLineLength > 0 && line[currLineLength - 1] == '\n') {
            line[currLineLength - 1] = '\0';
        }
        
        // Now start off by sending it to the first plugin in the order
        // In case there is any error, break out of the loop
        const char* error = plugins[0].place_work(line);
        if (error != NULL) {
            fprintf(stderr, "Error: couldnt send the line to the first plugin. error: %s\n", error);
            break;
        }
        
        // Compare the line to <END> to see if this is the end signal
        if (strcmp(line, "<END>") == 0) {
            break;
        }
    }
}

// Step 6
void WaitForPluginsToFinish () {

    // Go over all the plugins and call their wait_finished
    for (int i=0; i<numPlugins; i++) {
        const char* error = plugins[i].wait_finished();
        if (error != NULL) {
            fprintf(stderr, "Error: the plugin - %s couldnt finish. error: %s\n", plugins[i].name, error);
        }
    }
}

// Step7 
void Cleanup () {
    
    // In case there are no plugins, simply return 
    // in order to avoid derferencing a null pointer
    if (!plugins) {
        return;
    }
    
    // Go over all the plugins
    for (int i=0; i<numPlugins; i++) {
        
        // Call each plugin its fini function
        // Do it within an if statment so that in case the function was not loaded
        // we will avoid a sgementation fault
        if (plugins[i].fini) {
            plugins[i].fini();
        }

        // Do the same for handle
        // to unload from the memory any sharded object files that were opened
        if (plugins[i].handle) {
            dlclose(plugins[i].handle);
        }

        // Free the plugin name duplicated earlier
        if (plugins[i].name) {
            free(plugins[i].name);
        }
    }
    
    // Free the entire plugin array
    free(plugins);
    plugins = NULL;
    numPlugins = 0;
}

// Step 8
int Finalize () {
    printf("Pipeline shutdown complete\n");
    return 0;
}

int main (int argc, char* argv[]) {
    
    // Step 1
    ParseCommandLineArgs(argc, argv);

    // Step 2
    LoadPlugins();

    // Step 3
    InitializePlugins();

    // Step 4
    AttachPluginsTogether();

    // Step 5
    ReadInputFromSTDIn();
    
    // Step 6
    WaitForPluginsToFinish();
    
    // Step 7
    Cleanup();
    
    // Step 8
    return Finalize();
}