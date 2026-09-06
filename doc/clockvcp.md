# A VCPGUI demonstration program: clockvcp

---

![The clockvcp program](../images/clockvcp.png "The clockvcp program")

---

## Starting clockvcp from the command line

```
$ a68g clockvcp.a68 -- --help

Simple Clock. V0.1
==================
Running under Algol 68 Genie 3.12.0

Simple Clock Program
--------------------

This is a simple clock display program that shows some of the features
of the VCPGUI project. It can display the local time, UTC time, or the time
at one of a three cities. These can be defined in `clock-cities.ini`, along
with UTC time offsets. No attempt is made to use system timezone and DST data
as that would take too much effort from Algol 68!

-o, --ontop, (flag) : Keep DSURF window on top.
-n, --noborder, (flag) : Turn off window border for DSURF.
-a, --audiofile, (option) : Specify the name of the audio file for button clicks.
-h, --help, (flag) : Display help and exit.
```

## Using the clockvcp program

Perhaps the thing is pretty self explanatory ... except the city locations
shown as labels on the left hand radio button.

The `Local` button will show the local time at your location (hopefully).
The `UTC` button shows the Universal Time Coordinated time (once known as
Greenwich Mean Time). 

The "time" is taken from the local system clock. If this is synchronised with
reference clocks with NTP, this should be accurate.

The other three button labels and what they show are determined by the contents
of a configuration file named `clock-cities.ini`. Here is the default version
of that:

```
immediate-update = false

[cities]
Paris = 2
Tokyo = 9
Montreal = -4
```

The `[cities]` table contains city names and an offset from UTC that is
relevant to that city. Ideally, these would be determined from timezone
information, but, to be honest, that seems a bit too difficult to access
from Algol 68 at present!  As a result, DST (Daylight Savings Time), where
relevant, is also not accounted for. Well, maybe in Version 2 ...

The main point of this little program is a minimal demonstration of how
to use VCPGUI. Hopefully, examining the `clockvcp.a68` program will be useful 
to anyone interested in using VCPGUI (in the unlikely event anyone is).

Unlike the other VCPGUI programs, it should run on any Linux (and perhaps macOS)
system, given the VCPGUI installation has been completed successfully.

 