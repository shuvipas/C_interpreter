#!/bin/bash

# Configuration
TESTS_DIR="tests"
INTERPRETER_SOURCE="./c_interpreter.c"
INTERPRETER_BINARY="cint"

# Function to display usage information
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Compare the output of C programs between native execution and the C interpreter."
    echo "Tests all .c files in the '$TESTS_DIR' directory."
    echo ""
    echo "Options:"
    echo "  -h, --help    Show this help message and exit"
    echo ""
    echo "The script will:"
    echo "1. Compile the interpreter"
    echo "2. For each test program in '$TESTS_DIR':"
    echo "   - Compile the native version"
    echo "   - Run both versions and capture outputs"
    echo "   - Compare the outputs"
    echo "3. Report results"
    echo "4. Clean up temporary files"
}

# Function to clean up temporary files
cleanup() {
    # Remove temporary files if they exist
    rm -f "$INTERPRETER_BINARY" \
       native_stdout.tmp native_stderr.tmp \
       interpreted_stdout.tmp interpreted_stderr.tmp
    # Clean up compiled test programs
    find "$TESTS_DIR" -name "*.c" -exec sh -c 'rm -f "$(dirname "$1")/$(basename "$1" .c)"' sh {} \;
}

# Register cleanup function to run on exit or interrupt
trap cleanup EXIT INT

# Check for help option
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    usage
    exit 0
fi

# Check that the interpreter source exists
if [[ ! -f "$INTERPRETER_SOURCE" ]]; then
    echo "Error: $INTERPRETER_SOURCE not found in current directory" >&2
    exit 1
fi

# Check that tests directory exists
if [[ ! -d "$TESTS_DIR" ]]; then
    echo "Error: Tests directory '$TESTS_DIR' not found" >&2
    exit 1
fi

# Compile the interpreter
echo "Compiling C interpreter..."
if ! gcc -o "$INTERPRETER_BINARY" "$INTERPRETER_SOURCE"; then
    echo "Error: Failed to compile C interpreter" >&2
    exit 1
fi

# Initialize counters
total_tests=0
passed_tests=0

# Find all .c files in tests directory and subdirectories
while IFS= read -r -d '' test_file; do
    ((total_tests++))
    test_name=$(basename "$test_file")
    relative_path=${test_file#$TESTS_DIR/}
    
    echo -e "\nTesting $relative_path..."
    
    # Compile the test program
    if ! gcc -o "${test_file%.c}" "$test_file"; then
        echo "  ERROR: Failed to compile $test_name"
        continue
    fi
    
    # Run the native program and capture outputs
    "./${test_file%.c}" > native_stdout.tmp 2> native_stderr.tmp
    
    # Run the interpreted program and capture outputs
    "./$INTERPRETER_BINARY" "$test_file" > interpreted_stdout.tmp 2> interpreted_stderr.tmp
    
    
    # Remove the "exit(0)" line from interpreter output if it's the last line
    # sed -i '/^exit([0-9]\+)$/d' interpreted_stdout.tmp
    # Remove all last 8 chars 
    head -c -8 interpreted_stdout.tmp > tmp && mv tmp interpreted_stdout.tmp



    # Compare stdout outputs
    stdout_diff=$(diff -u native_stdout.tmp interpreted_stdout.tmp)
    # Compare stderr outputs
    stderr_diff=$(diff -u native_stderr.tmp interpreted_stderr.tmp)
    
    # Check if there are any differences
    if [[ -z "$stdout_diff" && -z "$stderr_diff" ]]; then
        echo "  PASS: Outputs match"
        ((passed_tests++))
    else
        echo "  FAIL: Differences detected in $relative_path"
        # Show stdout differences if any
        if [[ -n "$stdout_diff" ]]; then
            echo "  Standard output differences:"
            echo "$stdout_diff" | sed 's/^/    /'
        fi
        # Show stderr differences if any
        if [[ -n "$stderr_diff" ]]; then
            echo "  Standard error differences:"
            echo "$stderr_diff" | sed 's/^/    /'
        fi
    fi
    
    # Clean up the compiled test program
    rm -f "${test_file%.c}"
done < <(find "$TESTS_DIR" -type f -name "*.c" -print0)

# Print summary
echo -e "\nTest results:"
echo "  $passed_tests/$total_tests tests passed"

# Exit with appropriate status
if [[ "$passed_tests" -eq "$total_tests" ]]; then
    exit 0
else
    exit 1
fi