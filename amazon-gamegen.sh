#!/bin/bash
while true; do
    python3 generate_data.py
    echo "Restarting generation in 5 seconds..."
    sleep 5
done