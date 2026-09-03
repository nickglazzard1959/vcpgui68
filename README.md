# VCPGUI - Virtual Control Panel Graphical User Interface

A somewhat special purpose GUI for making "virtual control panels"
for electronic test and measurement devices, and some application
programs that use it.

Some "less travelled" paths in technology are taken.

Here is an example of a "virtual control panel" created using this
software:

---

![Function Generator Virtual Control Panel](images/afg-dm3k6.png)

---

This project started with a desire to provide a user interface for a home
made function generator (an electronic instrument that can output sine,
triangle or square waves at a precise frequency and fairly precise
amplitude). This thing is controlled internally by an Arduino Uno and "understands"
a small set of commands sent over a "serial" line (physically, USB). It
has no control panel and can only be controlled externally by sending it commands
from a computer. I have been using Python to do this for over 8 years
now, but never got around to creating any kind of GUI for it.

I have other, similar, projects as well as a small set of commercial
instruments that can be controlled using USB TMC (Test and Measurement
Class) with SCPI (Standard Commands for Programmable Instruments)
commands. It would be nice if all of these could be controlled by a GUI
running on a laptop or other computer.

The "obvious" way to do this is to continue to use Python, which has
several packages that "talk" USB TMC directly or via VISA (Virtual
Instrument Software Architecture) and a library for serial communications.
It also has several choices for constructing a GUI (such as TkInter and
Qt via PySide or PyQt).

However, there were a few "reasons" to not take the "obvious" route. 
Perhaps not very good "reasons" (or proper "reasons" at all, arguably),
but here they are:

- Before retiring, I had used Python and PyQt *a lot*. Using something else
  was appealing, just for a change.
- I wanted a "GUI" that looked more like physical control panels than
  could easily be achieved with the usual GUI toolkits. 
- I had some quite decent pictures of Nixie tubes showing all the decimal
  digits (taken from my old HP 3430 voltmeter with a Fujifilm S7000, both
  now gone to other owners, regrettably). I wanted to use these for "numeric
  displays".
- Back in the 'noughties, I had come across a marvellous piece of software
  called "Algol 68 Genie", an interpreter / compiler written by Marcel van
  der Veer. As you would expect, this lets you write software in the "old"
  Algol 68 language on modern platforms. I wanted to use this for something
  ever since I discovered it. A bit more on this below.
- GUI toolkits tend to be large and complex (in the case of Qt, huge). How
  small and simple can a GUI be while still being useful, even if only for
  a limited set of applications? (There are some lightweight GUIs out there,
  such an "Dear ImGui" and "GuiLite", but I'm not familar with these as yet).
  It might be interesting to see how little code we can get away with ...

So, the plan became:

- Keep as much of the GUI logic in Algol 68 running on Algol 68 Genie as
  possible. As A68G is an interpreter (as well as a compiler) this might
  give many of the same "rapid application development" benefits that come
  from using Python.
- Write all application programs (that use the GUI) in Algol 68 (for A68G).
- Not all application programs need to use the GUI. Being able to "script"
  the activities of test and measurement equipment is actually essential. Using
  an interpreted language is very advantageous for this sort of work, and A68G
  provides that.
- Draw GUI components, primarily using texture images stored in a texture atlas,
  using SDL (Simple DirectMedia Layer). SDL now has three somewhat incompatible
  major versions, with SDL 3 being the current version still under development.
  At present, though, we will use SDL 2. Using SDL for the "GUI backend" makes
  the thing quite portable, in principle, although I am primarily interested
  in targetting Linux and, to a lesser degree, macOS.
- A68G has no Foreign Function Interface (FFI) as such. Short of adding new
  code into the A68 interpreter/compiler itself, I don't think there is any
  way of calling external libraries written in, say, C or C++. The FFI capabilities
  of Python are one of its greatest advantages. However, there is a workaround
  which works perfectly well for this project's purposes: A68G can launch another
  process and communicate with it using pipes. We can therefore write "helper
  programs" in C++ that are linked with SDL 2, or libusb or can access the 
  operating system's serial i/o capabilities. By writing to and reading from the
  pipes connected to the "helper programs", we can (indirectly) access essentially
  any C, C++ or Fortran library code we may need. Admittedly, this is not a very
  high performance solution, but it turns out to be quite fast enough for this
  project.

There is another "reason" for using Algol 68 for this work. Some 46 years ago (1980),
I wrote what may be the worst Algol 68 program ever written (to analyse data for
a final year undergraduate experimental physics project). It worked (eventually), but
it was terrible. Maybe not so surprising, as the only code I had previously written
was in Dartmouth BASIC (quite a big step up from that to A68!). I wanted to see what
the language was really like now I have a lot more experience.

Algol 68 Genie has proved to be robust, fast and capable. I have encountered zero
problems with it. None. The Algol 68 language's "last stable release" was 1973 -- 53
years ago, so it naturally has some omissions when compared to today's popular
languages. But it was *far* ahead of its time and feels surprisingly "modern". The
main limitation is that it is a purely imperative, procedural language. Most
"popular" languages these days are "multi-paradigm", mixing together procedural, 
object oriented, functional and other features for greater "generality". It perhaps
could benefit from some way of better controlling symbol visibility (e.g. "module" or
"namespace" features). It would be good to have more polymorphism (operators can
be polymorphic, but not procedures). Apart from that, to my mind, it is pretty 
much perfect as a procedural language: elegant, expressive and concise. 
This is, of course, a purely subjective opinion!

## Component parts

As an interpreter, Algol 68 Genie does not make use of object libraries. Instead, we
can reuse functionality through "preludes" -- source code that is "included" and
intepreted along with the application ("particular-program") code. This is the same
approach as "header-only libraries" in C++. 

The VCPGUI project has a number of these "preludes" that are included as needed in
the application programs. These fall into a few categories.

### VCPGUI itself.
The VCPGUI "API" is defined in the file: `vcpgui.a68`. The procedures in there allow
a user to define the GUI components, "callback" functions that respond to interactions
with those components, and lay out those components in a window, thus creating the
"virtual control panel". 

The overall logic follows the usual pattern: declare the UI components, then call a
function which instantiates the defined components, then run an event loop which
fields all user interactions with the GUI and calls any associated callback functions.

Note that one omission is there are no "layout automation" features. There are functions
which will calculate the sizes of declared componnents and generate coordinates to
distribute or align components relative to others, but the user must specify coordinates
for each component (actually the centre positions).

### General purpose preludes

There are a number of "preludes" that are likely to be useful in many application
programs. Specifically:

- `utility.a68` provides a wide variety of frequently useful functions (A68G has quite
  a lot of "batteries included" functions, but this adds quite a few more). Specifically:
    - Some time related routines, such as simple arithmetic in hours, minutes and seconds.
    - Routines for clamping numbers to ranges and min and max functions.
    - Many routines for trimming white space from strings, classifying strings and so on.
    - Many routines for converting strings to INTs and REALs and vice versa.
    - Functions for opening files and character devices in familiar ways.
    - Functions for splitting filenames and paths (getting extensions, base name, etc.)
    - Functions for reading and writing 1D and 2D arrays (row and row,row) of REALs to/from files.
    - Some other simple array functions.
    - Some aids to handling errors.
- `dictionaries.a68` provides "dictionaries" (also known as "maps" or "associative arrays") for
  string keys and string or integer values. It also provides lists of strings.
- `arg_parser.a68` provides an easy to use way of accessing command line arguments, similar to
  the functionality provided by the `argparse` module in Python or `CLI11` for C++11 (and later).
- `config_parser.a68` provides for parsing configuration files (using a syntax similar to, but
  not entirely compatible with, TOML). This supports string keys, with string, multi-line string,
  boolean, integer, real and 1D array of real values, arranged in groups or "tables". It is possible
  to define key-value pairs in tables without parsing a configuration file, giving another way of
  implementing "dictionaries".

### Non-specific instrument control preludes

These provide ways of communicating with test and measurement instruments that are generally
applicable and not specific ro a given instrument.

- `serialio.a68` provides serial i/o functions to communicate with appropriate instruments
  from Algol 68 Genie.
- `tmcusbio.a68` provides functions to communicate with appropriate instruments using USB TMC
  from Algol 68 Genie.

### Instrument specific preludes

These provide functions to control specific instruments. They can be used from programs to
script what one or more instruments do, allowing automatic measurement sequences to be 
performed. This can be very powerful, and is often more useful that having a nice looking
GUI, to be honest.

- `angelfg.a68` provides instrument specific functions for controlling the home made
  function generator. Since this is a "one-off" instrument, it isn't really useful to
  anyone other than me.
- `rigol3kdmm.a68` provides functions for controlling Rigol DM3000 series digital
  multimeters.

I intend to add other "instrument specific" preludes for the other instruments I have
(eventually).

### Virtual control panel application programs

These use VCPGUI and instrument specific preludes to implement virtual control
panels for specific instruments. At present, we have:

- `afgvcp.a68` provides a virtual control panel for the home made function
  generator. 
- `rigoldm3kvcp.a68` provides a virtual control panel for Rigol DM3000 series
  digital multimeters. Most (but not all) functions of the multimeter can be
  controlled.

### A generally usable demonstration program.

One problem with this project is that much of it is not directly usable if
you don't have one of the supported instruments. One of these being a unique
"one-off" makes this even worse!

So a demonstration program is supplied which can be used without any physical
instrument being needed. This is a simple clock program: `clockvcp.a68`.

