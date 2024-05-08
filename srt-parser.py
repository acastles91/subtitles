#!/usr/bin/python

from datetime import datetime, timedelta, time as dt_time
import time
import pygame
import re
import sys

# Function to parse SRT file and yield subtitles with timecodes
# Function to write text to input.txt

#def parse_time(time_str):
#    """Convert an SRT time string into a datetime object."""
#    hours, minutes, seconds, milliseconds = map(int, re.split('[:,]', time_str))
#    return datetime.combine(datetime.today(), dt_time(hour=hours, minute=minutes, second=seconds, microsecond=milliseconds*1000))
#


def center_text(text, max_chars):
    """Center text within a given width by adding whitespace."""
    line_length = len(text)
    if line_length < max_chars:
        padding_left = (max_chars - line_length) // 2
        padding_right = max_chars - line_length - padding_left
        return ' ' * padding_left + text + ' ' * padding_right
    return text

#def center_text(text, max_chars):
#    """Center text within a given width by adding whitespace."""
#    line_length = len(text)
#    if line_length < max_chars:
#        padding = (max_chars - line_length) // 2
#        return ' ' * padding + text + ' ' * padding
#    return text

def preprocess_srt_content(content):
    """Collapse multiple consecutive empty lines into a single empty line."""
    # Split the content into lines, remove empty lines, and rejoin with a single empty line
    # This effectively collapses multiple consecutive empty lines into one
    processed_content = '\n\n'.join([block for block in content.split('\n\n') if block.strip()])
    return processed_content



def parse_time(time_str):
    """Convert an SRT time string into a timedelta object."""
    hours, minutes, seconds, milliseconds = map(int, re.split('[:,]', time_str))
    return timedelta(hours=hours, minutes=minutes, seconds=seconds, milliseconds=milliseconds)

def split_text_by_length(text, first_line_length):
    """Split text into two lines based on a predefined length for the first line."""
    if len(text) <= first_line_length:
        # If the text is shorter than or equal to the limit, the first line is the text itself,
        # and there's no need for a second line.
        return text, ''
    else:
        # If the text exceeds the limit, find the last space before the limit
        # to avoid breaking words. If there's no space, split at the limit.
        split_point = text.rfind(' ', 0, first_line_length)
        if split_point == -1:  # No space found, split at the length limit
            split_point = first_line_length
        first_line = text[:split_point]
        second_line = text[split_point:].strip()
        return first_line, second_line


def remove_html_tags(text):
    """Remove HTML tags from a string."""
    clean_text = re.sub('<.*?>', '', text)
    return clean_text

def play_audio(audio_filename):
    """Play audio file."""
    pygame.mixer.init()  # Initialize the mixer module
    pygame.mixer.music.load(audio_filename)  # Load the audio file
    pygame.mixer.music.play()  # Play the audio file

def parse_srt(filename, max_chars):
    """Parse an SRT file and yield start time, end time, and text for each subtitle entry."""
    with open(filename, 'r', encoding='utf-8') as file:
        content = file.read().strip()

    content = preprocess_srt_content(content)
    subtitles = content.split('\n\n')
    
    for subtitle in subtitles:
        lines = subtitle.split('\n')
        if len(lines) >= 3:
            times = lines[1].split(' --> ')
            if len(times) == 2:
                start_time = parse_time(times[0].strip())
                end_time = parse_time(times[1].strip())
                text_lines = lines[2:]
                # Apply centering to each individual line within the subtitle
                centered_lines = [center_text(remove_html_tags(line), max_chars) for line in text_lines]
                centered_text = '\n'.join(centered_lines)  # Combine lines back into single text block
                yield start_time, end_time, centered_text
                print(f"Start time: {start_time} \n End time: {end_time} \n Text:/n {centered_text} \n Length text: {len(centered_text)}")
            else:
                print(f"Unexpected format in time line: {lines[1]}")
        else:
            print(f"Unexpected format in subtitle block: {subtitle}")
            

def write_to_input_file(text):
    with open('input.txt', 'w') as file:
        file.write(text.strip() + '\n')  # Ensure only necessary newlines are added
    if text == ' ':
        print("Clearing the display.")
    else:
        print(f"Displaying text: {text}")

def write_to_input_file_lines(lines):
    with open('input.txt', 'w') as file:
        for line in lines:
            file.write(line + "\n")
            print(line + "\n")
        if line == ' ':
            print("Clearing the display.")
        else:
            print(f"Displaying text: {lines}")

# Using the lines from earlier
def split_text_into_lines(text, max_chars_per_line):
    words = text.split()
    lines = []
    current_line = ""
    
    for word in words:
        # Check if adding the next word exceeds the line length
        if len(current_line + " " + word) > max_chars_per_line:
            lines.append(current_line)
            current_line = word
        else:
            if current_line:  # If not the first word in the line
                current_line += " "
            current_line += word
            
    # Don't forget to add the last line
    lines.append(current_line)
    
    return lines

def main(srt_filename, audio_filename, max_chars):
    play_audio(audio_filename)  # Assuming this is correctly implemented elsewhere
    previous_end_time = None

    for start_time, end_time, text in parse_srt(srt_filename, max_chars):
        # Center each line of text and write to file
        centered_text = center_text(text, max_chars)
        print("Centered text: " + "\n" + centered_text)

        if previous_end_time is None:
            wait_time = start_time.total_seconds()
        else:
            wait_time = max(0, (start_time - previous_end_time).total_seconds())

        if wait_time > 0:
            write_to_input_file(' ')  # Clear the display during the blank moment
        print("Waiting time: " + str(wait_time))
        time.sleep(wait_time)

        # Avoid writing extra new lines if text is empty
        with open("input.txt", "w", encoding='utf-8') as out_file:
            ##out_file.write(centered_text + "\n\n")
            out_file.write(centered_text)

        time.sleep((end_time - start_time).total_seconds())
        previous_end_time = end_time

    # Clear the display after the last subtitle
    with open("input.txt", "w", encoding='utf-8') as out_file:
        out_file.write(' ')


if __name__ == "__main__":
    srt_filename = 'files/subtitles.srt'
    audio_filename = 'files/audio.wav'
    max_chars = int(sys.argv[1])
    main(srt_filename, audio_filename,max_chars)
