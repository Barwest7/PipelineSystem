#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// From the assignment, this plugin should:
// "Logs all strings that pass through to standard output."

// Implemntatoin of own transformation logic:
const char* plugin_transform (const char* input) {
    
    // Safety check for null input to avoid seg faults    
    if (input == NULL) { return NULL; }
    
    // Logger should print this prefix
    printf("[logger] %s\n", input);
    fflush(stdout);
    
    // Now we need to pass ad duplication of the original string to the next plugin
    // So we duplicate it using stduup
    // strdup allocates memory (behind the scenes) and duplicates the strin
    char* newString = strdup(input);
    
    // Malloc will return null if we are out of memory
    // This way we check for failures of memory allocation
    if (newString == NULL) {
        return NULL;
    }
    
    return newString;
}

// Required init function
const char* plugin_init (int queue_size) {
    return common_plugin_init(plugin_transform, "logger", queue_size);
}