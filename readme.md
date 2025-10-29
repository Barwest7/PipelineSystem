** Modular Pipeline System

A multithreaded string processing pipeline in C, Built for an OS course final project and scored 100/100 on it.

What is it?
This is a plugin based system where you can chain together text transformations.
Each plugin runs in its own thread and they communicate through thread safe queues. 

Available Plugins:
uppercaser: converts text to uppercase
rotator: shifts each character one position right
flipper: reverses the string
expander: adds spaces between characters
logger: prints the string to stdout
typewriter: prints character by character with delay (typewriter effect)

Building:
./build.sh

Usage:
./output/analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>

Example:
echo -e "hello\n<END>" | ./output/analyzer 20 uppercaser rotator logger

Output:
[logger] OHELL
Pipeline shutdown complete

You can chain plugins in any order and even reuse the same plugin multiple times.

Testing:
Unit test for monitor and queue are included for your convience, but are not run as part of the test.sh file.
./test.sh
This will runs 25 tests including stress tests for race conditions.

How does it all work?
Each plugin:
Gets its own thread and input queue -->
Takes work from its queue -->
Processes it -->
Passes result to the next plugin.

The main program loads plugins dynamically at runtime using dlopen. 
I used dlmopen with LM_ID_NEWLM so you can have multiple instances of the same plugin with separate state.

Implementation highlights:
The tricky parts were:

Building a stateful monitor to avoid race conditions (standard pthread cond vars can miss signals)
Making the producer-consumer queues block properly without busy-waiting
Getting graceful shutdown working when <END> signal comes through
Managing memory ownership across plugin boundaries

Adding your own plugin:
Create plugins/myplugin.c:

#include "plugin_common.h"

const char* plugin_transform(const char* input) {
    // do your transformation
    return strdup(input);  // return allocated string
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "myplugin", queue_size);
}

Then add it to the plugin list in build.sh and rebuild.

Notes:
Developed for Operating Systems course at Reichman University. The assignment required implementing all synchronization primitives from scratch.

Tested on Ubuntu 24.04 with GCC 13.