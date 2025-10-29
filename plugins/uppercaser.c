#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// From the assignment, this plugin should:
// "Converts all alphabetic characters in the string to uppercase."

// Implemntatoin of own transformation logic:
const char* plugin_transform (const char* input) {

    // Safety check for null input to avoid seg faults
    if (input == NULL) { return NULL; }

    // Duplicate the new string
    // strdup allocates memory (behind the scenes) and duplicates the string
    char* newString = strdup(input);

    // Malloc will return null if we are out of memory
    // This way we check for failures of memory allocation
    if (newString == NULL) { return NULL; }
    
    // Save the length of the input string for the for loop
    size_t originalLength = strlen(input);

    // Convert the duplicated string to uppercase
    for (size_t i=0; i<originalLength; i++) {
        newString[i] = toupper(newString[i]);
    }
    
    return newString;
}

// Required init function
const char* plugin_init(int queue_size) {
    return common_plugin_init (plugin_transform, "uppercaser", queue_size);
}