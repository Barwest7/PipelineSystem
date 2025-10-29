#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>

// From the assignment, this plugin should:
// "Moves every character in the string one position to the right. The last
// character wraps around to the front."

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
    
    // Allocate memory for the new string (+1 for the null terminator)
    char* newString = malloc(originalLength + 1);

    // Malloc will return null if we are out of memory
    // This way we check for failures of memory allocation
    if (newString == NULL) { return NULL; }
    
    // Perform the required transfromationt:
    // Last character from the original string comes first in the modified string
    newString[0] = input[originalLength - 1];

    // Move all the characters one step right
    for (size_t i=1; i<originalLength; i++) {
        newString[i] = input[i - 1];
    }

    // Null terminate the string
    newString[originalLength] = '\0';
    
    return newString;
}

// Required init function
const char* plugin_init (int queue_size) {
    return common_plugin_init(plugin_transform, "rotator", queue_size);
}