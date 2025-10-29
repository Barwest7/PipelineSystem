#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// From the assignment, this plugin should:
// "Simulates a typewriter effect by printing each character with a 100ms
// delay (you can use the usleep function). Notice, this can cause a “traffic jam”."

// Implemntatoin of own transformation logic:
const char* plugin_transform (const char* input) {
    
    // Safety check for null input to avoid seg faults
    if (input == NULL) { return NULL; }
    
    // Typewriter should print this prefix
    printf("[typewriter] ");
    fflush(stdout);

    // Save the length of the input string for the for loop
    size_t originalLength = strlen(input);

    // Prints each char from the string with 100ms delay
    for (size_t i=0; i<originalLength; i++) {
        printf("%c", input[i]);
        fflush(stdout);
        
        // 100ms delay (100000ms)
        usleep(100000);
    }

    printf("\n");
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
const char* plugin_init(int queue_size) {
    return common_plugin_init (plugin_transform, "typewriter", queue_size);
}