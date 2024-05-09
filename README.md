# Raspberry Pi LED Matrix Display Project

This project enables a Raspberry Pi 4, equipped with an Adafruit hat, to control LED matrices and play synchronized audio using a USB Sabrent sound adapter. The system utilizes the rpi-rgb-led-matrix library by hzeller for matrix control, enhanced with custom script management for ease of use and functionality.

## Prerequisites

- Raspberry Pi 4
- Adafruit RGB Matrix Hat
- LED matrix panels compatible with the hat
- USB Sabrent sound adapter
- Raspberry Pi set up for headless access

## Installation

1. **Clone the repository:**

   Start by cloning this repository to your Raspberry Pi. Open a terminal and run:

```
git clone https://github.com/acastles91/subtitles.git
cd subtitles
```

2. **Run the installation script:**

Execute the `install.sh` script to set up necessary dependencies and configurations:

```
./install.sh
```

3. **Configure Audio Output:**

Copy the provided `asound.conf` file to the `/etc/` directory to configure ALSA to use the USB audio device:

```
sudo cp asound.conf /etc/
```

4. **Set up the systemd service:**

Install `start-screen.service` to initialize the system automatically at boot. Make sure to edit the service file to update any absolute paths to match the locations within your repository:

```
sudo cp start-screen.service /etc/systemd/system/
sudo systemctl enable start-screen.service
sudo systemctl start start-screen.service
```

## Usage

To run the program:

1. **Launch the script:**

Execute `launch-script.sh`, which orchestrates the start-up sequence:

```
./launch-script.sh
```

This script performs the following actions:
- Calls `command.sh`, which in turn executes the `subtitle` binary with parameters for the LED matrix, such as font, color, outline, etc. The parameter `-e` specifically sets the distance between two lines of text.
- Executes `srt-parser.py`, passing it the maximum characters display limit (54 by default), calculated based on LED count, panel number, and font pixel width.

2. **Understanding Parameters:**

The parameters for `command.sh` are detailed in the [official hzeller's rpi-rgb-led-matrix library documentation](https://github.com/hzeller/rpi-rgb-led-matrix). Modify these parameters based on your specific setup and preferences.

3. **File Management:**

The script reads audio (`audio.wav`) and subtitle (`subtitles.srt`) files from the `files` folder. It analyzes each block of subtitles and outputs formatted text to `input.txt`, centered and adjusted according to the passed character width.

## Accessing Raspberry Pi in Headless Mode

This project is designed to be run on a Raspberry Pi configured for headless operation. For guidance on setting up your Raspberry Pi in headless mode, you can refer to this detailed guide:

[Setting up Raspberry Pi in Headless Mode](https://www.raspberrypi.org/documentation/configuration/wireless/headless.md)

[Accessing the Raspberry Pi via SSH](https://www.raspberrypi.com/documentation/computers/remote-access.html)

## Notes

- Ensure that the subtitles file `subtitles.srt` is formatted correctly. The parser requires perfectly formatted blocks of text; otherwise, it may skip blocks or report errors to the console.
- Feel free to experiment with the number of characters based on different fonts or matrix sizes by adjusting the calculations provided in `srt-parser.py`.


