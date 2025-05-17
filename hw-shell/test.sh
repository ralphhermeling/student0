#!/bin/bash

(make clean && make) >> /dev/null

# Exit if any unexpected command fails
set -e

run_test() {
  description="$1"
  command="$2"
  expected_file="$3"

  echo "Running test: $description"

  # Run the shell with the given command
  echo "$command" | ./shell > test_redirection_out

  # Compare output
  if ! diff -q "$expected_file" test_redirection_out > /dev/null; then
    echo "❌ Test FAILED: $description"
    echo "--- Expected ---"
    cat "$expected_file"
    echo "--- Got ---"
    cat test_redirection_out
    echo
    exit 1
  else
    echo "✅ Test PASSED: $description"
    echo
  fi

  rm -f test_redirection_out
}

# === Test 1: wc with stdin redirect ===
echo "one two three
four five" > test_redirection_in
echo " 2  5 24" > expected_1.txt
run_test "wc with stdin and stdout redirection" \
         "wc < test_redirection_in > test_redirection_out" \
         expected_1.txt

# === Test 2: echo to file ===
echo "hello world" > expected_2.txt
run_test "echo output to file" \
         "echo hello world > test_redirection_out" \
         expected_2.txt

# === Test 3: cat with stdin redirect ===
echo "just a line" > test_redirection_in
echo "just a line" > expected_3.txt
run_test "cat with stdin redirection" \
         "cat < test_redirection_in > test_redirection_out" \
         expected_3.txt

# === Test 4: wc on empty file ===
touch test_redirection_in
wc < test_redirection_in > expected_4.txt
run_test "wc with empty input file" \
         "wc < test_redirection_in > test_redirection_out" \
         expected_4.txt


echo "🎉 All redirection tests passed!"

# === Test 5: single pipe ===
echo "hello world" | wc > expected_5.txt
run_test "echo piped to wc" \
         "echo hello world | wc > test_redirection_out" \
         expected_5.txt

# === Test 6: double pipe ===
echo "hello world" | tr a-z A-Z | wc > expected_6.txt
run_test "echo to tr to wc using two pipes" \
         "echo hello world | tr a-z A-Z | wc > test_redirection_out" \
         expected_6.txt

# === Test 7: three pipes with input redirection ===
echo -e "hello there\nhi\nhello again" > test_redirection_in
cat < test_redirection_in | tr a-z A-Z | grep HELLO | wc > expected_7.txt
run_test "cat with input redirection and three pipes" \
         "cat < test_redirection_in | tr a-z A-Z | grep HELLO | wc > test_redirection_out" \
         expected_7.txt

# === Cleanup ===
rm -f test_redirection_in expected_*.txt
