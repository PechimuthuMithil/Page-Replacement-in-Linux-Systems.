#!/bin/bash

# Define a range of values for n
start=1
end=11  # Change this to the desired end value
step=1

# Loop over the values of n
for ((n = start; n <= end; n += step)); do
    echo "Running with n = $n"
    ./mmap $n
done
