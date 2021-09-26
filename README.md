# Arduino2keypad v2 - WORK IN PROGRESS

I'm in the process of adding additional commands to trigger certain events on the keypads; certain bits in the F7 messages have been mapped since the original author created this code.

Why am I doing this?  I have a couple of Honeywell keypads and I want to use them with Home Assistant.  Or something like that.  Who knows. :)

# todo: 

- add fire alarm command (F7 msg byte 07 aka BYTE2, bit 1 - keypad does the typical 3-beep fire alarm tone)
- add standard alarm command (F7 msg byte 08 aka BYTE3, bit 1 - keypad does high-low alternating alarm tone)
- add programming mode & blinking cursor position (F7 msg byte 09 bit 1 set to 1, byte 10 prompt position)
- fully map byte 07 (BYTE2) and byte 08 (BYTE3) with information gathered from Dilbert66's esphome-vistaECP code (thanks!)
- add a parameter to send text to a specific keypad on the bus (byte 03, currently hardcoded to FF for all KPs; may be possible to direct messages to a specific keypad)

The other bits such as Low Batt, Bypass, AC Loss, Instant, Stay, Chime Mode and others may not have a visual effect on a 6160/6460 alpha keypad, but I believe those bits make the text indicators light up on the "fixed" (aka non-dot-matrix) keypads like the 6150.

There may be some other undocumented bits which might make the fixed display keypads show other messages such as "CANCELED", "NIGHT", "PHONE", "TEST", "CHECK", etc.  Same goes for the 6160CR-2 keypads with additional LEDs: "SILENCED", "SUPERVISORY", "TROUBLE".  I don't have any of those keypads, so if you want to test this, be my guest!

Many thanks to TomVickers for the original code, Dilbert66 for the esphome-vistaECP code which answered a LOT of my questions, as well as markkimsal for the TANC-security/keypad-firmware code.

# --original README below--

This repository is the companion of https://github.com/TomVickers/RPIalarm

This repository contains code to interface a 6160 alarm keypad with an Arduino.  If you are interested in using a 6160 alarm keypad (commonly found connected to a variety of Honeywell alarm systems like the Vista-20p), this code may be useful to you.

As with most things in life, my work on this is simply an extension of the work by others.  I found Mark Kimsal's website (http://getatanc.com/) and github repository (https://github.com/TANC-security/keypad-firmware) very helpful.  Most of what you will find here, I found first on his pages.  I have extended it a bit, and worked out some details of communicating directly with the keypad (his task involved talking to the alarm as a keypad).

The doc directory contains notes, circuit diagrams, etc.  If you find something is wrong (or learn something new), please let me know and I will try to update them as we learn more about this device.

The ArduinoProj directory contains the Arduino project named USB2keybus.  I build it using Arduino software (version 1.8.5) on a Mega 2560.  It will probably run on other Arduino processors with minor changes.  I can also build it on my alarm Raspberry PI using the provided Makefile.  There are some notes in the comments at the top of the Makefile that indicate which packages you must install to enable cross compiling for the Arduino.  You will notice that the project uses a modified version of the SoftwareSerial lib.  All the modifications in my ModSoftwareSerial files are marked with the comment NON_STANDARD (in case you want to port these changes to a different version of SoftwareSerial).

--------------- NOTE: Beta code ------------------------

This code is a work in progress.  A few features are not yet complete, but it appears to be stable.  I am currently using it as a bi-directional parser between a Raspberry Pi 3 USB serial port at 115200 baud and a 6160 keypad.

