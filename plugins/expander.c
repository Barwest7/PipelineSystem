#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

// From the assignment, this plugin should:
// "Inserts a single white space between each character in the string."

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
    
    // Now we calculate the length of the new string, which is 
    // n (original) + n-1 (sapces) 
    size_t newLength = originalLength + (originalLength - 1);
    
    // Allocate memory for the new string (+1 is for null terminator)
    char* newString = malloc(newLength + 1);

    // Malloc will return null if we are out of memory
    // This way we check for failures of memory allocation
    if (newString == NULL) { return NULL; }
    
    // Insert spaces between each character
    // We use indexInString to follow the position in the string we 
    // return, meaning, after the teansformation
    size_t indexInString = 0;
    for (size_t i=0; i<originalLength; i++) {
        newString[indexInString++] = input[i];
        
        // Add space after each character except the last one
        if (i < originalLength- 1) {
            newString[indexInString++] = ' ';
        }
    }

    // Null terminate the string
    newString[newLength] = '\0';
    
    return newString;
}

// Required init function
const char* plugin_init (int queue_size) {
    return common_plugin_init(plugin_transform, "expander", queue_size);
}