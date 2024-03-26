#!/usr/bin/python3
#
# Python program for controlling MPD in response to phat-beat button presses
#
# (c) Copyright 2024, Memotech-Bill. SPDX short identifier: BSD-3-Clause
#
import mpd
import gpiozero
import serial
import datetime
import os
import sys

class PiRadio:
    def __init__ (self):
        self.mpc = mpd.MPDClient ()
        self.mpc.connect ('localhost', 6600)
        self.upd_status ()
        self.last_song = ''
        self.lcd = serial.Serial ('/dev/serial0', 115200)
        self.hold = None

    def upd_status (self):
        self.status = self.mpc.status ()
        print (self.status)
        print (self.mpc.currentsong ())

    def display (self):
        if self.hold is not None:
            if datetime.now () < self.hold:
                return
            self.hold = None
            self.last_song = ''
        if self.status['state'] == 'play':
            info = self.mpc.currentsong ()
            song = info.get ('title', info.get ('name', os.path.basename (info.get ('file', 'Unknown'))))
            if song != self.last_song:
                self.lcd.write ((song + '\n').encode ('utf-8'))
                self.last_song = song

    def message (self, txt, hold = 30):
        self.lcd.write ((txt + '\n').encode ('utf-8'))
        self.hold = datetime.now () + datetime.timedelta (seconds = hold)

    def volume_up (self):
        vol = int (self.status['volume'])
        if vol < 100:
            if vol < 95:
                vol += 5
            else:
                vol = 100
            self.mpc.setvol (vol)
            self.upd_status ()
            self.message ('Volume {:d}'.format (vol))

    def volume_down (self):
        vol = int (self.status['volume'])
        if vol > 0:
            if vol > 5:
                vol -= 5
            else:
                vol = 0
            self.mpc.setvol (vol)
            self.upd_status ()
            self.message ('Volume {:d}'.format (vol))

    def on_play (self):
        if self.status['state'] == 'play':
            self.mpc.stop ()
            self.message ('Paused')
        else:
            self.mpc.play ()
        self.upd_status ()

    def previous (self):
        self.mpc.previous ()
        self.upd_status ()
        info = self.mpc.currentsong ()
        station = info.get ('name', os.path.basename (info.get ('file', 'Unknown')))
        self.message (station)

    def next (self):
        self.mpc.next ()
        self.upd_status ()
        info = self.mpc.currentsong ()
        station = info.get ('name', os.path.basename (info.get ('file', 'Unknown')))
        self.message (station)

    def disconnect (self):
        self.mpc.stop ()
        self.mpc.disconnect ()
        
def on_fwd (btn):
    print ('Forward button pressed.')
    piradio.next ()

def on_rwd (btn):
    print ('Rewind button pressed.')
    piradio.previous ()

def on_ply (btn):
    print ('Play button pressed.')
    piradio.on_play ()

def on_vup (btn):
    print ('Volume Up button pressed')
    piradio.volume_up ()

def on_vdn (btn):
    print ('Volume Down button pressed')
    piradio.volume_down ()

def on_pwr (btn):
    print ('Power button pressed')
    global run
    run = False

if __name__ == "__main__":
    piradio = PiRadio ()
    
    btn_fwd = gpiozero.Button (5)
    btn_rwd = gpiozero.Button (13)
    btn_ply = gpiozero.Button (6)
    btn_vup = gpiozero.Button (16)
    btn_vdn = gpiozero.Button (26)
    btn_pwr = gpiozero.Button (12)

    btn_fwd.when_pressed = on_fwd
    btn_rwd.when_pressed = on_rwd
    btn_ply.when_pressed = on_ply
    btn_vup.when_pressed = on_vup
    btn_vdn.when_pressed = on_vdn
    btn_pwr.when_pressed = on_pwr

    try:
        song1 = ''
        run = True
        while run:
            piradio.display ()
        piradio.disconnect ()
    except KeyboardInterrupt:
        pass
    print ('Exit program')
    sys.exit (0)
