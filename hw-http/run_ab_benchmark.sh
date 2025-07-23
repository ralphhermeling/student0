#!/bin/bash

# USAGE: ./run_ab_benchmark.sh <server_name>

SERVER_NAME="$1"
if [ -z "$SERVER_NAME" ]; then
  echo "Usage: $0 <server_name>"
  exit 1
fi

URL="http://0.0.0.0:8000/"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_FILE="ab_results_${SERVER_NAME}_${TIMESTAMP}.csv"

# Define test parameters
N_VALUES=(100 500 1000 2000)
C_VALUES=(1 10 100 500)

# Header for CSV file
echo "server,n,c,min,mean,stddev,median,max" > "$OUTPUT_FILE"

# Run tests
for N in "${N_VALUES[@]}"; do
  for C in "${C_VALUES[@]}"; do
    if (( C > N )); then
      continue  # skip invalid test case
    fi

    echo "Running test with -n $N -c $C..."

    # Run Apache Benchmark and capture output
    RESULT=$(ab -n "$N" -c "$C" "$URL" 2>&1)

    # Extract relevant metrics from "Connection Times (ms)" Total line
    METRICS=$(echo "$RESULT" | awk '
      /Total:/ {
        min=$2; mean=$3; stddev=$4; median=$5; max=$6;
        gsub("\\[|\\]", "", stddev);
        print min "," mean "," stddev "," median "," max;
        exit
      }')

    # Append to CSV
    echo "$SERVER_NAME,$N,$C,$METRICS" >> "$OUTPUT_FILE"
  done
done

echo "All benchmarks complete."
echo "Results saved in: $OUTPUT_FILE"

