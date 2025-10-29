#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

// From the assignment, this plugin should:
// "Reverses the order of characters in the string."

// Implemntatoin of own transformation logic:
const char* plugin_transform (const char* input) {

    // Safety check for null input to avoid seg faults
    if (input == NULL) { return NULL; }
    
    // Save the length of the input string for memory operations
    size_t originalLength = strlen(input);

    // Empty string doesnt need transformation, return it as it is
    if (originalLength == 0) {
        return strdup(input);
    }
    
    // Allocate memory for the new string (reverse string is of length n and + 1 for null terminator)
    char* newString = malloc(originalLength +1);

    // Malloc will return null if we are out of memory
    // This way we check for failures of memory allocation
    if (newString == NULL) { return NULL; }
    
    // Here we reverse the string
    for (size_t i=0; i<originalLength; i++) {
        newString[i] = input[originalLength - 1 -i];
    }

    // Add a null terminator
    newString[originalLength] = '\0';
    
    return newString;
}

// Required init function
const char* plugin_init (int queue_size) {
    return common_plugin_init(plugin_transform, "flipper", queue_size);
}