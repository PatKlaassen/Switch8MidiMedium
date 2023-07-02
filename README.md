# Switch8MidiMedium
ESP32 ArduinoIDE based device for switching midi device presets

Basic features
- 8 presets
- 10 banks
- Simple midi program change messages
- Bank lock feuture for allowing 10 presets for dummy mode
- Multiple color scheme's
- Mute feature (bank down hold)

Switch8-MidiSwitcher-ESP32-Buildlist.xlsx
Biuldlist, enclosure layout and pin usage 

Switch8ESP32.fzz (Switch8ESP32_bb.png)
Contains the Fritzing schematic for the breadboard prototype

Switch8-ESP2-Midi.ino
The ArduinoIDE sketch

In the first version the preset buttons only trigger a single midi program change message. In a future release a full blown midi interface will be available.
This interface will allow the user to create custom presets where multiple program/control chang messages can be send to multiple devices.

This switcher was devloped for switching presets on a Boss GT1000Core. My son uses it as a daily board that is compact, but packs a lot of punch.
https://www.instagram.com/rks_musiclife/?hl=en

My socials
https://www.instagram.com/patwklaassen/?hl=en
https://www.youtube.com/@PatrickKlaassen/videos
