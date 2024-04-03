#!/bin/bash

# Path to the input.txt file
INPUT_TXT_PATH="input.txt"

# Path to your LED matrix control binary
LED_MATRIX_BINARY="./command.sh"

# Path to your video file
VIDEO_FILE="files/reference.mp4"

# Path to the Python parser script
PARSER_SCRIPT="srt-parser.py"

# Clear the content of input.txt
> "$INPUT_TXT_PATH"

# Launch the LED matrix control binary
"$LED_MATRIX_BINARY" &

# Open the video file with MPV, mute sound, and send to the background
DISPLAY=:0 mpv --no-audio "$VIDEO_FILE" &

# Wait a bit to ensure MPV starts before running the parser script
sleep 2

# Start the parser script
python3 "$PARSER_SCRIPT"

# Optional: wait for the parser script to finish
wait

echo "All processes have completed."
