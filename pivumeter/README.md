# VU Meter using LEDS on Pimoroni PHAT-BEAT

This code is a mash-up of two Pimoroni projects:

*   [An ALSA plugin VU meter](https://github.com/pimoroni/pivumeter)
*   [A Pulse-Audio VU meter](https://github.com/pimoroni/pivumeter-pulseaudio)

To conform with the licenses of the above, this project is published under GPL version 3.0.

The reasons for developing this version were:

*   There are permissions issues if trying to run PulseAudio applications if not logged on
    to the Raspberry Pi desktop.

*   The original ALSA version used WiringPi which is obsolete and not supported on Bookworm.

So this project provides an ALSA plugin which uses libgpiod to control the LEDs.

By default, the LED scale has been modified to be logerithmic in signal amplitude
rather than linear. Each extra LED represents a 12dB increase in volume. The original
linear scale can be restored by changing the definition of LOG_SCALE in the **phat-beat.c**
file to zero.

To compile, install and configure the plugin:

````
sudo apt-get install autoconf automake libtool libasound2-dev libgpiod-dev
aclocal && libtoolize
autoconf && automake --add-missing
./configure && make
sudo make install
sudo cp -v asound.conf /etc
````

It is probably also advisable to remove any **.asoundrc** file in your home folder
in case it conflicts with that in **asound.conf**.

Modifying this to support any of the other devices in the original Pimoroni repository
is left as an exercise for the reader.
