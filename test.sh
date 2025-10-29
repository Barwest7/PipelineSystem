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

usageMessage="Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>

Arguments:
  queue_size    Maximum number of items in each plugin's queue
  plugin1..N    Names of plugins to load (without .so extension)

Available plugins:
  logger        - Logs all strings that pass through
  typewriter    - Simulates typewriter effect with delays
  uppercaser    - Converts strings to uppercase
  rotator       - Move every character to the right. Last character moves to 
the beginning.
  flipper       - Reverses the order of characters
  expander      - Expands each character with spaces

Example:
  ./analyzer 20 uppercaser rotator logger
  echo 'hello' | ./analyzer 20 uppercaser rotator logger
  echo '<END>' | ./analyzer 20 uppercaser rotator logger"

# Define the different error types - FIXED to match actual program output order
queueErrorMessage="Error, queue size cant be 0 or lower $usageMessage"

argsErrorMessage="Error, less than 3 args $usageMessage"


pluginErrorMessage="Error, couldnt load shared object $usageMessage"

print_status "Starting tests for piepline system:"

# Start off by building the entire project by running build.sh
print_status "Running build.sh script"
if ! ./build.sh; then
    print_error "Build failed"
    exit 1
fi

print_status "Build complete"

# Test counter
NumOfTotalTests=0
NumOfPassedTests=0

# Function that runs each test case
runTest() {
    local testName="$1"
    local input="$2"
    local command="$3"
    local expected="$4"
    
    # This gets true or false
    local shouldSucess="$5"

    # Default 15 seconds but longer for typewriter tests
    local timeOutTime="${6:-15}"
    
    # Increase by 1 the number of total tests run
    NumOfTotalTests=$((NumOfTotalTests + 1))
    
    print_status "Running test $NumOfTotalTests: $testName"
    
    # Create temporary files for stdout and stderr in order
    # to allow to examine the streams have the right output
    local stdout_file=$(mktemp)
    local stderr_file=$(mktemp)
    
    # Run the reuqired command and catch the output
    local exit_code=0
    if [ "$input" != "" ]; then
        echo -e "$input" | timeout $timeOutTime $command 1>"$stdout_file" 2>"$stderr_file" || exit_code=$?
    else
        timeout $timeOutTime $command 1>"$stdout_file" 2>"$stderr_file" || exit_code=$?
    fi
    
    # Read the outputs
    local stdOut=$(cat "$stdout_file")
    local stdErr=$(cat "$stderr_file")
    local combinedOutput="$stdErr$stdOut"
    
    # Cleanup the temporarty files of stdout and stderr
    rm -f "$stdout_file" "$stderr_file"

    # Test should succeed (positive test) that means
    # exit code 0, and not 124 from timeout
    if [ "$shouldSucess" = "true" ]; then
        if [ $exit_code -eq 0 ] && [[ "$combinedOutput" == $expected ]]; then
            print_status "$testName: Passed"

            # FIXED: Use correct variable name
            NumOfPassedTests=$((NumOfPassedTests + 1))
        else
            print_error "$testName: Failed"
            print_warning "Expected: $expected"
            print_warning "Actual: $combinedOutput"
            print_warning "Exited with code: $exit_code"
            if [ $exit_code -eq 124 ]; then
                print_error "Failed and timed out after $timeOutTime seconds"
            fi
            exit 1
        fi
    else
        
        # Test should fail (negative test) that means exit code was not 0
        # from sucess but neither 124 from timeout
        if [ $exit_code -ne 0 ] && [ $exit_code -ne 124 ]; then
            if [[ "$combinedOutput" == $expected ]]; then
                print_status "$testName: Test passed and as expected, meaning it failed with exit code $exit_code"
                # FIXED: Use correct variable name
                NumOfPassedTests=$((NumOfPassedTests + 1))
            else
                print_error "$testName: Failed. wrong error message"
                print_warning "Expected: $expected"
                print_warning "Actual: $combinedOutput"
                exit 1
            fi
        else
            print_error "$testName: Failed, it should have failed but it sucessed or timed out)"
            print_warning "Actual output: $combinedOutput"
            print_warning "Exited with code: $exit_code"
            exit 1
        fi
    fi
}

print_status "Positive tests:"

# Test 1: Just logger
runTest "Just logger" \
    "hello\n<END>" \
    "./output/analyzer 10 logger" \
    "\[logger\] hello
Pipeline shutdown complete" \
    "true"

# Test 2: Upper + logger
runTest "Upper + logger" \
    "hello\n<END>" \
    "./output/analyzer 10 uppercaser logger" \
    "\[logger\] HELLO
Pipeline shutdown complete" \
    "true"

# Test 3: Rotator + logger
runTest "Rotator + logger" \
    "hello\n<END>" \
    "./output/analyzer 5 rotator logger" \
    "\[logger\] ohell
Pipeline shutdown complete" \
    "true"

# Test 4: Flipper + logger  
runTest "Flipper + logger" \
    "hello\n<END>" \
    "./output/analyzer 5 flipper logger" \
    "\[logger\] olleh
Pipeline shutdown complete" \
    "true"

# Test 5: Expander + logger
runTest "Expander + logger" \
    "hello\n<END>" \
    "./output/analyzer 5 expander logger" \
    "\[logger\] h e l l o
Pipeline shutdown complete" \
    "true"

# Test 6: Typewriter
runTest "Typewriter plugin test" \
    "hello\n<END>" \
    "./output/analyzer 5 typewriter" \
    "\[typewriter\] hello
Pipeline shutdown complete" \
    "true" \
    "30"

# Test 7: Upper + typewriter
runTest "Upper + typewriter" \
    "hello\n<END>" \
    "./output/analyzer 5 uppercaser typewriter" \
    "\[typewriter\] HELLO
Pipeline shutdown complete" \
    "true" \
    "30"

# Test 8: Rotator + typewriter
runTest "Rotator + typewriter" \
    "hello\n<END>" \
    "./output/analyzer 5 rotator typewriter" \
    "\[typewriter\] ohell
Pipeline shutdown complete" \
    "true" \
    "30"

# Test 9: Flipper + typewriter
runTest "Flipper + typewriter" \
    "hello\n<END>" \
    "./output/analyzer 5 flipper typewriter" \
    "\[typewriter\] olleh
Pipeline shutdown complete" \
    "true" \
    "30"

# Test 10: Expander + typewriter
runTest "Expander + typewriter" \
    "hello\n<END>" \
    "./output/analyzer 5 expander typewriter" \
    "\[typewriter\] h e l l o
Pipeline shutdown complete" \
    "true" \
    "30"

# Test 11: Complex pipeline 1
runTest "Complex pipeline 1" \
    "hello\n<END>" \
    "./output/analyzer 15 uppercaser rotator flipper logger" \
    "\[logger\] LLEHO
Pipeline shutdown complete" \
    "true"

# Test 12: Complex pipeline 2
runTest "Complex pipeline 2" \
    "hello\n<END>" \
    "./output/analyzer 10 expander uppercaser rotator logger" \
    "\[logger\] OH E L L 
Pipeline shutdown complete" \
    "true"

# Test 13: <END> as input
runTest "<END> as input" \
    "<END>" \
    "./output/analyzer 5 logger" \
    "Pipeline shutdown complete" \
    "true"

# Test 14: Special chars in input
runTest "Special chars in input" \
    "!@#$%\n<END>" \
    "./output/analyzer 5 logger" \
    "\[logger\] !@#$%
Pipeline shutdown complete" \
    "true"

# Test 15: 0 queue size
runTest "0 queue size" \
    "hello\n<END>" \
    "./output/analyzer 0 logger" \
    "$queueErrorMessage" \
    "false"

# Test 16: Minus queue size  
runTest "Minus queue size" \
    "" \
    "./output/analyzer -5 logger" \
    "$queueErrorMessage" \
    "false"

# Test 17: Not a number queue size
runTest "Not a number queue size" \
    "" \
    "./output/analyzer abc logger" \
    "$queueErrorMessage" \
    "false"

# Test 18: Missing queue size
runTest "Missing queue size" \
    "" \
    "./output/analyzer logger" \
    "$argsErrorMessage" \
    "false"

# Test 19: Missing plugin arg
runTest "Missing plugin arg" \
    "" \
    "./output/analyzer 10" \
    "$argsErrorMessage" \
    "false"

# Test 20: Missing other arg
runTest "Missing other arg" \
    "" \
    "./output/analyzer" \
    "$argsErrorMessage" \
    "false"

# Test 21: No args
runTest "No args" \
    "" \
    "./output/analyzer" \
    "$argsErrorMessage" \
    "false"

# Test 22: Non existent plugin
runTest "Non existent plugin" \
    "" \
    "./output/analyzer 10 fakeplugin" \
    "$pluginErrorMessage" \
    "false"

# Test 23: Plugin reuse in chain
runTest "Plugin reuse" \
    "hello\n<END>" \
    "./output/analyzer 10 rotator rotator logger" \
    "\[logger\] lohel
Pipeline shutdown complete" \
    "true"

# Function for stress testing with multiple iterations
runStressTest() {
    local testName="$1"
    local iterations="$2"
    local input="$3"
    local command="$4"
    local expected="$5"
    local timeoutTime="${6:-10}"
    
    NumOfTotalTests=$((NumOfTotalTests + 1))
    print_status "Running stress test $NumOfTotalTests: $testName ($iterations iterations)"
    
    for i in $(seq 1 $iterations); do
        local stdout_file=$(mktemp)
        local stderr_file=$(mktemp)
        local exit_code=0
        
        if [ "$input" != "" ]; then
            echo -e "$input" | timeout $timeoutTime $command 1>"$stdout_file" 2>"$stderr_file" || exit_code=$?
        else
            timeout $timeoutTime $command 1>"$stdout_file" 2>"$stderr_file" || exit_code=$?
        fi
        
        local stdOut=$(cat "$stdout_file")
        local stdErr=$(cat "$stderr_file")
        local combinedOutput="$stdErr$stdOut"
        
        rm -f "$stdout_file" "$stderr_file"
        
        if [ $exit_code -ne 0 ] || [[ "$combinedOutput" != $expected ]]; then
            print_error "$testName: Failed on iteration $i"
            print_warning "Expected: $expected"
            print_warning "Actual: $combinedOutput"
            print_warning "Exit code: $exit_code"
            exit 1
        fi
    done
    
    print_status "$testName: Passed all $iterations iterations"
    NumOfPassedTests=$((NumOfPassedTests + 1))
}

# Test 24: Rapid succession runs to expose race conditions
runStressTest "Rapid succession (100 runs)" \
    100 \
    "test\n<END>" \
    "./output/analyzer 3 uppercaser logger" \
    "\[logger\] TEST
Pipeline shutdown complete" \
    "5"

# Test 25: Queue size 1 stress test (forces maximum synchronization)
runStressTest "Queue size 1 stress (forces tight synchronization)" \
    50 \
    "line1\nline2\nline3\n<END>" \
    "./output/analyzer 1 uppercaser flipper logger" \
    "\[logger\] 1ENIL
\[logger\] 2ENIL
\[logger\] 3ENIL
Pipeline shutdown complete" \
    "10"

print_status "Test Breakdown:"
print_status "Total tests run: $NumOfTotalTests"
print_status "Tests passed: $NumOfPassedTests"

if [ $NumOfPassedTests -eq $NumOfTotalTests ]; then
    print_status "ALL TESTS PASSED!"
    print_status "Your pipeline system is working correctly!"
    exit 0
else
    FAILED_TESTS=$((NumOfTotalTests - NumOfPassedTests))
    print_error "$FAILED_TESTS tests failed!"
    exit 1
fi