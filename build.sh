#!/bin/bash

# On any failure, we exit with non zero exit code
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status()
{
    echo -e "${GREEN}[BUILD]${NC} $1"
}
print_warning()
{
    echo -e "${YELLOW}[WARNING]${NC} $1"
}
print_error()
{
    echo -e "${RED}[ERROR]${NC} $1"
}

# Create output directory
# it is created only if it doesnt exist
mkdir -p output
print_status "Output dir sucessfuly created"

# Compile main app
gcc -o output/analyzer main.c -ldl -lpthread || {
    print_error "Error, couldnt compile main app"
    exit 1
}
print_status "Main app compiled sucessfully"

# All the plugins that need to be built
pluginList="logger typewriter uppercaser rotator flipper expander"

# Build all the plugins
print_status "Starting to build plugins:"
print_status "${pluginList}"

for pluginName in $pluginList; do
    
    # In case that the plugin doesnt have a {pluginName}.c file:
    if [ ! -f "plugins/${pluginName}.c" ]; then
        print_warning "Couldnt find the pluign: ${pluginName}"
        print_warning "Skipping to the next plugin"
        continue
    fi
    
    # This part will be skipped in case the plugin wasnt found
    # If the plugin was found, it will be built
    gcc -fPIC -shared -o output/${pluginName}.so \
        plugins/${pluginName}.c \
        plugins/plugin_common.c \
        plugins/sync/monitor.c \
        plugins/sync/consumer_producer.c \
        -ldl -lpthread || {
        print_error "Error, couldnt build the plugin: $pluginName"
        exit 1
    }
    
    print_status "Sucessfully built plugin: $pluginName"
done

print_status "Sucessfuly built the app :)"