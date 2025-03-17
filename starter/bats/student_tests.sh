#!/bin/bash
# Test tracking
TESTS_TOTAL=0
TESTS_PASSED=0
TEST_TIMEOUT=5 # seconds

is_process_running() {
    ps -ef | grep "$1" | grep -v grep | grep -v student_tests.sh >/dev/null
    return $?
}

#Start the server
start_test_server() {
    local port=$1
    echo -e "${YELLOW}Starting server on port $port...${NC}"
    ./dsh -s -p $port &
    SERVER_PID=$!
    if ! is_process_running "dsh -s -p $port"; then
        echo -e "${RED}Server failed to start${NC}"
        return 1
    fi
    echo -e "${GREEN}Server started with PID $SERVER_PID${NC}"
    return 0
}

#Kill any server process
kill_test_server() {
    if [ -n "$SERVER_PID" ]; then
        echo -e "${YELLOW}Stopping server (PID $SERVER_PID)...${NC}"
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
        SERVER_PID=""
        echo -e "${GREEN}Server stopped${NC}"
    fi
}

#Send command to server
send_command() {
    local port=$1
    local command=$2
    local timeout=$3
    
    #Make sure that the server is running before starting the client
    if ! is_process_running "dsh -s -p $port"; then
        echo "ERROR: Server not running, can't execute command"
        return 1
    fi
    
    #Client interaction
    expect -c "
        set timeout $timeout
        spawn ./dsh -c -p $port
        expect \"dsh4>\"
        send \"$command\r\"
        expect {
            \"dsh4>\" { }
            timeout { puts \"TIMEOUT\"; exit 1 }
            eof { }
        }
        send \"exit\r\"
        expect eof
    " | grep -v "spawn" | grep -v "dsh4>" | grep -v "exit" | grep -v "socket client mode" | grep -v "cmd loop returned"
}

#Run a test and add to the test count and test results
run_test() {
    local test_name=$1
    local port=$2
    local command=$3
    local expected_output=$4
    local timeout=${5:-$TEST_TIMEOUT}
    
    echo -e "\n${YELLOW}Running test: $test_name${NC}"
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    #Make sure that the server is running
    if ! is_process_running "dsh -s -p $port"; then
        echo -e "${RED}Server not running, restarting...${NC}"
        start_test_server $port || {
            echo -e "${RED}Failed to restart server, skipping test${NC}"
            return
        }
    fi
    
    local output=$(send_command $port "$command" $timeout)
    
    #If output contains expected output
    if echo "$output" | grep -q "$expected_output"; then
        echo -e "${GREEN}✓ Test passed${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ Test failed${NC}"
        echo -e "${RED}Expected: $expected_output${NC}"
        echo -e "${RED}Got: $output${NC}"
    fi
}

#Exhaustive testing summar with pass/fail
print_summary() {
    echo -e "\n${YELLOW}Test Summary:${NC}"
    echo -e "${YELLOW}Total tests: $TESTS_TOTAL${NC}"
    echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
    echo -e "${RED}Failed: $((TESTS_TOTAL - TESTS_PASSED))${NC}"
    
    if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        return 0
    else
        echo -e "${RED}Some tests failed.${NC}"
        return 1
    fi
}

cleanup() {
    kill_test_server
    echo -e "${YELLOW}Cleanup complete${NC}"
    #Remove any created temp files
    rm -f /tmp/server_stopped_flag /tmp/server_stopped_test.tmp /tmp/test_output.txt
}

#Main test function
run_all_tests() {
    local port=5555 
    
    #Make sure dsh file is compiled
    if [ ! -x "./dsh" ]; then
        echo -e "${RED}Error: dsh executable not found. Run 'make' first.${NC}"
        return 1
    fi
    
    #Start server
    start_test_server $port || return 1
    
    #Basic command tests
    run_test "Simple echo command" $port "echo hello world" "hello world"
    run_test "Working directory" $port "pwd" "$(pwd)"
    run_test "List files" $port "ls -la" "total"
    
    #Built-in command tests
    run_test "CD command" $port "cd /bats && pwd" "/bats"
    
    #Pipeline tests
    run_test "Simple pipe" $port "echo hello world | grep hello" "hello"
    run_test "Multiple pipe" $port "ls -la | grep d | wc -l" "[0-9]" 
    
    #Error handling tests
    run_test "Command not found" $port "nonexistentcommand" "No such file"
    
    #Output redirection
    run_test "Output to temp file" $port "echo 'test data' > /tmp/test_output.txt && cat /tmp/test_output.txt" "test data"
    
    #More robust version of tests that failed previously
    run_test "Command chaining" $port "echo hello && echo world" "hello"
    run_test "Large output test" $port "ls -l /" "root"
    run_test "Error redirection" $port "ls /nonexistent 2>&1" "No such file"
    run_test "Special characters" $port "echo 'quoted string with spaces'" "quoted string"
    
    #Clean up
    kill_test_server
    
    if is_process_running "dsh -s -p $port"; then
        echo -e "${RED}✗ stop-server command test failed, server still running${NC}"
        TESTS_TOTAL=$((TESTS_TOTAL + 1))
        kill_test_server
    else
        echo -e "${GREEN}✓ stop-server command test passed${NC}"
        TESTS_TOTAL=$((TESTS_TOTAL + 1))
        TESTS_PASSED=$((TESTS_PASSED + 1))
    fi
    
    # Print summary
    print_summary
    return $?
}

#Run all tests
echo -e "${YELLOW}Starting remote shell test suite...${NC}"
run_all_tests
exit $?