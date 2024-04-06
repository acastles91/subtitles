// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Copyright (C) 2015 Henner Zeller <h.zeller@acm.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

#include "led-matrix.h"
#include "graphics.h"

#include <algorithm>
#include <fstream>
#include <streambuf>
#include <string>
#include <iostream>

#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options] [<text>| -i <filename>]\n", progname);
  fprintf(stderr, "Takes text and scrolls it with speed -s\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
          "\t-f <font-file>    : Path to *.bdf-font to be used.\n"
          "\t-i <textfile>     : Input from file.\n"
          "\t-s <speed>        : Approximate letters per second. \n"
          "\t                    Positive: scroll right to left; Negative: scroll left to right\n"
          "\t                    (Zero for no scrolling)\n"
          "\t-l <loop-count>   : Number of loops through the text. "
          "-1 for endless (default)\n"
          "\t-b <on-time>,<off-time>  : Blink while scrolling. Keep "
          "on and off for these amount of scrolled pixels.\n"
          "\t-x <x-origin>     : Shift X-Origin of displaying text (Default: 0)\n"
          "\t-y <y-origin>     : Shift Y-Origin of displaying text (Default: 0)\n"
          "\t-t <track-spacing>: Spacing pixels between letters (Default: 0)\n"
          "\n"
          "\t-C <r,g,b>        : Text Color. Default 255,255,255 (white)\n"
          "\t-B <r,g,b>        : Background-Color. Default 0,0,0\n"
          "\t-O <r,g,b>        : Outline-Color, e.g. to increase contrast.\n"
          );
  fprintf(stderr, "\nGeneral LED matrix options:\n");
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}

static bool parseColor(Color *c, const char *str) {
  return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

static bool FullSaturation(const Color &c) {
  return (c.r == 0 || c.r == 255)
    && (c.g == 0 || c.g == 255)
    && (c.b == 0 || c.b == 255);
}

static void add_micros(struct timespec *accumulator, long micros) {
  const long billion = 1000000000;
  const int64_t nanos = (int64_t) micros * 1000;
  accumulator->tv_sec += nanos / billion;
  accumulator->tv_nsec += nanos % billion;
  while (accumulator->tv_nsec > billion) {
    accumulator->tv_nsec -= billion;
    accumulator->tv_sec += 1;
  }
}
std::string centerText(const std::string& text, int max_line_length) {
    int text_length = text.length();
    if (text_length >= max_line_length) {
        // If the text is longer or equal to the max length, return it as is.
        return text;
    }

    int padding_total = max_line_length - text_length;
    int padding_side = padding_total / 2; // Evenly distribute padding on both sides

    // Create a padded string with spaces
    return std::string(padding_side, ' ') + text + std::string(padding_total - padding_side, ' ');
}
// Read line and return if it changed.
typedef uint64_t stat_fingerprint_t;

static bool ReadSplitLineOnChange(const char *filename, std::vector<std::string*>& out,
                                  stat_fingerprint_t *last_file_status, int max_line_length) {

  struct stat sb;
  if (stat(filename, &sb) < 0) {
    perror("Couldn't determine file change");
    return false;
  }


  const stat_fingerprint_t fp = ((uint64_t)sb.st_mtime << 32) + sb.st_size;
  if (fp == *last_file_status) {
    return false; // No change according to stat()
  }

  *last_file_status = fp;
  std::ifstream fs(filename);
  if (!fs.is_open()) {
    std::cerr << "Failed to open file: " << filename << std::endl;
    return false;
  }

  std::string str((std::istreambuf_iterator<char>(fs)),
                  std::istreambuf_iterator<char>());

  size_t newline_pos = str.find('\n');
  if (newline_pos != std::string::npos) {
    *out[0] = centerText(str.substr(0, newline_pos), max_line_length); // First part until newline
    std::replace(out[0]->begin(), out[0]->end(), '\n', ' '); 
    *out[1] = centerText(str.substr(newline_pos + 1), max_line_length); // Rest after the newline
    std::replace(out[1]->begin(), out[1]->end(), '\n', ' '); 
  } else {
    *out[0] = centerText(str, max_line_length); // If no newline, center the whole string
    std::replace(out[0]->begin(), out[0]->end(), '\n', ' '); 
    out[1]->clear();
  }
  return true;
}



static bool ReadLineOnChange(const char *filename, std::string *out,
                             stat_fingerprint_t *last_file_status) {
  struct stat sb;
  if (stat(filename, &sb) < 0) {
    perror("Couldn't determine file change");
    return false;
  }
  const stat_fingerprint_t fp = ((uint64_t)sb.st_mtime << 32) + sb.st_size;
  if (fp == *last_file_status) {
    return false;  // no change according to stat()
  }

  *last_file_status = fp;
  std::ifstream fs(filename);
  std::string str((std::istreambuf_iterator<char>(fs)),
                  std::istreambuf_iterator<char>());
  //std::replace(str.begin(), str.end(), '\n', ' '); Removing this line will allow for new lines in the text file
  if (*out == str) {
    return false;  // no content change
  }
  *out = str;
  return true;
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  // If started with 'sudo': make sure to drop privileges to same user
  // we started with, which is the most expected (and allows us to read
  // files as that user).
  runtime_opt.drop_priv_user = getenv("SUDO_UID");
  runtime_opt.drop_priv_group = getenv("SUDO_GID");
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }
  
  printf("Matrix Options:\n");
  printf("  Rows: %d\n", matrix_options.rows);
  printf("  Cols: %d\n", matrix_options.cols);
  printf("  Chain length: %d\n", matrix_options.chain_length);
  printf("  Parallel: %d\n", matrix_options.parallel);
  printf("  Brightness: %d\n", matrix_options.brightness);
  printf("  PWM Bits: %d\n", matrix_options.pwm_bits);
  printf("  PWM LSB Nanoseconds: %d\n", matrix_options.pwm_lsb_nanoseconds);
  printf("  LED RGB Sequence: %s\n", matrix_options.led_rgb_sequence);
  printf("  Pixel Mapper Config: %s\n", matrix_options.pixel_mapper_config ? matrix_options.pixel_mapper_config : "None");

  printf("Runtime Options:\n");
  printf("  GPIO Slowdown: %d\n", runtime_opt.gpio_slowdown);
  printf("  Daemon: %d\n", runtime_opt.daemon);
  printf("  Drop privileges: %d\n", runtime_opt.drop_privileges);


  Color color(255, 255, 255);
  Color bg_color(0, 0, 0);
  Color outline_color(0,0,0);
  bool with_outline = false;

  const char *bdf_font_file = NULL;
  const char *input_file = NULL;
  std::string line;
  bool xorigin_configured = false;
  int x_orig = 0;
  int y_orig = 0;
  int letter_spacing = 0;
  float speed = 7.0f;
  int loops = -1;
  int blink_on = 0;
  int blink_off = 0;
  int number_modules = matrix_options.chain_length;
  int char_per_module = 10;

  //Multi-line support
  std::string firstLine, secondLine;
  std::vector<std::string*> lines{&firstLine, &secondLine};

  int opt;
  while ((opt = getopt(argc, argv, "x:y:f:C:B:O:t:s:l:b:i:")) != -1) {
    switch (opt) {
    case 's': speed = atof(optarg); break;
    case 'b':
      if (sscanf(optarg, "%d,%d", &blink_on, &blink_off) == 1) {
        blink_off = blink_on;
      }
      fprintf(stderr, "hz: on=%d off=%d\n", blink_on, blink_off);
      break;
    case 'l': loops = atoi(optarg); break;
    case 'x': x_orig = atoi(optarg); xorigin_configured = true; break;
    case 'y': y_orig = atoi(optarg); break;
    case 'f': bdf_font_file = strdup(optarg); break;
    case 'i': input_file = strdup(optarg); break;
    case 't': letter_spacing = atoi(optarg); break;
    case 'C':
      if (!parseColor(&color, optarg)) {
        fprintf(stderr, "Invalid color spec: %s\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 'B':
      if (!parseColor(&bg_color, optarg)) {
        fprintf(stderr, "Invalid background color spec: %s\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 'O':
      if (!parseColor(&outline_color, optarg)) {
        fprintf(stderr, "Invalid outline color spec: %s\n", optarg);
        return usage(argv[0]);
      }
      with_outline = true;
      break;
    default:
      return usage(argv[0]);
    }
  }

  int max_line_length = char_per_module * number_modules;

  stat_fingerprint_t last_change = 0;

  if (input_file) {
    if (!ReadSplitLineOnChange(input_file, lines, &last_change, max_line_length)) {
      fprintf(stderr, "Couldn't read file '%s'\n", input_file);
      return usage(argv[0]);
    }
  }

  else {
    for (int i = optind; i < argc; ++i) {
      line.append(argv[i]).append(" ");
    }

    if (line.empty()) {
      fprintf(stderr, "Add the text you want to print on the command-line or -i for input file.\n");
      return usage(argv[0]);
    }
  }

  if (bdf_font_file == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return usage(argv[0]);
  }

  /*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
  rgb_matrix::Font font;
  if (!font.LoadFont(bdf_font_file)) {
    fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
    return 1;
  }

  /*
   * If we want an outline around the font, we create a new font with
   * the original font as a template that is just an outline font.
   */
  rgb_matrix::Font *outline_font = NULL;
  if (with_outline) {
    outline_font = font.CreateOutlineFont();
  }

  RGBMatrix *canvas = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);

 // if (canvas == NULL)
 //   return 1;

  if (canvas != NULL) {
    printf("Canvas created successfully.\n");
    printf("  Canvas width: %d, Canvas height: %d\n", canvas->width(), canvas->height());
  } else {
    printf("Failed to create canvas.\n");
    return 1;  // or handle the error as appropriate
  }


  const bool all_extreme_colors = (matrix_options.brightness == 100)
    && FullSaturation(color)
    && FullSaturation(bg_color)
    && FullSaturation(outline_color);
  if (all_extreme_colors)
    canvas->SetPWMBits(1);

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  printf("CTRL-C for exit.\n");

  // Create a new canvas to be used with led_matrix_swap_on_vsync
  FrameCanvas *offscreen_canvas = canvas->CreateFrameCanvas();

  const int scroll_direction = (speed >= 0) ? -1 : 1;
  speed = fabs(speed);
  int delay_speed_usec = 1000000;
  if (speed > 0) {
    delay_speed_usec = 1000000 / speed / font.CharacterWidth('W');
  }

  if (!xorigin_configured) {
    if (speed == 0) {
      // There would be no scrolling, so text would never appear. Move to front.
      x_orig = with_outline ? 1 : 0;
    } else {
      x_orig = scroll_direction < 0 ? canvas->width() : 0;
    }
  }

  //int x = x_orig;
  //int y = y_orig;
  int x = 0;
  int y = 0;  
  int length = 0;

  struct timespec next_frame = {0, 0};

  uint64_t frame_counter = 0;
  printf("Text Properties:\n");
  printf("  Color: (%d, %d, %d)\n", color.r, color.g, color.b);
  printf("  Background Color: (%d, %d, %d)\n", bg_color.r, bg_color.g, bg_color.b);
  printf("  Outline Color: (%d, %d, %d)\n", outline_color.r, outline_color.g, outline_color.b);
  printf("  Letter Spacing: %d\n", letter_spacing);
  printf("  X Origin: %d, Y Origin: %d\n", x_orig, y_orig);

  if (!font.LoadFont(bdf_font_file)) {
          fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
          return 1;
      } else {
          printf("Font '%s' loaded.\n", bdf_font_file);
          printf("  Font height: %d\n", font.height());
      }

      if (with_outline) {
          printf("Outline font created based on '%s'.\n", bdf_font_file);
      }
  while (!interrupt_received && loops != 0) {
    if (input_file) {
      ReadSplitLineOnChange(input_file, lines, &last_change, max_line_length);
      //x = x_orig;
      //y = y_orig;
      }
    
//    if (input_file){    //if (input_file && ReadLineOnChange(input_file, &line, &last_change)) {
//      ReadLineOnChange(input_file, &line, &last_change);
//      x = x_orig;
//    }
    ++frame_counter;
    offscreen_canvas->Fill(bg_color.r, bg_color.g, bg_color.b);
    const bool draw_on_frame = (blink_on <= 0)
      || (frame_counter % (blink_on + blink_off) < (uint64_t)blink_on);
    if (draw_on_frame) {

//std::cout << "Line 0: '" << *lines[0] << "'" << std::endl;
//std::cout << "Line 1: '" << *lines[1] << "'" << (lines[1]->empty() ? " (empty)" : " (non-empty)") << std::endl;
bool has_two_lines = !lines[1]->empty() && std::find_if(lines[1]->begin(), lines[1]->end(), [](unsigned char c) { return !std::isspace(c); }) != lines[1]->end();

//        bool has_two_lines = !lines[1]->empty();
        int baseline_y;
        int linespace = baseline_y / 4;
        if (has_two_lines){
          baseline_y = y + font.baseline();
          int second_line_y = y + 2 * font.baseline() + linespace;
            if (outline_font) {
                rgb_matrix::DrawText(offscreen_canvas, *outline_font,
                                     x - 1, second_line_y,
                                     outline_color, nullptr,
                                     lines[1]->c_str(), letter_spacing - 2);
            }
            rgb_matrix::DrawText(offscreen_canvas, font,
                                 x, second_line_y,
                                 color, nullptr,
                                 lines[1]->c_str(), letter_spacing);

            if (outline_font) {
                rgb_matrix::DrawText(offscreen_canvas, *outline_font,
                                     x - 1, baseline_y,
                                     outline_color, nullptr,
                                     lines[0]->c_str(), letter_spacing - 2);
            }
            rgb_matrix::DrawText(offscreen_canvas, font,
                                          x, baseline_y,
                                          color, nullptr,
                                          lines[0]->c_str(), letter_spacing);

        
        } else {
            //printf("Drawing single line\n");
            baseline_y = y + 2 * font.baseline() - linespace;
            if (outline_font) {
                rgb_matrix::DrawText(offscreen_canvas, *outline_font,
                                     x - 1, baseline_y,
                                     outline_color, nullptr,
                                     lines[0]->c_str(), letter_spacing - 2);
            }
            rgb_matrix::DrawText(offscreen_canvas, font,
                                 x, baseline_y,
                                 color, nullptr,
                                 lines[0]->c_str(), letter_spacing);
        }
    }
//    if (draw_on_frame) {
//
//        bool has_two_lines = !lines[1]->empty();
//        int baseline_y = y + font.baseline();
//        int linespace = baseline_y / 4;
//
//        if (has_two_lines) {
//          baseline_y = y + font.baseline();
//          int second_line_y = y + 2 * font.baseline() + linespace;
//            if (outline_font) {
//                rgb_matrix::DrawText(offscreen_canvas, *outline_font,
//                                     x - 1, second_line_y,
//                                     outline_color, nullptr,
//                                     lines[1]->c_str(), letter_spacing - 2);
//            }
//            rgb_matrix::DrawText(offscreen_canvas, font,
//                                 x, second_line_y,
//                                 color, nullptr,
//                                 lines[1]->c_str(), letter_spacing);
//
//            if (outline_font) {
//                rgb_matrix::DrawText(offscreen_canvas, *outline_font,
//                                     x - 1, baseline_y,
//                                     outline_color, nullptr,
//                                     lines[0]->c_str(), letter_spacing - 2);
//            }
//            rgb_matrix::DrawText(offscreen_canvas, font,
//                                          x, baseline_y,
//                                          color, nullptr,
//                                          lines[0]->c_str(), letter_spacing);
//
//        
//        } else {
//            printf("Drawing single line\n");
//            baseline_y = y + 2 * font.baseline();
//            if (outline_font) {
//                rgb_matrix::DrawText(offscreen_canvas, *outline_font,
//                                     x - 1, baseline_y,
//                                     outline_color, nullptr,
//                                     lines[0]->c_str(), letter_spacing - 2);
//            }
//            rgb_matrix::DrawText(offscreen_canvas, font,
//                                 x, baseline_y,
//                                 color, nullptr,
//                                 lines[0]->c_str(), letter_spacing);
//        }
//    }


    // Make sure render-time delays are not influencing scroll-time
    if (speed > 0) {
      if (next_frame.tv_sec == 0 && next_frame.tv_nsec == 0) {
        // First time. Start timer, but don't wait.
        clock_gettime(CLOCK_MONOTONIC, &next_frame);
      } else {
        add_micros(&next_frame, delay_speed_usec);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_frame, NULL);
      }
    }
    // Swap the offscreen_canvas with canvas on vsync, avoids flickering
    offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
    //if (speed <= 0) pause();  // Nothing to scroll.
  }

// Finished. Shut down the RGB matrix.


//    if (draw_on_frame) {
//      if (outline_font) {
//        // The outline font, we need to write with a negative (-2) text-spacing,
//        // as we want to have the same letter pitch as the regular text that
//        // we then write on top.
//        rgb_matrix::DrawText(offscreen_canvas, *outline_font,
//                             x - 1, y + font.baseline(),
//                             outline_color, NULL,
//                             line.c_str(), letter_spacing - 2);
//      }
//
//      // length = holds how many pixels our text takes up
//      length = rgb_matrix::DrawText(offscreen_canvas, font,
//                                    x, y + font.baseline(),
//                                    color, NULL,
//                                    line.c_str(), letter_spacing);
//    }
//
//    x += scroll_direction;
//    if ((scroll_direction < 0 && x + length < 0) ||
//        (scroll_direction > 0 && x > canvas->width())) {
//      x = x_orig + ((scroll_direction > 0) ? -length : 0);
//      if (loops > 0) --loops;
//    }
//
//    // Make sure render-time delays are not influencing scroll-time
//    if (speed > 0) {
//      if (next_frame.tv_sec == 0 && next_frame.tv_nsec == 0) {
//        // First time. Start timer, but don't wait.
//        clock_gettime(CLOCK_MONOTONIC, &next_frame);
//      } else {
//        add_micros(&next_frame, delay_speed_usec);
//        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_frame, NULL);
//      }
//    }
//    // Swap the offscreen_canvas with canvas on vsync, avoids flickering
//    offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
//    //if (speed <= 0) pause();  // Nothing to scroll.
//  }
//
  // Finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  return 0;
}