# Python program to control MPC using buttons on PHAT-BEAT

A fairly simple program to use the buttons on a PHAT-BEAT
to control the operation of
[Music Player Daemon](https://mpd.readthedocs.io/en/stable/user.html).

The buttons perform the following actions:

*   **Power**: Exit the program. TODO: Perform a system shutdown.
*   **Fast Forward**: Advance to next track (next internet station).
*   **Play**: Start or stop playing.
*   **Rewind**: Go back to previous track (previous internet station).
*   **Plus**: Increase playback volume.
*   **Minus**: Decrease playback volume.

The program outputs status messages to /dev/serial

To install the program prerequisites:

````
sudo apt update
sudo apt install python3-serial python3-mpd python3-gpiozero
````

TODO: Write a **systemd** script to auto-run this program.
