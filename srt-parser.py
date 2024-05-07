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
        padding = (max_chars - line_length) // 2
        return ' ' * padding + text + ' ' * padding
    return text

def preprocess_srt_content(content):
    """Collapse multiple consecutive empty lines into a single empty line."""
    # Split the content into lines, remove empty lines, and rejoin with a single empty line
    # This effectively collapses multiple consecutive empty lines into one
    processed_content = '\n\n'.join([block for block in content.split('\n\n') if block.strip()])
    return processed_content


#def parse_time(time_str):
#    """Convert an SRT time string into a datetime object."""
#    print(f"Parsing time string: {time_str}")  # Debug print
#    try:
#        hours, minutes, seconds_milliseconds = time_str.split(':')
#        seconds, milliseconds = seconds_milliseconds.split(',')
#        return datetime.combine(datetime.today(), dt_time(hour=int(hours), minute=int(minutes), second=int(seconds), microsecond=int(milliseconds)*1000))
#    except ValueError as e:
#        print(f"Error parsing time string '{time_str}': {e}")
#        raise

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

def parse_srt(filename):
    """Parse an SRT file and yield start time, end time, and text for each subtitle entry."""
    with open(filename, 'r', encoding='utf-8') as file:
        content = file.read().strip()

    # Preprocess content to ensure correct block separation
    content = preprocess_srt_content(content)
    # Split the content by double newlines to separate each subtitle block
    subtitles = content.split('\n\n')
    
    for subtitle in subtitles:
        lines = subtitle.split('\n')
        if len(lines) >= 3:
            sequence_number = lines[0]
            times = lines[1].split(' --> ')
            if len(times) == 2:  # Ensure there are exactly two times (start and end)
                start_time = parse_time(times[0].strip())
                end_time = parse_time(times[1].strip())
                text_lines = lines[2:]
                text_lines = [center_text(line, max_chars) for line in text_lines]
                text = '\n'.join(text_lines)  # Keep original lines as they are
                text = remove_html_tags(text)
                yield start_time, end_time, text
            else:
                print(f"Unexpected format in time line: {lines[1]}")
        else:
            print(f"Unexpected format in subtitle block: {subtitle}")

#def parse_srt(filename):
#    """Parse an SRT file and yield start time, end time, and text for each subtitle."""
#    with open(filename, 'r', encoding='utf-8') as file:
#        content = file.read().strip()
#    
#    # Preprocess content to ensure correct block separation
#    content = preprocess_srt_content(content)
#    # Split the content by double newlines to separate each subtitle block
#    subtitles = content.split('\n\n')
#    
#    for subtitle in subtitles:
#        lines = subtitle.split('\n')
#        if len(lines) >= 3:
#            sequence_number = lines[0]
#            times = lines[1].split(' --> ')
#            if len(times) == 2:  # Ensure there are exactly two times (start and end)
#                start_time = parse_time(times[0].strip())
#                end_time = parse_time(times[1].strip())
#                text = ' '.join(lines[2:]).replace('\n', ' ')
#                text = remove_html_tags(text)
#                yield start_time, end_time, text
#            else:
#                print(f"Unexpected format in time line: {lines[1]}")
#        else:
#            print(f"Unexpected format in subtitle block: {subtitle}")

#def parse_srt(filename, first_line_length):
#    """Parse an SRT file and yield start time, end time, and text for each subtitle,
#       with the text split according to a predefined length for the first line."""
#    with open(filename, 'r', encoding='utf-8') as file:
#        content = file.read().strip()
#
#    # Preprocess content to ensure correct block separation
#    content = preprocess_srt_content(content)
#    # Split the content by double newlines to separate each subtitle block
#    subtitles = content.split('\n\n')
#    
#    for subtitle in subtitles:
#        lines = subtitle.split('\n')
#        if len(lines) >= 3:
#            sequence_number = lines[0]
#            times = lines[1].split(' --> ')
#            if len(times) == 2:  # Ensure there are exactly two times (start and end)
#                start_time = parse_time(times[0].strip())
#                end_time = parse_time(times[1].strip())
#                text = ' '.join(lines[2:]).replace('\n', ' ')
#                text = remove_html_tags(text)
#
#                # Split text into two lines based on the predefined length
#                first_line, second_line = split_text_by_length(text, first_line_length)
#                # Combine the two lines with a newline character
#                text = first_line + "\n" + second_line
#
#                yield start_time, end_time, text
#            else:
#                print(f"Unexpected format in time line: {lines[1]}")
#        else:
#            print(f"Unexpected format in subtitle block: {subtitle}")
            
# Function to write text to input.txt
def write_to_input_file(text):
    with open('input.txt', 'w') as file:
        file.write(text)
    if text == ' ':
        print("Clearing the display.")
    else:
        print(f"Displaying text: {text}")

def write_to_input_file_lines(lines):
    with open('input.txt', 'w') as file:
        for line in lines:
            file.write(line + "\n")

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
        
        for start_time, end_time, text in parse_srt(srt_filename):
            # Split the text into lines that fit your LED matrix
            #lines = split_text_into_lines(text, max_chars_per_line)
            lines = split_text_by_length(text, max_chars)
            # Write the lines to input.txt for your C++ program to display
            # Calculate wait time until the next subtitle
        
            if previous_end_time is None:
                # If this is the first subtitle, wait until its start time
                wait_time = start_time.total_seconds()
            else:
                # Otherwise, wait until the start time of the next subtitle,
                # but ensure there's no negative wait time in case subtitles overlap or are back-to-back
                wait_time = max(0, (start_time - previous_end_time).total_seconds())
        
             # Clear the display if there's a gap between the previous subtitle and the next
            if previous_end_time is not None and wait_time > 0:
                write_to_input_file(' ')  # Clear the display during the blank moment
            print("Waiting wait_time " + str(wait_time))
            time.sleep(wait_time)  # Wait until it's time for the next subtitle
        
            #write_to_input_file_lines(lines)
            #write_to_input_file(text)  # Show the subtitle text
            with open("input.txt", "w", encoding='utf-8') as out_file:
                out_file.write(text + "\n\n")  # Write each subtitle entry separated by double newlines
        
            # Wait for the duration of the subtitle
            duration = (end_time - start_time).total_seconds()
            time.sleep(duration)
        
            previous_end_time = end_time  # Update the end time for the next loop iteration
        
            # Clear the display after the last subtitle
            with open("input.txt", "w", encoding='utf-8') as out_file:
                out_file.write(' ')

if __name__ == "__main__":
    srt_filename = 'files/subtitles.srt'
    audio_filename = 'files/audio.wav'
    max_chars = int(sys.argv[1])
    main(srt_filename, audio_filename,max_chars)
