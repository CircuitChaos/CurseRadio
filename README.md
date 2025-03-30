# CurseRadio – text-mode transceiver controller, keyer and logger

## Objective

CurseRadio is a text-mode, ncurses-based program to control a CAT-capable transceiver. It was written with contests in mind. In theory it can be used as a general-purpose Cabrillo logger (it doesn't support ADI), but in practice it doesn't allow sending any report other than 59 or 599 (it's on the TODO list).

It was written as a private project to be used exclusively with Yaesu FT-891, but it should work also with other CAT-capable radios. See the CAT section below for details.

It doesn't check the radio model, so if you're not using FT-891, do your own experiments.

It can be extended to support other radios in the future, if there's a demand for it. Perhaps a hamlib integration could be done as well.

Despite my radio and shack acting cursed sometimes, the name comes from the fact that it uses ncurses interface. ncurses is used in a simple, basic way – the first, general idea was to make an interface similar to *irssi*, but it turns out that a simple interface, with radio status and meters on top and text UI on the rest of the screen, is sufficient, and maybe even controlling the program with keypresses instead of text commands is more efficient.

Note that this is **not** a generic radio controller. It can do basic, most useful things, but many other things require pressing buttons and turning knobs on the radio.

Also note that this is **not** a generic logger (but it can be extended in the future, if there's a need). Main purpose of the built-in logger is to aid in contesting.

## Features

Current list of features:

* Control the radio via the CAT interface (frequency, mode, band, VFO swap, CW ZIN)
* Read radio parameters from the CAT interface (meters, frequency, band) and show them on the status bar
* Send CW presets (hard-coded in the program) and free (entered from keyboard) texts
* Keep track of contest exchange numbers
* Log QSOs
* Check if the callsign already exists in the log

Keying the radio for CW is done by toggling the DTR line on another serial port.

## Command line arguments

The program can be run without arguments, but then it won't be useful. A list of command-line arguments can be obtained by typing curseradio -h. Some arguments that are not obvious are described below.

* -s &lt;callsign&gt; is used to specify your callsign (for example: -s SP5XXX). Callsign is used in presets and logging. If callsign is not specified, then preset, logging and callsign check functions will be disabled.

* -f &lt;file&gt; is used to specify Cabrillo file to append to. If it's not specified, then logging and callsign check functions will be disabled. Logging event appends a QSO line to this file (it doesn't rewrite anything). Callsign check function reads the file every time. Therefore, it is safe to edit the file in external editor between using these functions (you don't need to exit the program, reload the file, etc.).

Note that the program doesn't write the Cabrillo header, so you have to do it yourself (at any point – before the contest, during, after, doesn't matter).

* -c &lt;port&gt; and -b &lt;rate&gt; pair is used to specify CAT serial port and baudrate. FT-891 exposes two serial ports – one is used for CAT control and another for PTT control. If you want to use permanent port names (I use /dev/ttyFTCAT and /dev/ttyFTPTT), see the section called *udev* below. If CAT port is not specified, then most program functions won't be enabled.

Other serial parameters are not editable (they're fixed to 8 data bits, 1 stop bit, no parity checking, no flow control), and only baudrates supported by FT-891 are supported by this program (4800 bps, 9600 bps, 19200 bps, 38400 bps; I recommend sticking to 38400 bps).

* -p &lt;port&gt; is used to specify the port used to key the radio (for CW) by controlling the DTR line. Note that PC control has to be enabled in radio settings. Also note that when the port is opened, both DTR and RTS lines are momentarily brought up, so the radio will be keyed for a moment when you start the program with this option. This is how the Linux serial driver works and it can't be changed without modifying the driver code.

Various workarounds for that can be implemented (like changing configuration via CAT before opening the port, implementing some system-wide PTT daemon, etc.), but they're not done now.

* -w &lt;wpm&gt; is used to specify the initial speed of the keyer built into the program, in words per minute (WPM). Note that this is **not** the speed of the keyer built into the radio. Due to CAT interface limitations (KY command disappointed me…), text to be sent as CW is paced by the program, not by the radio. Speed can be changed during operation and if it's not specified, a default of **25** WPM is used.

* -P &lt;prefix&gt;, -I &lt;infix&gt;, -S &lt;suffix&gt; are used to track exchange group sent in contests. All three arguments are optional, but at least one must exist for exchange group tracking to be enabled. Prefix and suffix parts are fixed (do not change during the contest). Infix part normally contains a number, optionally prefixed with zeroes, and is incremented after every logged QSO. One example: -P PFX -I 009. Exchange will be PFX009 in the first QSO, PFX010 in the second QSO, and so on.

## Text UI

When the program is started, it displays a black screen with a blue status bar. On the statusbar, frequency, mode and meters are shown – in RX mode it's the signal level, in TX mode it's the IDD (drain current of the final transistors), ALC, compressor level, output power, and of course SWR. Meters are updated every 100 ms (this value is fixed, but might be made configurable from the command-line in the future; see `METER_POLL_INTERVAL` in `curseradio.cpp`). There's also a rotating indicator on the left of the statusbar to show that CAT is working, although after two seconds of no response from the radio a CAT timeout trips and the program exits.

Program is controlled from the keyboard. Press 'h' to see a list of keys, or 'q' to quit. Some keys and their explanations:

* n: allows you to enter a note. It's not saved anywhere, just kept on the screen. It's useful when, for example, you're receiving an exchange and want to write it down before logging it. Just press 'n', type what you like, and press Enter to end this mode.

Frequency control:

* left/right arrows: slow tuning (currently 10 Hz steps)
* up/down arrows: normal tuning (currently 100 Hz steps)
* pgup/pgdown: fast tuning (currently 1000 Hz steps)
* =: reset frequency to the lowest possible value for given band

Hint: frequency limits are defined in `band.cpp` and they're valid for IARU region 1. If your region has different limits, feel free to change them.

* b: allows to select band. A screen is presented with all bands supported by FT-891, plus a generic band and a MW band. Backspace aborts selection.

* m: allows to set mode. CAT manual for FT-891 is very unclear on how modes are mapped to numbers (different numbers map to the same mode, and so on), so if this doesn't work as expected, make sure to fill a bug report. Currently it's impossible to switch between USB and LSB – the mode is always SSB, and sideband is defined in the radio per band. Backspace aborts selection.

Note that you don't have to use 'b' or 'm'. Switching the band and mode with the radio's BAND button is sufficient – the program will read it automatically after at most 100 ms.

* v: swaps VFOs (A with B, and B with A) and their respective modes.

* p: shows a list of CW presets. See the section about presets for detail.

* 0...9: sends a CW preset from the list. 'p' doesn't have to be pressed first.

* x: shows current (next) exchange. Useful when you want to pick up the microphone and tell the number, but you don't remember it. This exchange will then be logged (and possibly incremented) when you press 'l'.

* c: checks if the callsign is already in the log. Press 'c', type the callsign (lowercase or uppercase, doesn't matter), and the program will scan the CBR file, telling you if the callsign is in the log or not.

In the future, maybe all QSOs found with this callsign will be printed as well. Another thing that's left to be done is checking this callsign in a list of known callsigns (for example, callsigns that participated in contests in previous years), to spot misspelled callsigns.

This check is also done when logging.

* l: logs the QSO in Cabrillo format. Your part is logged as your callsign, 59 or 599 (depending on the mode – 599 for CW, 59 for all others), and the next exchange group (the same as shown when pressing 'x'). After pressing 'l', you can either specify the remote callsign and received exchange (for example: SP1ZZZ 123), or remote callsign, received report, and received exchange (if the report is something else than 59, or 599 for CW). If you want to leave logging mode, just erase everything and press Enter – nothing will be logged.

Note that current time is stored when you press 'l', not when you actually type the callsign and report, so if you pressed 'l' at 12:34, but entered the callsign and exchange group at 12:36, the QSO will be logged at 12:34.

* t: sends text as CW. Pressing 'a' will abort sending.

* u and d: increases and decreases built-in CW keyer speed.

* z: zero-ins on the CW frequency. It just sends the command to the radio. It's equivalent to pressing the ZIN button.

## Presets

Currently, presets are hard-coded (can't be edited without editing the source code). It might be changed in the future. Current presets:

* 1: CQ &lt;callsign&gt; TEST
* 2: &lt;callsign&gt;
* 3: 5NN &lt;exchange&gt;
* 4: TU 5NN &lt;exchange&gt;
* 5: &lt;exchange&gt;
* 6: NR NR
* 7: AGN
* 8: QRS
* 9: QRL?
* 0: TU

&lt;callsign&gt; is the callsign entered on the command line (-s argument). &lt;exchange&gt; is the current exchange.

## Building

To build the program, you need:

* scons
* libncurses-dev
* g++
* git

On Debian, type:

`sudo apt-get install scons libncurses-dev g++ git`

Then just type `scons` and press Enter. If everything's OK, a binary will be put in `build/curseradio`. To install it system-wide, type `sudo scons install` – it will be installed in `/usr/local/bin/` directory.

This one-liner should do the job:

`git clone https://github.com/CircuitChaos/curseradio && cd curseradio && scons && sudo scons install`

Git binary is not only needed to download the latest repository version, but also to set the displayed program version.

If for whatever reason you don't want to use the scons build system, you can make a simple Makefile based on the SConstruct file. There's no magic involved there, I just find scons easiest to use.

Let me know if I forgot about any prerequisites.

## CAT

This is the list of CAT commands used by the program. It can be useful when determining if and what things have to be done to adjust it to work with other radios.

Commands implemented:

* RMn: read meter
* FA: read VFO A frequency
* FAnnnnnnnnn: set VFO A frequency
* MD0: get mode
* MD0n: set mode
* BSnn: set band
* SV: swap VFO
* ZI: zero-in

For numbers encoding specific meters, bands and modes, refer to `cat.cpp` (meterMap, bandMap, modeMap).

Note that in FT-891 sometimes two different numbers encode the same mode. I don't know why. I thought it's because of the sideband used, but it's probably more complicated than that, and the CAT manual isn't clear on this. That's why the program is tolerant on all modes received from the radio, and if more than one number encodes a mode, it shows a number next to the mode (see the map in `mode.cpp`), but UI allows setting only certain modes (see readMode() function in `ui.cpp`).

Please let me know if you can shed some light on this mode mess.

Recognized responses from the radio:

* RMnxxx: meter number n with value xxx (0 to 255). These raw, numeric values are then translated to meaningful numbers in `meters.cpp`, and this translation is probably only correct for FT-891

* FAnnnnnnnnn: current VFO A frequency

* MD0nn: current operating mode

## Meter reading and timeouts

Once every 100 ms, program reads IDD meter and determines what to do next.

If IDD is 0, then it assumes we're in RX mode. Signal (S-meter) is read, followed by frequency and band, and this information is presented on the status bar.

If IDD is not 0, then it assumes the radio is transmitting. Other meters (ALC, compressor, output power, SWR) are read and presented on the status bar. Frequency and mode are **not** read, as it's assumed that they can't (or at least shouldn't) change when TXing. It means that if the program is started when the radio is transmitting, frequency and mode information won't be available (it's a program limitation, but I think an insignificant one – I can't find any use case for this).

Once a CAT command that warrants a response is issued, the radio has two seconds to deliver that response. If it fails, then the program will terminate with the CAT timeout error.

## udev

If you want your FT-891 ports to be available as /dev/ttyFTCAT (CAT port) and /dev/ttyFTPTT (PTT port), create a file in `/etc/udev/rules.d/` directory (for example `80-tty.rules`) with the following content:

```
# FT-891 CAT port
ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea70", ENV{ID_USB_INTERFACE_NUM}=="00", SUBSYSTEM=="tty", SYMLINK+="ttyFTCAT"

# FT-891 PTT port
ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea70", ENV{ID_USB_INTERFACE_NUM}=="01", SUBSYSTEM=="tty", SYMLINK+="ttyFTPTT"
```

## TODO and limitations

Lots of things, because the program is in the very early phase.

### High priority

* Decoding of meters other than S-meter doesn't really work – IDD, SWR and ALC values aren't shown correctly
* Map compressor levels to meaningful values
* Implement CW cut numbers

### Tuning

* Implement band limits per mode (so it's impossible to tune into CW part when doing SSB, etc.)
* Allow rolling back of the frequency within band limits per mode
* Allow entering frequency from the keyboard
* Prevent frequency changes when sending
* Check if tuning steps for CW are reasonable, maybe different steps should be done for CW and for SSB?
* Consider removing the 1 Hz part from the frequency when tuning (maybe as an option) – so if the frequency is, say, 14123.456 kHz, and left arrow is pressed, it's now changed to 14123.446 kHz, but maybe should be changed to 14123.440 kHz
* Add frequency presets, or at least one default frequency (set from CLI)

### Logging

* Allow editing exchange in the UI
* Allow logging into other formats than Cabrillo (ADI, maybe also my tlog, when it's finally published)
* Log editing (at least last entry)
* Info about number of QSOs in the log file
* In 'c' mode, show all QSO lines
* Option to check current frequency in log too (if it's already in log)
* Wildcards when checking if the callsign is in the log
* Command to decrement exchange
* Handle RY (RTTY) mode in CBR – maybe add a CLI option to override mode in CBR? Right now data modes are logged as DG
* AM mode is unsupported in the logger (CBR doesn't support it), but supported by the program – think how best to solve this
* Allow having a file with known callsigns (for example callsigns used in previous contests) for easier callsign misspelling detection

### CW

* Add preset sets (for example, different sets for fox and for hound)
* Rethink the presets subsystem (it's not OK that they're hardcoded)
* When sending text, show information about characters that couldn't be mapped to Morse
* If USB disconnects due to RFI when sending, a radio will be stuck in the keyed mode. Not much can be done about this, maybe just document it better

### UI and meters

* Rework text input subroutine, now it's very limited and looks like reinventing the wheel – can readline be used in an asynchronous way? How it's done in irssi?
* Allow remapping of keys (now they're hardcoded)
* Add TX statistics when switching to RX (max power, max SWR, etc.)
* Some peak mode for meters
* First refresh erases the screen – why?
* UI is now too verbose – don't print frequency changes, don't print certain keys (or print no keys at all), declutter the interface
* If wrong key is entered in 'm' and 'b' modes, newline is not printed before printing an error
* Maybe it would be better to abort 'm' and 'b' commands by pressing Enter instead of backspace
* Handle SIGWINCH – now the screen would probably get messed up if window size was changed

### Radio control and CAT

* Think about moving to hamlib (if there's enough demand)
* Make fake CAT implementation for testing the program
* CAT interval and timeout might be configurable
* Investigate if polling is really needed – does the AI (Auto Information) mode send all things we use?
* Handle built-in RTTY mode (I use DATA for that, but maybe someone really uses RTTY?)
* Optionally disable meters (might be useful with other radios)
* What is the real difference between all these modes (for example SSB 1 and SSB 2)? Figure it out somehow
* Read the radio ID at the beginning and refuse operation for radios other than FT-891 (but add a CLI switch to override it)
* Check how CAT handling behaves when NUL (0x00) is received
* Add option to change output power (all power settings) from the UI
* The way CAT is currently implemented, gotEvent() function might trip an exception sometimes (although it never happened to me). If you receive „Got CAT event ..., but not expecting any” or „Got CAT event ..., but expecting ... first” errors, make sure to contact me – it might need to be redesigned
* Add an option to log CAT traffic for debugging

### Rest

* Optionally log notes and UI interactions to a file
* Allow sending and logging other report than 59 / 599
* Right now only 59 and 599 reports are supported – what about reports in certain data modes (like RSV in SSTV)?
* Some smarter handling of a remote callsign (now it has to be reentered in logging and checking, and can't be used in presets)
* select() timeout is not recalculated in util::watch() function if EAGAIN or EINTR are received. In current implementation it doesn't matter, because we don't rely on this timeout (timeouts are implemented separately), but it's not the best way to code things. Maybe remove support for timeout in this function altogether
* Sort this TODO list, now it's very chaotic
* Add some license

## License

I'm a programmer, not a lawyer, so it will be simple.

If you want to use this program, do it. Just don't expect anything from me and don't hold me accountable for anything. Think before you do anything – remember that you can kill your radio if you transmit into an unmatched load, you can kill your appliances if you have RF in your shack, you can burn yourself or others with RF energy, introduce interference, etc. You're licensed, so you know what you're doing, and if you're not licensed, then don't use this program with a real radio.

If you want to use parts of this program in your own non-commercial project (open- or closed-source), you're also free to do it. You don't have to list me anywhere (although it would be nice if you provided a link to this program, if you think your users would benefit from it). Sharing is caring.

If you want to use parts of this program in a commercial product, please contact me first.

In general, if you have any doubts, just contact me and we'll work something out.

## Contact with the author

Please use the GitHub issue reporting system, or contact me at circuitchaos (at) interia (dot) com.
