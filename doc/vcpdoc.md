---
title: "VCPGUI Preludes"
author: "Mindoc68"
date: "2026-9-2"
header-includes:
 - |
   \usepackage{a4wide}
   \usepackage{tocloft}
   \setlength{\cftsecnumwidth}{2.0em}
   \setlength{\cftsubsecnumwidth}{2.5em}
   \setlength{\cftsubsubsecnumwidth}{3.0em}
   \usepackage[scaled]{helvet}
   \renewcommand*\familydefault{\sfdefault}
---

# Introduction

This is documentation automatically extracted from the source code for
a number of "header only libraries" (in C++ terms) or "preludes" as
Algol-68 terms them.

Some of these are generally useful -- for example, for parsing command
line arguments, configuration files and a wide variety of "utility"
functions, sometimes vaguely modelled on Python library functions.

There is also the "library" that supplies the Virtual Control Panel
"GUI" functionality: `vcpgui.a68`.

The remaining "libraries" are specific to communicating with test
and measurement instruments, either broadly applicable (`serialio.a68`
and `tmcusbio.a68`) or only applicable to a particular instrument
(`angelfg.a68` and `rigol3kdmm.a68`).

# `angelfg.a68`
Routines for controlling an ANGEL X3325X DDS function generator over serial.

Allow all features of this homemade function generator to be controlled programmatically.
Since the device has no physical control panel, this is really the only way to use it!

When the device is opened, it initialises, after which it sends a single * character.
Commands are short strings (1 character, in fact) followed by a numeric argument terminated by
a . or a string argument terminated by ]. Each command gets a 1 character response: * if OK
or ? if the command was not understood.


---

```
PROC open_afg = (INT dev_no, BOOL fake) REF AFG: 

```
Open a serial connection to the function generator. This will be on /dev/ttyACM<dev_no>.
For debug and demo purposes, this can operate *without* an AFG attached if fake is TRUE.

---

```
OP AFG_INVALID = (REF AFG afg) BOOL: 

```
Check if the device was opened correctly.

---

```
PROC close_afg = (REF AFG afg) VOID: 

```
Close the connection to the AFG.

---

```
PROC send_cmd_afg = (REF AFG afg, STRING cmd) BOOL: 

```
Send a command and check for the expected response.

---

```
PROC set_waveform_afg = (REF AFG afg, STRING waveform) BOOL: 

```
Set the waveform to sine, triangle or square.

---

```
PROC set_frequency_afg = (REF AFG afg, REAL frequency) BOOL: 

```
Set the frequency to output.

---

```
PROC set_range_afg = (REF AFG afg, INT range) BOOL: 

```
Set the range to output. 0=0:5V. 1=0:500mV, 2=0:50mV

---

```
PROC set_atten_afg = (REF AFG afg, INT atten) BOOL: 

```
Set the output attenuation. 0=min output. 255=max output.

---

```
PROC set_lower_gain = (REF AFG afg, BOOL on) BOOL: 

```
Set lower gain mode in output amplifier. This sort of doubles the available output levels.

---

```
PROC set_rms_volts_afg = (REF AFG afg, REAL volts) BOOL: 

```
Set the output level to a specified RMS voltage.

---

```
PROC set_pp_volts_afg = (REF AFG afg, REAL volts) BOOL: 

```
Set the output level to a specified p-p voltage.

---

```
PROC set_volts_afg = (REF AFG afg, REAL volts, STRING variant) BOOL: 

```
Set the output level according to one of definitions.

---

```
PROC format_volts_string = (REF AFG afg, REAL volts, INT choice) VOID: 

```
Format the volts string for display on the LCD and send the string.

---

## Statistics
PROC declarations: 12\
  OP declarations: 1\
      Total lines: 264\
    Lines of code: 185\

# `arg_parser.a68`
A command line argument parser.

This is go at a command line argument parser for Algol 68
along the lines of Python's argparse or the CLI11 header only library
for C++. It isn't "feature rich", but it does the essentials quite
cleanly, IMO. (CLI11 is nice to use ... but why does it need over
400K of source?!).

An argument parser "instance" is created with make_arg_parser().

Options are added with arg_add_MODE_option() which defines an option
(which always has a value immediately following it), sets its default
value and returns a variable of mode MODE set to the default. Allowed
MODEs are currently string, int and real.

Flags (which control boolean values by being present or not) are
added with arg_add_flag(). Each time they are used, they toggle the default.

The parse_args() routine sets the variables to values specified on
the command line, as well as detecting errors and outputting help.
The variables can then be used to control the behaviour of the program.

Any number of positional arguments can also be supplied. These must be
treated as strings, of course. The number of supplied positional arguments
can be retrieved with arg_get_n_positionals() (after parse_args() has run),
and each postional arg can be retrieved by its positional index with
arg_get_positional_n(). You can add descriptions for positional arguments
which will appear in help output. If you do add descriptions, the number
of supplied positionals must match the number of descriptions.

See the TESTS section at the bottom of the file for a usage example.

Some test cases, for example:
```  
a68g arg_parser.a68                          # OK # 
a68g arg_parser.a68 -- -h                    # OK # 
a68g arg_parser.a68 -- -f -t Yikes           # OK # 
a68g arg_parser.a68 -- -f -t Yikes hello     # OK # 
a68g arg_parser.a68 -- -f -t                 # Fail, missing argument. 
...# 
a68g arg_parser.a68 -- -f -t -s gg           # Fail, string argument (for 
...-t) starts with -. # 
a68g arg_parser.a68 -- -f -t Yikes hello -i 67  # OK (hello is a positional 
...argument)  # 
a68g arg_parser.a68 -- -f -t Yikes hello -i 67s # Fail, malformed integer 
...67s. # 
a68g arg_parser.a68 -- -f -t Yikes hello -i -67 # OK. Integers can be negative. 
...# 
a68g arg_parser.a68 -- -f -t Yikes hello -i 78 *.a68 # OK. Lots of positional 
...arguments. # 
a68g arg_parser.a68 -- -f -t Yikes "Hello Kitty!" -i -78 *.a68 # OK. Shell 
...handles quoted strings for us. # 
a68g arg_parser.a68 -- --help                # OK, long form options work. 
...# 
a68g arg_parser.a68 -- -f -t Yikes "Hello Kitty!" -i -78 "*.a68" # OK. 
...# 
a68g arg_parser.a68 -- "Hello Kitty!"        # OK, positionals only allowed. 
...# 
a68g arg_parser.a68 -- -f -t Yikes "Hello Kitty!" -i +78 *.a68 # OK. Signed 
...positive integers allowed. # 
a68g arg_parser.a68 -- -f -t Yikes "Hello Kitty!" -i -78 "*.a68" -r -1.5e7 
... # OK. All real number fmts should work. # 
```


---

```
PROC make_arg_parser = (STRING app_desc) REF ARG_PARSE: 

```
Make an argument parser.

---

```
PROC arg_set_description = (REF ARG_PARSE arg_parser, STRING long_desc) 
VOID: 

```
Add a longer description of the program.

---

```
PROC arg_add_any_option = (REF ARG_PARSE arg_parser, REF ARG_OPTION new) 
VOID: 

```
Internal functions. These just extend FLEX arrays.
Add non-positional argument (option or flag).

---

```
PROC arg_add_positional = (REF ARG_PARSE arg_parser, STRING posarg) VOID: 

```
Add a positional argument.

---

```
PROC arg_add_positional_description = (REF ARG_PARSE arg_parser, STRING 
posarg_desc) VOID: 

```
Add a positional argument description.

---

```
PROC arg_add_int_option = (REF ARG_PARSE arg_parser, STRING short, STRING 
long, STRING desc, INT uvalue) REF INT: 

```
Add an option for an integer argument. E.g. -i, --intopt, "An integer option", 999

---

```
PROC arg_add_string_option = (REF ARG_PARSE arg_parser, STRING short, STRING 
long, STRING desc, STRING uvalue) REF STRING: 

```
Add an option with a string argument. E.g. -s, --stringopt, "A string option", "Default"

---

```
PROC arg_add_real_option = (REF ARG_PARSE arg_parser, STRING short, STRING 
long, STRING desc, REAL uvalue) REF REAL: 

```
Add an option for a real argument. E.g. -r, --realopt, "A real option", 3.14159

---

```
PROC arg_add_flag = (REF ARG_PARSE arg_parser, STRING short, STRING long, 
STRING desc, BOOL uvalue) REF BOOL: 

```
Add a flag, which has no argument.Sets a BOOL variable. E.g. -f, --flag, "A flag", TRUE

---

```
PROC arg_get_n_positionals = (REF ARG_PARSE arg_parser) INT: 

```
Get the number of supplied positional arguments.

---

```
PROC arg_get_positional_n = (REF ARG_PARSE arg_parser, INT n) STRING: 

```
Get the n-th positional argument. If the index is invalid, return empty string.

---

```
PROC parse_args = (REF ARG_PARSE arg_parser) BOOL: 

```
Parse user supplied command line args and update the variables they are associated with.

---

## Statistics
PROC declarations: 12\
  OP declarations: 0\
      Total lines: 367\
    Lines of code: 245\

# `config_parser.a68`
Read a config file, parse it and allow its definitions to be accessed as variables

This parses something like a sub-set of TOML, but compatibility wasn't a goal.
A full TOML parser should cope with the things this parser can deal with, though.

Example of usage:
```  
REF CONFIG_PARSE cp := make_config_parser("test.ini"); 
IF NOT parse_config_file(cp) 
THEN  
put(stand error, ("Failed to parse config file. Giving up.", newline)); 
...stop  
FI;  
 
# Basic string # 
STRING again := config_get_string_value(cp, "again", "nuts"); 
# Type error here, a is an INT in test.ini # 
STRING a := config_get_string_value(cp, "a", "nuts"); 
# Integer # 
INT anint := config_get_int_value(cp, "anint", -1234); 
# Float/Real # 
REAL afloat := config_get_real_value(cp, "afloat", 2.718); 
# Multi-line string # 
STRING ml := config_get_string_value(cp, "tablea1.ml", "coffee"); 
# 1D array of REALs # 
FLEX [1:0] REAL ra := config_get_real_array(cp, "array", LOC [2]REAL := 
...(-1,-2)); 
```

Beyond the straightforward use of getting settings from a configuration file, the key-value pairs can
be grouped into 'tables' and those tables can be used pretty much as separate 'dictionaries' or
'associative arrays' with string keys and string (plain, literal or multi-line), integer, boolean, real
or array-of-real values. Key-value pairs and tables can be added to a CONFIG_PARSE object without
referencing a file at all, which may sometimes be useful.


---

```
PROC make_config_parser = (STRING config_file_name) REF CONFIG_PARSE: 

```
Make a config file parser.

---

```
PROC config_add_key_value = (REF CONFIG_PARSE config_parser, STRING table, 
STRING key, STRING value) VOID: 

```
Add a key-value to the string dictionary. The value string must use the conventions for config files.

---

```
PROC config_delete_key_value = (REF CONFIG_PARSE config_parser, STRING 
table, STRING key) VOID: 

```
Delete a key-value from the string dictionary.

---

```
PROC string_escapes_to_chars = (STRING s) STRING: 

```
Convert escape sequences to characters.

---

```
PROC chars_to_string_escapes = (STRING s) STRING: 

```
Convert special characters to escape sequences.

---

```
PROC parse_config_file = (REF CONFIG_PARSE config_parser) BOOL: 

```
Read a config file and convert it to entries in a string dictionary.

---

```
PROC config_display_type = (STRING abbreviated) STRING: 

```
Decode the leading type character of a value stored in the string dictionary for output.

---

```
PROC config_undefined_key_msg = (STRING key) VOID: 

```
Internal undefined key message helper.

---

```
PROC config_wrong_value_type_msg = (STRING key, STRING wanted_type, STRING 
actual_type_1) VOID: 

```
Internal wrong value message helper.

---

```
PROC config_get_string_value = (REF CONFIG_PARSE cp, STRING key, STRING 
default) STRING: 

```
Get a string value from the parsed config file.

---

```
PROC config_get_int_value = (REF CONFIG_PARSE cp, STRING key, INT default) 
INT: 

```
Get an integer value from the parsed config file.

---

```
PROC config_get_bool_value = (REF CONFIG_PARSE cp, STRING key, BOOL default) 
BOOL: 

```
Get a boolean value from the parsed config file.

---

```
PROC config_get_real_value = (REF CONFIG_PARSE cp, STRING key, REAL default) 
REAL: 

```
Get a real value from the parsed config file.

---

```
PROC config_get_real_array = (REF CONFIG_PARSE cp, STRING key, []REAL default) 
[]REAL: 

```
Get an array of real values from the parsed config file.

---

```
PROC config_dump_dictionary = (REF CONFIG_PARSE cp) VOID: 

```
Debug aid: dump contents of the config parser dictionary.

---

```
PROC config_get_table_names = (REF CONFIG_PARSE cp) []STRING: 

```
Return an array of table names.

---

```
PROC config_has_table = (REF CONFIG_PARSE cp, STRING table_name) BOOL: 

```
Return TRUE if the config parser has a specified table.

---

```
PROC config_get_keys_in_table = (REF CONFIG_PARSE cp, STRING table_name, 
BOOL strip_table) []STRING: 

```
Get all keys in a table.

---

```
PROC config_get_value_type = (REF CONFIG_PARSE cp, STRING key) STRING: 

```
Get the type of a value. Return one of: '' (bad key), S, L, M, B, I, F, A.

---

```
PROC config_get_value_as_string = (REF CONFIG_PARSE cp, STRING key) STRING: 

```
Get a value of any type as a string.

---

```
PROC config_add_table = (REF CONFIG_PARSE cp, STRING table) VOID: 

```
Add a new table. If the table already exists, that is OK.
key-value pairs can then be added with config_add_key_value().

---

```
PROC config_display_contents = (REF CONFIG_PARSE cp, BOOL sort) VOID: 

```
Display all the contents of a CONFIG_PARSE object.

---

```
PROC write_config_file = (REF CONFIG_PARSE cp, STRING filename) BOOL: 

```
Write a config file from the current contents of a config parser object.
Note that this *intentionally* does not use the file name in the config parser object.

---

## Statistics
PROC declarations: 23\
  OP declarations: 0\
      Total lines: 797\
    Lines of code: 611\

# `dictionaries.a68`
Dictionaries (a.k.a maps, a.k.a associative arrays)

Because Algol 68 is strongly typed and doesn't have any meta-programming
features, it's hard to see how to avoid having specific code for each value
type. You could easily make a single dictionary type configurable by
setting a few modes and values, but you can only have one dictionary type
in a single program if you do that, AFAICS. You could try using UNIONs,
but I'm not sure that doesn't lead to painful complications.

Actually, a very simple macro processor could solve this problem ...

At present, here are STRING keys with STRING and INT values. It would be
easy (but repetitive) to add any other value types.

We can use a single access operator (//) and test for a "key existing" operator
(HASKEY) for *all* the dictionary "types", though, because we do have operator
overloading.

NOTE: The dictionary code began with the associative array example for Algol 68 at
Rosetta Code, which can be found here:

[https://rosettacode.org/wiki/Associative_array/Iteration#ALGOL_68](https://rosettacode.org/wiki/Associative_array/Iteration#ALGOL_68)

This file also contains implementations of some other basic data structures.
Currently lists of strings.


---

## Simplest single linked list of strings.

```
OP STRING_LIST_INIT = (REF STRING_LIST list) REF STRING_LIST: 

```
Initialise string list.

---

```
OP +:= = (REF STRING_LIST list, STRING elem) REF STRING_LIST: 

```
Append a string to a list. Overload PLUSAB. Cheap. O(1).

---

```
PRIO HASELEM = 1; OP HASELEM = (REF STRING_LIST list, STRING elem) BOOL: 

```
Test if list contains a given string. Expensive. O(n)

---

```
OP ELEMS = (REF STRING_LIST list) INT: 

```
Return length of list. Overload ELEMS.

---

```
PRIO GET = 1; OP GET = (REF STRING_LIST list, INT i) STRING: 

```
Get list value by "index". Expensive. Very expensive if used to get the whole list!

---

```
OP GET = (REF STRING_LIST_ELEMENT sle) REF STRING: 

```
Get a reference to a string list element value. This can be used to get and set that value.

---

```
PROC string_list_as_array = (REF STRING_LIST list) []STRING: 

```
Return the contents of the list as an array of strings.

---

```
PROC string_array_as_list = (REF []STRING array) REF STRING_LIST: 

```
Convert a string array to a string list.

---

```
PROC string_list_print = (REF STRING_LIST list) VOID: 

```
Print the contents of a string list.

---

```
PROC string_list_print_elements = (REF STRING_LIST list) VOID: 

```
Print each element of a string list.

---

```
OP FIRST = (REF STRING_LIST list) REF STRING_LIST_ELEMENT: 

```
Return a reference to the first element of a list. Return nil_sle if empty.

---

```
OP NEXT = (REF STRING_LIST list) REF STRING_LIST_ELEMENT: 

```
Move to the next element of a list and return a reference to it, or nil_sle at the end.

---

```
OP POP = (REF STRING_LIST list) STRING: 

```
Pop the tail element off the list. O(n).

---

```
OP CLEAR = (REF STRING_LIST list) INT: 

```
Empty a list. Return the number of elements that were in it before.

---

```
PRIO COUNT = 1; OP COUNT = (REF STRING_LIST list, STRING value) INT: 

```
Count how many times a value occurs in a string list.

---

```
PRIO INDEX = 1; OP INDEX = (REF STRING_LIST list, STRING value) INT: 

```
Find the index of the first occurrence of value in a string list. 0 if not there.

---

```
PRIO DEL = 1; OP DEL = (REF STRING_LIST list, INT index) STRING: 

```
Delete a list item, given the index of the item. Return value of list item.

---

```
PROC string_list_split_to_words = (REF STRING_LIST lines, REF STRING_LIST 
words) VOID: 

```
For a string list of lines, split each line to a set of words (separated by spaces).
This preserves blank lines by marking them with a specific 'marker word': <EMPTY>.

---

```
PROC string_list_join_to_lines = (REF STRING_LIST words, REF STRING_LIST 
lines, INT line_length) VOID: 

```
For a string list of words, join a set of words to a set of lines, breaking at line_length chars.
This preserves the original blank lines.

---

```
PROC string_lines_to_string_list = (STRING lines, REF STRING_LIST list) 
VOID: 

```
Split a string on newline characters into a string list.

---

## Dictionary hash functions.

```
OP DICT_HASH = (STRING key) INT: 

```
Hash function for strings. Used to hash keys.

---

## STRING value dictionaries.

```
OP STRING_DICT_INIT = (REF STRING_DICT dict) REF STRING_DICT: 

```
Initialise a dictionary by setting all its collision lists to NIL.

---

```
PRIO // = 1; OP // = (REF STRING_DICT dict, STRING key) REF STRING_DICT_VALUE: 

```
Return a reference to the value for key. This can be used to retrieve an existing key-value AND
to create a NEW key-value, the value then being set by ASSIGNING to the returned reference.
Note that this ALWAYS adds a new key if it isn't already defined.

---

```
OP // = (REF STRING_DICT dict, CHAR key) REF STRING_DICT_VALUE: 

```
If a single character key is provided, it must be cast to a STRING.

---

```
PRIO HASKEY = 1; OP HASKEY = (REF STRING_DICT dict, STRING key) BOOL: 

```
Check if a key exists in a dictionary.

---

```
OP HASKEY = (REF STRING_DICT dict, CHAR key) BOOL: 

```
Again, if a single character key is provided, it must be cast to a STRING.

---

```
PROC get_string_dict_keys_array = (REF STRING_DICT dict) []STRING: 

```
Get keys as a row of STRINGs.

---

```
PROC print_string_dict_keys = (REF STRING_DICT dict) VOID: 

```
Print a list of keys readably.

---

```
OP ELEMS = (REF STRING_DICT dict) INT: 

```
Return number of keys in dictionary. Overload ELEMS.

---

```
OP FIRST = (REF STRING_DICT dict) REF STRING_DICT_KEYVAL: 

```
Start traversal of dictionary contents. Returns "first" element. Can be NIL (nil element list).

---

```
OP NEXT = (REF STRING_DICT dict) REF STRING_DICT_KEYVAL: 

```
Find the "next" element of the dictionary, if any.  Return nil element list if no more.

---

```
OP DEL = (REF STRING_DICT dict, STRING key) BOOL: 

```
Delete the keyval with a given key from a string dictionary. Return TRUE if key found and deleted, else FALSE.

---

```
OP DEL = (REF STRING_DICT dict, CHAR key) BOOL: 

```
DEL when key is a single character.

---

## INT value dictionaries.

```
OP INT_DICT_INIT = (REF INT_DICT dict) REF INT_DICT: 

```
Initialise a dictionary by setting all its collision lists to NIL.

---

```
OP // = (REF INT_DICT dict, STRING key) REF INT_DICT_VALUE: 

```
Return a reference to the value for key. This can be used to retrieve an existing key-value AND
to create a NEW key-value, the value then being set by ASSIGNING to the returned reference.
Note that this ALWAYS adds a new key if it isn't already defined.

---

```
OP // = (REF INT_DICT dict, CHAR key) REF INT_DICT_VALUE: 

```
If a single character key is provided, it must be cast to a STRING.

---

```
OP HASKEY = (REF INT_DICT dict, STRING key) BOOL: 

```
Check if a key exists in a dictionary.

---

```
OP HASKEY = (REF INT_DICT dict, CHAR key) BOOL: 

```
Again, if a single character key is provided, it must be cast to a STRING.

---

```
PROC print_int_dict_keys = (REF INT_DICT dict) VOID: 

```
Print a list of keys readably.

---

```
OP ELEMS = (REF INT_DICT dict) INT: 

```
Return number of keys in dictionary. Overload ELEMS.

---

```
OP FIRST = (REF INT_DICT dict) REF INT_DICT_KEYVAL: 

```
Start traversal of dictionary contents. Returns "first" element. Can be NIL (nil element list).

---

```
OP NEXT = (REF INT_DICT dict) REF INT_DICT_KEYVAL: 

```
Find the "next" element of the dictionary, if any.  Return nil element list if no more.

---

```
OP DEL = (REF INT_DICT dict, STRING key) BOOL: 

```
Delete the keyval with a given key from an INT dictionary. Return TRUE if key found and deleted, else FALSE.

---

```
OP DEL = (REF INT_DICT dict, CHAR key) BOOL: 

```
DEL when key is a single character.

---

## Sundry related algorithms.

```
PROC permuted_integers = (INT n, INT seed) []INT: 

```
Find an array of integers [1:n] randomly permuted. Fisher-Yates algorithm.

---

## Statistics
PROC declarations: 11\
  OP declarations: 27\
      Total lines: 1064\
    Lines of code: 759\

# `rigol3kdmm.a68`
Routines for controlling a Rigol 3K series DMM over USBTMC.

A subset of commands (hopefully the most useful) for a Rigol DM3000
series multi-meter are implemented. This has been tested with a DM3058E on
Debian Linux.

This uses the Rigol command set (the multi-meters also support Agilent and
Fluke command sets - presumably as used by those brands). The Agilent set
seems very similar, so it might be easy to adapt this to work with Agilent
devices. The Fluke set seems quite different, though.



---

## Device open and close

```
PROC open_dm3k = (BOOL reset) REF DMKKK: 

```
Open a connection to a DM3K multi-meter. The vendorID and productId are internally fixed.
The device MUST be attached, and r/w permissions via UDEV rules MUST have been set for it
before calling this routine. Optionally, reset the device.

---

```
OP DEV_INVALID = (REF DMKKK dev3k) BOOL: 

```
Check if the device was opened correctly.

---

```
PROC close_dm3k = (REF DMKKK dev3k) VOID: 

```
Close the device.

---

## Function get and set

```
PROC func_to_i_func = (STRING func) INT: 

```
Identify func as an acceptable function name and return the function code for that.

---

```
PROC invalid_i_func = (REF DMKKK dev3k, STRING caller) VOID: 

```
Output informative message when the function code is invalid.

---

```
PROC get_function_dm3k = (REF DMKKK dev3k) BOOL: 

```
Get the current function from the device and set i_func in dev3k appropriately.
The device should return one of the following strings:
DCV, ACV, DCI, ACI, RESISTANCE, CAPACITANCE, CONTINUITY, FRESISTANCE,
DIODE, FREQUENCY, PERIOD, but there appear to be some variations with resistance.
Return TRUE if OK, else FALSE.

---

```
PROC set_function_dm3k = (REF DMKKK dev3k, STRING function) BOOL: 

```
Set what we what to measure.

---

## Range get and set

```
PROC get_range_dm3k = (REF DMKKK dev3k, BOOL override_auto, REF INT queried_code) 
BOOL: 

```
Get the range for the current function from the device and set i_range[i_func] in devk
to the value obtained. Only do this if i_func is NOT currently auto ranging unless override_auto.
Return the range code reported by the device in queried_code.
Return TRUE if OK, else FALSE.

---

```
PROC range_code_to_range_string = (REF DMKKK dev3k, INT range_code) STRING: 

```
Given a range code for the current function, convert it to a range string relevant to that function.

---

```
PROC range_code_to_range_string_func = (REF DMKKK dev3k, INT i_func, INT 
range_code) STRING: 

```
Given a range code for a specified function, convert it to a range string relevant to that function.

---

```
PROC range_code_to_resolution_string = (REF DMKKK dev3k, INT range_code) 
STRING: 

```
Given a range code for the current function, convert it to a resolution string relevant to that function.

---

```
PROC range_code_to_resolution_string_func = (REF DMKKK dev3k, INT i_func, 
INT range_code) STRING: 

```
Given a range code for the current function, convert it to a resolution string relevant to that function.

---

```
PROC get_range_code_of_current_function = (REF DMKKK dev3k) INT: 

```
Get the range code of the currently selected function.

---

```
PROC get_range_string_of_current_function = (REF DMKKK dev3k) STRING: 

```
Get the range string of the currently selected function.

---

```
PROC set_manual_range_dm3k = (REF DMKKK dev3k) BOOL: 

```
Set manual ranging for the current function and get the range for that function.

---

```
PROC set_auto_range_dm3k = (REF DMKKK dev3k) BOOL: 

```
Set auto-ranging mode for the current function.

---

```
PROC set_range_dm3k = (REF DMKKK dev3k, INT range) BOOL: 

```
Set the range for the current function to a specified range code (function dependent).

---

```
PROC inc_dec_range_dm3k = (REF DMKKK dev3k, BOOL inc) BOOL: 

```
Increment (inc=TRUE) or decrement the range code and set that range. Clamp at valid code limits.

---

```
PROC set_range_by_maximum_value_dm3k = (REF DMKKK dev3k, REAL value) BOOL: 

```
Set the range for the current function to one suitable for measuring up to value.

---

## Measure

```
PROC measure_dm3k = (REF DMKKK dev3k, REF REAL value, REF BOOL overload) 
BOOL: 

```
Make a measurement of the quantity defined by the current function.

---

## Statistics
PROC declarations: 19\
  OP declarations: 1\
      Total lines: 617\
    Lines of code: 372\

# `serialio.a68`
serialio.a68 - Routines for reading and writing serial lines via the serialio.cpp helper program.

There is no way to configure serial devices directly from a68g, but this is fairly easy to
work around using a helper program written in C++, then communicating with that helper program
using pipes.


---

```
PROC open_serial = (STRING devname, STRING config, REF PIPE serp) BOOL: 

```
Run serialio returning a pair of pipes connected to its stdin and stdout.
The devname arg is a device name, such as /dev/ttyACM0. The config arg may
be an empty string for the defaults, or a four part string of the form:
<baud>,<size>,<parity>,<stop>, such as 9600,8,N,1 (this is the default).

---

```
PROC write_serial_char = (PIPE p, CHAR outchar) VOID: 

```
Write a single character to serial.

---

```
PROC write_serial_line = (PIPE p, STRING outstring) VOID: 

```
Write string followed by a newline to serial.

---

```
PROC read_serial_char = (PIPE p) CHAR: 

```
Read a single character from serial.

---

```
PROC read_serial_line = (PIPE p) STRING: 

```
Read a line terminated by newline from serial.

---

## Statistics
PROC declarations: 5\
  OP declarations: 0\
      Total lines: 136\
    Lines of code: 89\

# `tmcusbio.a68`
tmcusbio.a68 - Perform basic i/o to a USBTMC device on Linux (for instrument control).

This communicates with a device using the USBTMC protocol via the tmcusbio.cpp helper
program.

Originally, this was handled by usbtmc.a68 via the kernel USBTMC driver in Linux.
This makes devices accessible as `/dev/usbtmc<N>`. Unfortunately, this did not work
perfectly reliably. It *could* perform thousands of queries without error, then fail,
or fail almost immediately. Even more unfortunately, upgrading from a 5.10 Linux kernel
to a 6.0 kernel made things significantly *worse*. Life is too short to try to work
out what the real cause of this is. Much less fix it. Note that the problems were
*not* related to using Algol 68 ... C++ test programs had exactly the same issues.

So, we are now trying a libusb based solution. Since libusb works almost entirely in user
space, working around defects (whatever their cause) is a lot more feasible than with the
kernel based USBTMC implementation.

As with the serialio.a68 code, we launch a C++ based helper program which uses libusb
to communicate with devices. We talk to that program with a `PIPE` -- the helper just reads
from stdin and writes to stdout.

___

Note: It turned out that the errors noted above were caused by USB hardware. They persist
in the libusb based version. The cure is to *power down* the host computer and restart it.
Powering down is essential. Restarting without power down is insufficient.

___


---

```
PROC open_usbtmc = (STRING vid, STRING pid, REF PIPE usbp) BOOL: 

```
Run tmcusbio returning a pair of pipes connected to its stdin and stdout.
The device is identified by a 'vendorId', 'productId' pair, supplied as
STRINGs here.

---

```
PROC close_usbtmc = (PIPE p) INT: 

```
Close the device. This terminates the helper program.

---

```
PROC put_usbtmc = (PIPE p, STRING s) INT: 

```
Send a string to a USBTMC device.

---

```
PROC query_usbtmc = (PIPE p, STRING s, REF STRING result) INT: 

```
Send a string to a USBTMC device and get a response string.
Return 0 unless there is an error. Distinguish between timeouts and other errors.
Return 2 for timeouts and 1 for all other errors.

---

```
PROC query_binary_usbtmc = (PIPE p, STRING s, REF INT retval) []INT: 

```
Send a string to a USBTMC device and get a binary response.
Return 0 if OK, 2 for timeouts and 1 for all other errors.

---

```
PROC set_timeout_usbtmc = (PIPE p, INT timeout) INT: 

```
Set the timeout period in milliseconds for USBTMC transfers.

---

## Statistics
PROC declarations: 6\
  OP declarations: 0\
      Total lines: 230\
    Lines of code: 151\

# `utility.a68`
Widely useful sundry routines, many to do with strings.

Includes:

- Some time related routines, such as simple arithmetic in hours, minutes and seconds.
- Routines for clamping numbers to ranges and min and max functions.
- Many routines for trimming white space from strings, classifying strings and so on.
- Many routines for converting strings to INTs and REALs and vice versa.
- Functions for opening files and character devices in familiar ways.
- Functions for splitting filenames and paths (getting extensions, base name, etc.)
- Functions for reading and writing 1D and 2D arrays (row and row,row) of REALs to/from files.
- Some other simple array functions.
- Some aids to handling errors.


---

## Sundry and OS related aids.

```
PROC pause = (STRING s) VOID: 

```
Pause for test purposes.

---

```
PROC sleep_void = (INT t) VOID: 

```
Sleep. The only approach seems to be to run sleep (1) command. t is in seconds.
ACTUALLY: this seems to be in the standard prelude, but isn't documented. So use that.
But void the result.

---

```
PROC run_command_get_output = (STRING cmd, REF STRING output) BOOL: 

```
Run a command in sh and retrieve the output as a string. Adapted from a68g docs.

---

```
PROC get_os_name = STRING: 

```
Return a string denoting the OS being used (debian, macos or linux for now).

---

```
PROC print_std_title = (STRING title) VOID: 

```
Print a program title in a standard format.

---

## Time arithmetic aids.

```
PROC print_hms = (STRING leader, HMSTIME hms) VOID: 

```
Print a decently formatted HMSTIME.

---

```
PROC hms_to_s = (HMSTIME hms) INT: 

```
Convert hours, minutes, seconds to seconds.

---

```
PROC s_to_hms = (INT s) HMSTIME: 

```
Convert seconds to  hours, minutes, seconds.

---

```
PROC get_time_of_day = (BOOL utc) HMSTIME: 

```
Get the time according to the system clock as an HMSTIME. Get local time or UTC.

---

```
PROC make_hms = (INT d, h, m, s) HMSTIME: 

```
Make an HMSTIME from its components. If any component is -ve, the thing is -ve.

---

```
PROC set_hms = (REF HMSTIME hms, INT d, h, m, s) VOID: 

```
Set the contents of an HMSTIME from components.

---

```
OP + = (HMSTIME a, HMSTIME b) HMSTIME: 

```
Define addition for HMSTIME values.

---

```
OP - = (HMSTIME a, HMSTIME b) HMSTIME: 

```
Define subtraction for HMSTIME values.

---

```
OP = = (HMSTIME a, HMSTIME b) BOOL: 

```
Are HMSTIMEs equal?

---

```
OP < = (HMSTIME a, HMSTIME b) BOOL: 

```
Is HMSTIME a less/earlier than b?

---

```
OP > = (HMSTIME a, HMSTIME b) BOOL: 

```
Are HMSTIME a greater/later than b?

---

## Numeric range etc. aids.

```
OP DEGRAD = (REAL a) REAL : 

```
Degrees to radians operator.

---

```
OP RADDEG = (REAL a) REAL: 

```
Radians to degrees operator.

---

```
PROC check_in_range_int = (INT v, lo, hi) BOOL: 

```
Check v is in the range lo to hi inclusive.

---

```
PROC clamp_int = (INT v, lo, hi) INT: 

```
Clamp integer values.

---

```
PROC clamp_real = (REAL v, lo, hi) REAL: 

```
Clamp real values.

---

```
PROC min2int = (INT a, b) INT: 

```
Min and max of pairs of integer values.

---

## String aids incl extensive string <--> numeric.

```
PROC wint = (INT i) STRING: 

```
Minimum width string from integer.

---

```
PROC doll = (STRING s) STRING: 

```
Return a dollar quoted version of a string.

---

```
PROC split_string = (STRING s, CHAR c) []STRING: 

```
Split a string on a character to a row of strings.

---

```
PROC string_in_row_of_strings = (STRING s, []STRING r) INT: 

```
Index of string in a row of strings. 0 if not found.

---

```
PROC trim_leading = (STRING s) STRING: 

```
Trim leading blanks from a string.

---

```
PROC trim_trailing = (STRING s) STRING: 

```
Trim trailing blanks from a string.

---

```
PROC trim = (STRING s) STRING: 

```
Trim leading and trailing blanks from a string.

---

```
PROC string_to_lower = (STRING s) STRING: 

```
Change the case of a string.

---

```
PROC despace = (STRING s) STRING: 

```
Remove space characters from a string.

---

```
PROC strip_blank = (STRING s) STRING: 

```
Remove all white space from a string. Same as despace() currently.

---

```
PROC strip_char = (STRING s, CHAR c) STRING: 

```
Remove c characters from a string.

---

```
PROC check_valid = (STRING test, STRING valid) BOOL: 

```
Check if a string is in a comma separated list of valid strings.

---

```
PROC string_to_choice_index = (STRING test, STRING choices) INT: 

```
Convert a string to a "choice" index by searching for it in a
comma separated list of choice strings.

---

```
PROC string_to_bool = (STRING s) BOOL: 

```
Convert a string to a BOOL. Accept true and false in any combination of case.

---

```
PROC all_upper = (STRING s) BOOL: 

```
Check string only contains upper case letters.

---

```
PROC all_lower = (STRING s) BOOL: 

```
Check string only contains lower case letters. Allow underscores too.

---

```
PROC all_digits = (STRING s) BOOL: 

```
Check string only contains decimal digits.

---

```
PROC all_digits_signed = (STRING s) BOOL: 

```
Check string only contains decimal digits after an optional sign.

---

```
PROC all_hexdigits = (STRING s) BOOL: 

```
Check string only contains hexadecimal digits.

---

```
PROC string_to_int = (STRING s) INT: 

```
String to integer. NOTE: I don't know how to use event routines with string transput ...
Actually, I think associate is the answer, but never mind.

---

```
PROC hex_char_to_int = (CHAR c) INT: 

```
Convert a hex digit to an INT. Return -1 for illegal characters.

---

```
PROC int_array_to_string = ([]INT array) STRING: 

```
Convert an array of INTs to a string. Aid to parsing binary data.

---

```
PROC ifromc = (STRING s, REF INT v, INT b, REF INT p) BOOL: 

```
Another string to integer routine. Start at s[b], return v, increment p.

---

```
PROC string_to_real = (STRING s, REF REAL v, BOOL use_all_s) BOOL: 

```
String to real. If use_all_s, return status will be FALSE unless s has no trailing non-float chars.
Otherwise, return status will be TRUE if s is a valid representation of a floating point number.

---

```
PROC si_string_to_real = (STRING s, REF REAL v, REF STRING units) BOOL: 

```
String to real, accounting for standard SI metric prefixes.
They should follow the number, maybe after a space.

---

```
PROC real_to_si_real = (REAL v, REF REAL v_out, REF STRING prefix, BOOL 
do_small) BOOL: 

```
Convert a real number to a scaled real number with an appropriate SI prefix.
If do_small is TRUE, use hecto, deca, deci and centi otherwise use only 10**3n prefixes.
This is quite hard to get right, and I'm not certain I have in all cases!

This variant uses u for micro prefix and is suitable for all output (command line, etc.).

---

```
PROC real_to_si_real_dsurf = (REAL v, REF REAL v_out, REF STRING prefix, 
BOOL do_small, BOOL do_mu) BOOL: 

```
This variant uses an above 7 bit code for micro. DSURF will map this to Unicode for Greek mu.

---

```
PROC real_to_si_real_internal = (REAL v, REF REAL v_out, REF STRING prefix, 
BOOL do_small, BOOL do_mu) BOOL: 

```
Do all the work of real_to_si_real() and real_to_si_real_dsurf().

---

```
PROC real_to_si_string = (REAL v, REF STRING s, INT width, after, BOOL 
do_small) BOOL: 

```
Convert a real number to a scaled real number with an appropriate SI prefix, as a string.
The scaled number before the prefix will have width characters, after after the decimal pt. E.g. 8, 3

---

```
PROC real_to_si_real_scaled = (REAL v, REF INT iv_out, INT n_digits, REF 
STRING prefix, REF INT dp, BOOL do_small) BOOL: 

```
Convert a real number to a scaled real number with an appropriate SI prefix, as a scaled integer.
The scaled integer will have n_digits and should be displayed with dp (returned) digits after the
decimal point. This is for use primarily with vcpgui's 'numeric display' component.

---

```
PROC real_to_si_real_scaled_dsurf = (REAL v, REF INT iv_out, INT n_digits, 
REF STRING prefix, REF INT dp, BOOL do_small) BOOL: 

```
Variant of real_to_si_real_scaled() for DSURF to display Greek mu for micro.

---

```
PROC real_to_si_real_scaled_internal = (REAL v, REF INT iv_out, INT n_digits, 
REF STRING prefix, REF INT dp, BOOL do_small, BOOL do_mu) BOOL: 

```
Handle most of both real_to_si_real_scaled() and real_to_si_real_scaled_dsurf().

---

```
PROC string_to_real_internal = (STRING s, REF REAL v, BOOL use_all_s, REF 
INT p, BOOL exp_ok) BOOL: 

```
Do the real work of string to real. This uses and returns (modified) start character index in p.
If exp_ok is FALSE, this will not attempt to process exponents (scientific notation). This is for
use with si_string_to_real(), where the prefix E (exa) may be read as an exponent.

---

```
PROC count_char_occurrences = (CHAR c, STRING s) INT: 

```
Count how many times a character occurs in a string.

---

```
PROC begins_with = (STRING s, STRING b) BOOL: 

```
Does string s begin with string b?

---

```
PROC ends_with = (STRING s, STRING b) BOOL: 

```
Does string s end with string b?

---

## File aids.

```
PROC open_file = (STRING filename, STRING mode, REF FILE fio) INT: 

```
Open a "regular" file. a68g open() only checks if a file exists when reading
at the time of the first i/o operation. This function behaves more like Unix fopen()/open().
Note that modes r and w do not allow seeking or binary i/o. Use r+ or w+ for those capabilities.
HOWEVER: binary does NOT mean 'raw bytes'. It is much more like FORTRAN UNFORMATTED.

---

```
PROC open_chardev = (STRING devname, STRING mode, REF FILE fio) INT: 

```
Open a character device.

---

```
PROC splitext = (STRING pathname) STRINGPAIR: 

```
Split a path name into a path and an extension.
Intended to work as per Python splitext.

---

```
PROC splitpath = (STRING pathname) STRINGPAIR: 

```
Split a pathname into a pair by cleaving off what is after the last / (if any).
Leave a trailing / on the first part of the split string.

---

```
PROC joinpair = (STRINGPAIR pair) STRING: 

```
Join a string pair into a string.

---

```
PROC print_string_pair = (STRINGPAIR p) VOID: 

```
Print a string pair.

---

```
PROC copy_file_contents = (STRING filename, REF FILE fout, BOOL trimspaces) 
BOOL: 

```
Copy the contents of an entire file to an open output file. Optionally trim white space.

---

## Array aids.

```
PROC write_1d_real_array = (STRING filename, []REAL array) BOOL: 

```
Write a 1D REAL array to a file. Return FALSE on error.

---

```
PROC read_1d_real_array = (STRING filename) []REAL: 

```
Read a 1D REAL array from a file. If this fails, return a vacuum.

---

```
OP ARRAYEMPTY = ([]REAL array) BOOL: 

```
Return TRUE if a 1D array is a vacuum.

---

```
OP ARRAYMININDEX = ([]REAL array) INT: 

```
Return index of minimum value in a 1D array.

---

```
OP ARRAYMIN = ([]REAL array) REAL: 

```
Return minimum value in a 1D array.

---

```
OP ARRAYMAXINDEX = ([]REAL array) INT: 

```
Return index of the maximum value in an array.

---

```
OP ARRAYMAX = ([]REAL array) REAL: 

```
Return the maximum value in an array.

---

```
PRIO ARRAYCLOSESTINDEX = 1; OP ARRAYCLOSESTINDEX = ([]REAL array, REAL 
value) INT: 

```
Return the index of the element of array that is closest to a given value.

---

```
PROC write_2d_real_array = (STRING filename, [,]REAL array) BOOL: 

```
Write a 2D REAL array to a file. Return FALSE on error.

---

```
PROC read_2d_real_array = (STRING filename) [,]REAL: 

```
Read a 2D REAL array from a file. If this fails, return a vacuum.

---

```
OP ARRAYEMPTY = ([,]REAL array) BOOL: 

```
Return TRUE if a 2D array is a vacuum.

---

## Error handling aids.

```
PROC print_os_error = (STRING leader, INT ierror) VOID: 

```
Print error message for ierror (as per errno) to stand error. Prepend with leader.

---

```
PROC ie = (INT status) VOID: 

```
If status /= 0, an internal error must have occurred. Abort.

---

```
PROC ieb = (BOOL status) VOID: 

```
If status FALSE, an internal error must have occurred. Abort.

---

## Statistics
PROC declarations: 65\
  OP declarations: 13\
      Total lines: 1574\
    Lines of code: 1129\

# `vcpgui.a68`
A somewhat specialised GUI for controlling some instruments.

This was developed because of a desire to control a homemade function
generator using a GUI ... but a GUI that looks like a traditional physical
control panel, at least to some extent.

Other motivations were that I had wanted to do something with Algol 68 Genie
since the nought-ies (not long after it was first released) and I had some
decent pictures of Nixie tube digits taken from my HP 3430A voltmeter with
a Fujifilm S7000 camera long ago. So ...

Another motivation was to see how much code was really needed for a viable
(albeit limited) GUI? Most GUI toolkits are huge (one exception may be GuiLite).
I have used Qt quite a bit and it is very capable ... but also very heavy.
Can something much lighter be useful? (The answer seems to be yes).

To have a GUI, some means of drawing graphical elements is obviously needed,
as well as a way of sensing mouse clicks (and other forms of input) in
relation to the drawn graphical elements. For appearance sake and flexibility,
the best way of drawing most elements is using texture mapped rectangles.
The basis for such a capability is SDL (Simple DirectMedia Layer). This provides
a high degree of portability between operating systems and windowing systems
(although the primary target for VCPGUI is Linux and no Windows support is
planned), At present, SDL 2 is being used.

SDL cannot be accessed directly from Algol 68 Genie and A68G also has no
FFI (foreign function interface), so it would not be easy to add such direct
access. One approach to using SDL is to write a 'helper program' in C++. This
program draws the graphical elements and fields interactions with them. It is
started from A68G as a subprocess that communicates with A68G via a PIPE.
Software written in Algol 68 can use writes and reads to/from the PIPE to draw
whatever it wants and to receive notification of 'interaction events'.

The C++14 based SDL display program is called DSURF (Display SURFace). It maintains
a 'display list' built by commands sent by Algol 68 functions. The display list
entries are commands to draw a small number of primitives supported by SDL. Commands
exist that remove display list entries too, so elements can be updated by removing
them then adding the updated versions of them. Primitives to remove are identified
by their position on the drawing surface (positions are mostly center positions
as this helps make this removal scheme work).

Creation and update of the GUI elements, the size of the DSURF window, and responding
to all interactions with those elements is done from Algol 68. All the program logic
lives there.

The lowest level drawing primitives are supplied by procedures with names
beginning with `ds_`. The basic drawing commands are:

- rectangles
- rounded rectangles
- filled rectangles
- filled rounded rectangles
- lines
- thick lines
- circles
- filled circles
- arcs
- ellipses
- filled ellipses
- basic text
- text drawn in a TrueType font
- texture mapped rectangles

These are mostly drawn by the SDL 2 graphics library. Unfortunately, except for
thin lines, this does not do anti-aliasing, so 'good' looking output mostly relies
on using textures.

To help with texture usage, DSURF makes use of a single texture image (dsurf.png)
and a texture atlas file (dsurf.tal) which identifies rectangular regions on the
texture image, each of which has a name which can be used to draw it at a specified
position on the display surface. The texture rectangles can also be rotated about
their centres. The texture image has an alpha channel, so irregularly shaped
primitives can be used rather than just rectangles.

Only one procedure is used to allow interaction to work: `ds_sense_rect` which
defines a rectangular area in which mouse clicks are fielded, resulting in messages
being sent from DSURF to the Algol 68 code over the PIPE. These *sense rects* are
usually aligned with drawn items, of course.

The following GUI components are currently defined:

- Push buttons
- Toggle buttons
- Radio buttons
- Knobs ('continuously' variable numeric controls).
- Selectors (rotary versions of radio buttons).
- Keypads (to enter numeric values).
- Numeric displays (to display general numeric values).
- Lamps
- Time displays (for HH:MM:SS displays).

Keypads and numeric displays can be linked to allow easy numeric value input.

Apart from the 'output only' components -- numeric and time displays and lamps --
procedures can be associated with components that are called when interactions
occur with those components (i.e. 'callbacks').

Layout of components on the display surface must be done manually. Routines are
provided to help with this, but there is no automatic layout available.

There are no facilities for inputting text from a keyboard (e.g. 'text widgets').
There are no 'file browser' or similar components. There is no scrolling of the
display surface window or any component.

Clearly, VCPGUI is hardly competition for Qt! On the other hand, what it does is
appropriate for a system intended to look like physical control panels and it is
quite sufficient for a wide range of device control applications. The VCPGUI code
itself consists of less than 2500 lines (< 1900 LOC) of Algol 68 and less than
1600 lines of C++, which is pretty lightweight. That doesn't include SDL, of course,
so it depends where you choose to draw lines to some extent. All Algol 68 code
written in conjunction with VCPGUI (to 'include more batteries') is less than
7500 lines (a bit over 5000 LOC).


---

## Run DSURF and communicate with it.

```
PROC dsurf_end_handler = (REF FILE f) BOOL: 

```
Handle end of the pipe connections to DSURF (DSURF exited?).
Establish by using "on logical file end(pipes)" in main line,

---

```
PROC open_dsurf = (STRING title, INT width, INT height) PIPE: 

```
Run DSURF returning a pair of pipes connected to its stdin and stdout.

---

```
PROC close_dsurf = (PIPE p) VOID: 

```
Close pipes to DSURF.

---

```
PROC sync_dsurf = (PIPE p, STRING s) VOID: 

```
Synchronise client and DSURF.

---

```
PROC write_dsurf = ( PIPE p, STRING outstring ) VOID: 

```
Write strings to DSURF.

---

```
PROC read_dsurf = ( PIPE p ) STRING: 

```
Read strings from DSURF.

---

## DSURF Drawing Primitives

```
PROC push_dstate = VOID: 

```
Push the DSURF drawing state.

---

```
PROC pop_dstate = VOID: 

```
Pop the DSURF drawing state.

---

```
PROC rgba_string = STRING: 

```
Convert the RGBA part of the drawing state to string.

---

```
PROC rgb_font_string = STRING: 

```
Convert RGB part of drawing state and the font number to string.

---

```
PROC ds_reset_display = ( PIPE p ) VOID: 

```
Reset the display list.

---

```
PROC ds_show_display = ( PIPE p ) VOID: 

```
Show display (swap buffers)

---

```
PROC ds_use_main = ( PIPE p ) VOID: 

```
Use the main display list.

---

```
PROC ds_use_overlay = ( PIPE p ) VOID: 

```
Use the overlay display list.

---

```
PROC ds_load_texture = ( PIPE p, STRING filename ) VOID: 

```
Load a texture atlas.

---

```
PROC ds_load_font = ( PIPE p, STRING filename, INT size, INT ifont ) VOID: 

```
Load a TrueType font.

---

```
PROC ds_set_colour_rgba = ( PIPE p, INT r, g, b, a ) VOID: 

```
Set the drawing colour.

---

```
PROC ds_clear = ( PIPE p ) VOID: 

```
Clear the display (fill with current colour).

---

```
PROC ds_rectangle = ( PIPE p, INT x1, y1, x2, y2 ) VOID: 

```
Draw a rectangle.

---

```
PROC ds_rounded_rectangle = ( PIPE p, INT x1, y1, x2, y2, r ) VOID: 

```
Draw a rounded rectangle.

---

```
PROC ds_rectangle_filled = ( PIPE p, INT x1, y1, x2, y2 ) VOID: 

```
Draw a filled rectangle.

---

```
PROC ds_rounded_rectangle_filled = ( PIPE p, INT x1, y1, x2, y2, r ) VOID: 

```
Draw a filled rounded rectangle.

---

```
PROC ds_line = ( PIPE p, INT x1, y1, x2, y2 ) VOID: 

```
Add a line to the display list.

---

```
PROC ds_thick_line = ( PIPE p, INT x1, y1, x2, y2, w ) VOID: 

```
Add a thick line to the display list.

---

```
PROC ds_vmark = ( PIPE p, INT x, y, dy ) VOID: 

```
Add a vertical line, length dy center x,y to the display list.

---

```
PROC ds_circle = ( PIPE p, INT x1, y1, r ) VOID: 

```
Add a circle to the display list.

---

```
PROC ds_circle_filled = ( PIPE p, INT x1, y1, r ) VOID: 

```
Add a filled circle to the display list.

---

```
PROC ds_arc = ( PIPE p, INT x1, y1, r, start_angle, end_angle ) VOID: 

```
Add an arc to the display list.

---

```
PROC ds_ellipse = ( PIPE p, INT x1, y1, rx, ry ) VOID: 

```
Add an ellipse to the display list.

---

```
PROC ds_ellipse_filled = ( PIPE p, INT x1, y1, rx, ry ) VOID: 

```
Add a filled ellipse to the display list.

---

```
PROC ds_text = ( PIPE p, STRING s, INT x, y, BOOL centered, INT dy ) VOID: 

```
Add basic text to the display list.

---

```
PROC ds_text_in_font = ( PIPE p, STRING s, INT x, y, BOOL centered, INT 
dy ) VOID: 

```
Add text drawn with a font renderer to the display list.

---

```
PROC ds_set_font = ( PIPE p, INT ifont ) VOID: 

```
Set the current font by number.

---

```
PROC ds_sub_texture = ( PIPE p, STRING name, INT x, y, downscale, BOOL 
centered, INT rotang, rx, ry ) VOID: 

```
Draw a sub-texture from the current texture atlas.

---

```
PROC ds_sense_rect = ( PIPE p, STRING tag, STRING event_mask, INT x, y, 
sx, sy, downscale, BOOL centered ) VOID: 

```
Add a sense rectangle, specifying the events of interest.

---

```
PROC ds_remove_at = ( PIPE p, INT x, y ) VOID: 

```
Remove all primitives at (x,y) from a display list.

---

```
PROC ds_start_timer = ( PIPE p, INT milliseconds ) VOID: 

```
Start periodic event timer.

---

```
PROC ds_stop_timer = ( PIPE p ) VOID: 

```
Stop periodic event timer.

---

```
PROC ds_config_set_fontdir = ( STRING dir ) VOID: 

```
Set the TrueType font directory to search for fonts. Call BEFORE open_gui().

---

```
PROC ds_config_set_fast_update = ( BOOL yes ) VOID: 

```
Set or clear the "fast update" capability flag. If FALSE, only update with show_display commands.

---

```
PROC ds_config_set_on_top = ( BOOL yes ) VOID: 

```
Set keep DSURF window on top.

---

```
PROC ds_config_set_no_border = ( BOOL yes ) VOID: 

```
Set hide DSURF window decorations.

---

```
PROC ds_config_set_click_audio = ( STRING filename ) VOID: 

```
Set the audio file to use for button clicks.

---

## Tiny UI code

```
PROC add_component = (ANYBUTTONREF component) VOID: 

```
Add a component "button" of any type to the UI.

---

## Standard USERPROCs

```
PROC null_proc = (STRING tag, STRING event, ANYBUTTONREF state) BOOL: 

```
Do nothing.

---

```
PROC quit_proc = (STRING tag, STRING event, ANYBUTTONREF state) BOOL: 

```
Exit the event loop.

---

## Push button

```
PROC read_pushbutton_metrics = (STRING talfile) VOID: 

```
Read layout data for a push button from a texture atlas file.

---

```
PROC make_pushbutton = (STRING tag, STRING label, USERPROC proc, INT x, 
y) REF PUSHBUTTON: 

```
Make a push button.

---

```
PROC add_pushbutton = (STRING tag, STRING label, USERPROC proc, INT x, 
y) VOID: 

```
Add a push button to the UI.

---

```
PROC get_dimensions_pushbutton = (REF INT width, height) VOID: 

```
Get the dimensions (width/height) of a push button.

---

```
PROC instantiate_pushbutton = (PIPE p, REF PUSHBUTTON pb) VOID: 

```
Put the drawing commands for a push button in the DSURF display list.

---

```
PROC update_pushbutton = (PIPE p, REF PUSHBUTTON pb, STRING event) VOID: 

```
Update the appearance of a push button. Does nothing at present.

---

```
PROC set_pushbutton = (PIPE p, STRING tag, BOOL callproc) VOID: 

```
Set (i.e. push) a push button.

---

## Toggle button

```
PROC read_togglebutton_metrics = (STRING talfile) VOID: 

```
Read layout data for a toggle button from a texture atlas file.

---

```
PROC make_togglebutton = (STRING tag, STRING label, USERPROC proc, INT 
x, y, BOOL state) REF TOGGLEBUTTON: 

```
Make a toggle button.

---

```
PROC add_togglebutton = (STRING tag, STRING label, USERPROC proc, INT x, 
y, BOOL state) VOID: 

```
Add a toggle button to the UI.

---

```
PROC get_dimensions_togglebutton = (REF INT width, height) VOID: 

```
Get the dimensions (width/height) of a toggle button.

---

```
PROC instantiate_togglebutton = (PIPE p, REF TOGGLEBUTTON tb) VOID: 

```
Put the drawing commands for a toggle button in the DSURF display list.

---

```
PROC update_togglebutton = (PIPE p, REF TOGGLEBUTTON tb, STRING event) 
VOID: 

```
Update the state and appearance of a toggle button.

---

```
PROC set_togglebutton = (PIPE p, STRING tag, BOOL on_or_off, BOOL callproc) 
VOID: 

```
Set the state of a toggle button. Optionally call its USERPROC.

---

## Radio button

```
PROC read_radiobutton_metrics = (STRING talfile) VOID: 

```
Read layout data for a radio button from a texture atlas file.

---

```
PROC make_radiobutton = (STRING tag, STRING labels, USERPROC proc, INT 
x, y, INT which) REF RADIOBUTTON: 

```
Make a radio button.

---

```
PROC add_radiobutton = (STRING tag, STRING labels, USERPROC proc, INT x, 
y, INT which) VOID: 

```
Add a radio button to the UI.

---

```
PROC get_dimensions_radiobutton = (REF RADIOBUTTON rb, REF INT width, height, 
BOOL single) VOID: 

```
Get the dimensions (width/height) of a push button.

---

```
PROC instantiate_radiobutton = (PIPE p, REF RADIOBUTTON rb) VOID: 

```
Put the drawing commands for a radio button in the DSURF display list.

---

```
PROC update_radiobutton = (PIPE p, REF RADIOBUTTON rb, STRING event, STRING 
sub_tag) VOID: 

```
Update the state and appearance of a radio button (group).

---

```
PROC get_radiobutton_sel_label = (REF RADIOBUTTON rb) STRING: 

```
Get the label of the currently selected button.

---

```
PROC set_radiobutton = (PIPE p, STRING tag, INT which, BOOL callproc) VOID: 

```
Set the state of a radio button. Optionally call its USERPROC.

---

```
PROC set_radiobutton_by_label = (PIPE p, STRING tag, STRING label, BOOL 
callproc) BOOL: 

```
Set a radiobutton by locating a specified label in the sub-buttons. Return FALSE if label not found.

---

```
PROC change_radiobutton_label = (PIPE p, STRING tag, STRING label, STRING 
new_label) BOOL: 

```
Change the label displayed by one button in a radio button. Labels must be unique for this to work!

---

```
PROC get_radiobutton_x_boundaries = (STRING tag) []INT: 

```
Get the X coordinate of the left boundary of each button on a radio button.
Returns an array of n_buttons+1 x coordinates: n_buttons+1 has the right edge of the array.

---

## Numeric display

```
PROC read_numericdisplay_metrics = (STRING talfile) VOID: 

```
Read layout data for a numeric display from a texture atlas file.

---

```
PROC make_numericdisplay = (STRING tag, INT x, y, INT value, INT n_digits, 
INT n_dps, BOOL lz_sup) REF NUMERICDISPLAY: 

```
Make a numeric display.

---

```
PROC add_numericdisplay = (STRING tag, INT x, y, INT value, INT n_digits, 
INT n_dps, BOOL lz_sup) VOID: 

```
Add a numeric display.

---

```
PROC calc_numeric_layout = (REF NUMERICDISPLAY nd, REF INT digit_width, 
array_width, xc, yc) VOID: 

```
Calculate layout parameters for a numeric display.

---

```
PROC calc_dp_position = (REF NUMERICDISPLAY nd, INT digit_width, array_width, 
yc, REF INT xcc, ycc) VOID: 

```
Calculate where a decimal point should be drawn. This is a bit hairy.

---

```
PROC calc_numeric_bezel = (REF NUMERICDISPLAY nd, INT digit_width, array_width, 
REF INT x1, y1, x2, y2) VOID: 

```
Calculate corners of a bezel rectangle for a numeric display.

---

```
PROC get_dimensions_numericdisplay = (REF NUMERICDISPLAY nd, REF INT width, 
height) VOID: 

```
Get the dimensions (width/height) of a numeric display.

---

```
PROC instantiate_numericdisplay = (PIPE p, REF NUMERICDISPLAY nd) VOID: 

```
Put the drawing commands for a numeric display in the DSURF display list.

---

```
PROC update_numericdisplay = (PIPE p, REF NUMERICDISPLAY nd, INT value, 
INT n_dps, BOOL lz_sup) VOID: 

```
Update the contents of a numeric display.

---

```
PROC set_numericdisplay = (PIPE p, STRING tag, INT value, INT n_dps, BOOL 
lz_sup) VOID: 

```
Set the contents of a numeric display. Note value must be scaled according to n_dps by the caller.

---

```
PROC set_numericdisplay_real = (PIPE p, STRING tag, REAL rvalue, INT n_dps, 
BOOL lz_sup) VOID: 

```
Set the contents of a numeric display to a REAL value.

---

```
PROC set_numericdisplay_units = (PIPE p, STRING tag, STRING units) VOID: 

```
Set the units string of a numeric display (appears in upper part of left blank area).

---

```
PROC set_numericdisplay_function = (PIPE p, STRING tag, STRING function) 
VOID: 

```
Set the function string of a numeric display (appears in lower part of left blank area).

---

```
PROC set_numericdisplay_range = (PIPE p, STRING tag, STRING range) VOID: 

```
Set the range string of a numeric display (appears in middle part of left blank area).

---

```
PROC set_numericdisplay_warn = (PIPE p, STRING tag, BOOL warn) VOID: 

```
Set or clear the 'warn' flag. This controls the bezel colour. Used by keypad when setting values.

---

## Knob

```
PROC read_knob_metrics = (STRING talfile) VOID: 

```
Read layout data for a knob from a texture atlas file.

---

```
PROC make_knob = (STRING tag, STRING label, USERPROC proc, INT x, y, INT 
angmin, angmax, REAL vmin, vmax, vnow) REF KNOB: 

```
Make a knob.

---

```
PROC add_knob = (STRING tag, STRING label, USERPROC proc, INT x, y, INT 
angmin, angmax, REAL vmin, vmax, vnow) VOID: 

```
Add a knob to the UI.

---

```
PROC get_dimensions_knob = (REF INT width, height) VOID: 

```
Get the dimensions (width/height) of a toggle button.

---

```
PROC instantiate_knob = (PIPE p, REF KNOB kb) VOID: 

```
Put the drawing commands for a knob in the DSURF display list.

---

```
PROC parse_event_knob = (STRING event, REF INT angle) BOOL: 

```
Extract the angle parameter from a knob event string. Return TRUE if it is an A event.

---

```
PROC angle_to_value_knob = (REF KNOB kb, INT angle) REAL: 

```
Convert an angle to a value mapping angmin->vmin and angmax->vmax.

---

```
PROC value_to_angle_knob = (REF KNOB kb, REAL value) INT: 

```
Convert a value to an angle.

---

```
PROC update_knob = (PIPE p, REF KNOB kb, STRING event) VOID: 

```
Update the state and appearance of a knob.

---

```
PROC set_knob = (PIPE p, STRING tag, REAL vnew, BOOL callproc) VOID: 

```
Set the state of a knob. Optionally call its USERPROC.

---

## Selector

```
PROC read_selector_metrics = (STRING talfile) VOID: 

```
Read layout data for a selector from a texture atlas file.

---

```
PROC make_selector = (STRING tag, STRING labels, USERPROC proc, INT x, 
y, INT which) REF SELECTOR: 

```
Make a selector switch.

---

```
PROC add_selector = (STRING tag, STRING labels, USERPROC proc, INT x, y, 
INT which) VOID: 

```
Add a selector switch to the UI.

---

```
PROC get_dimensions_selector = (REF INT width, height) VOID: 

```
Get the dimensions (width/height) of a toggle button.

---

```
PROC selector_line_end_points = (REF SELECTOR sb, INT radius, REAL c_theta, 
REF INT xi, yi) VOID: 

```
Find the end point of a line from selector center to some radius at some angle.

---

```
PROC selector_angles = (REF SELECTOR sb, REF REAL d_theta, t_theta, c_theta) 
VOID: 

```
Calculate angles in radians needed to draw a selector switch.
This is a bit "odd" as 0 is up and things need to be measured clockwise ...

---

```
PROC selector_angle_from_choice = (REF SELECTOR sb, INT i_choice) INT: 

```
Given a selector setting (choice), return an angle in degrees.

---

```
PROC selector_choice_from_angle = (REF SELECTOR sb, INT angle) INT: 

```
Given an angle in degrees, return a selector setting (choice).

---

```
PROC parse_event_selector = (STRING event, REF INT angle, INT dangle) BOOL: 

```
Parse a selector event.

---

```
PROC instantiate_selector = (PIPE p, REF SELECTOR sb) VOID: 

```
Put the drawing commands for a selector in the DSURF display list.

---

```
PROC update_selector = (PIPE p, REF SELECTOR sb, STRING event) VOID: 

```
Update the state and appearance of a selector.

---

```
PROC set_selector = (PIPE p, STRING tag, INT inew, BOOL callproc) VOID: 

```
Set the state of a selector. Optionally call its USERPROC.

---

## Keypad

```
PROC read_keypad_metrics = (STRING talfile) VOID: 

```
Read layout data for a keypad from a texture atlas file. $$$

---

```
PROC make_keypad = (STRING tag, USERPROC proc, INT x, y, STRING ndtag) 
REF KEYPAD: 

```
Make a keypad.

---

```
PROC add_keypad = (STRING tag, USERPROC proc, INT x, y, STRING ndtag) VOID: 

```
Add a keypad to the UI.

---

```
PROC get_dimensions_keypad = (REF INT width, height) VOID: 

```
Get the dimensions (width/height) of a keypad.

---

```
PROC instantiate_keypad = (PIPE p, REF KEYPAD kp) VOID: 

```
Put the drawing commands for a keypad in the DSURF display list.

---

```
PROC get_keypad = (PIPE p, STRING tag) REAL: 

```
Get value held by a keypad as a real number.

---

```
PROC get_keypad_internal = (REF KEYPAD kp) REAL: 

```
Convert keypad's internal representation to a REAL.

---

```
PROC old_key_keypad = (REF KEYPAD kp) VOID: 

```
Old key pressed: call the callback to restore the previous contents
of the associated numeric display. Use event string "A,0" for this.

---

```
PROC set_key_keypad = (REF KEYPAD kp) VOID: 

```
Set key pressed: call the callback to set the value in the application associated with
the keypad and numeric display. Use event string "B,0" for this.

---

```
PROC update_keypad = (PIPE p, REF KEYPAD kp, STRING event, STRING sub_tag) 
VOID: 

```
Update the state and appearance of a keypad.

---

```
PROC set_keypad = (PIPE p, STRING tag, REAL value, INT ndp, BOOL callproc) 
VOID: 

```
Set the state of a keypad. Optionally call its USERPROC.

---

## Timer

```
PROC make_timer = (STRING tag, USERPROC proc, INT milliseconds) REF TIMER: 

```
Make a timer. For now, there can only be one and it must have tag 'timer'.

---

```
PROC add_timer = (STRING tag, USERPROC proc, INT milliseconds) VOID: 

```
Add a timer to the defined components.

---

```
PROC instantiate_timer = (PIPE p, REF TIMER tm) VOID: 

```
Put the command to create a timer in the DSURF display list.

---

```
PROC update_timer = (PIPE p, REF TIMER tm, STRING event) VOID: 

```
Update the appearance of a timer (which has no appearance).

---

```
PROC set_timer = (PIPE p, STRING tag, BOOL run, BOOL callproc) VOID: 

```
Set a timer. This starts (run=TRUE) or stops (run=FALSE) it.

---

## Lamp

```
PROC read_lamp_metrics = (STRING talfile) VOID: 

```
Read layout data for a lamp from a texture atlas file.

---

```
PROC make_lamp = (STRING tag, INT x, y, BOOL state) REF LAMP: 

```
Make a lamp.

---

```
PROC add_lamp = (STRING tag, INT x, y, BOOL state) VOID: 

```
Add a lamp to the UI.

---

```
PROC get_dimensions_lamp = (REF INT width, height) VOID: 

```
Get the dimensions (width/height) of a lamp.

---

```
PROC instantiate_lamp = (PIPE p, REF LAMP lp) VOID: 

```
Put the drawing commands for a lamp in the DSURF display list.

---

```
PROC update_lamp = (PIPE p, REF LAMP lp) VOID: 

```
Update the appearance of a lamp. Remove existing item for the lamp then instantiate.

---

```
PROC set_lamp = (PIPE p, STRING tag, BOOL state) VOID: 

```
Set a lamp (turn it on or off).

---

## Time display

```
PROC read_timedisplay_metrics = (STRING talfile) VOID: 

```
Read layout data for a time display from a texture atlas file.

---

```
PROC make_timedisplay = (STRING tag, INT x, y, INT h, m, s) REF TIMEDISPLAY: 

```
Make a time display.

---

```
PROC add_timedisplay = (STRING tag, INT x, y, INT h, m, s) VOID: 

```
Add a time display.

---

```
PROC add_timedisplay_hms = (STRING tag, INT x, y, HMSTIME hms) VOID: 

```
Add a time display initialised with an HMSTIME.

---

```
PROC calc_time_layout = (REF TIMEDISPLAY td, REF INT digit_width, digit_height, 
colon_width, array_width, xc, yc) VOID: 

```
Calculate layout parameters for a time display.

---

```
PROC calc_time_bezel = (REF TIMEDISPLAY td, INT digit_width, colon_width, 
array_width, REF INT x1, y1, x2, y2) VOID: 

```
Calculate corners of a bezel rectangle for a time display. Same logic as numeric display.

---

```
PROC get_dimensions_timedisplay = (REF TIMEDISPLAY td, REF INT width, height) 
VOID: 

```
Get the dimensions (width/height) of a time display.

---

```
PROC instantiate_timedisplay = (PIPE p, REF TIMEDISPLAY td) VOID: 

```
Put the drawing commands for a time display in the DSURF display list.

---

```
PROC update_timedisplay = (PIPE p, REF TIMEDISPLAY td, INT h, m, s) VOID: 

```
Update state of the time display and redraw it, remembering to remove to old version.

---

```
PROC update_timedisplay_hms = (PIPE p, REF TIMEDISPLAY td, HMSTIME hms) 
VOID: 

```
As update_timedisplay(), but with an HMSTIME argument.

---

```
PROC set_timedisplay = (PIPE p, STRING tag, INT h, m, s) VOID: 

```
Set the contents of a time display.

---

```
PROC set_timedisplay_hms = (PIPE p, STRING tag, HMSTIME hms) VOID: 

```
As set_timedisplay(), but with an HMSTIME argument.

---

## Layout aids

```
PROC evenly_distribute = (GEOMETRY g, []INT button_counts, INT button_width, 
BOOL flush_lr, REF BOOL over) []INT: 

```
Distribute groups of buttons, each with a number of buttons specified in a
button_counts array, with an individual button having a width of button_width,
so they are evenly distributed with equal gaps across total_width. The same logic
applies to vertical distribution with heights instead of widths,
If the arrangement won't fit in total_width, return over=TRUE, else FALSE.

---

```
PROC vertical_spacing_buttons = (BOOL with_group_label) INT: 

```
Standard vertical spacing between rows of buttons.

---

```
PROC vertical_spacing_knobs = (BOOL with_group_label) INT: 

```
Standard vertical spacing between roes of knobs.

---

```
PROC vertical_group_label_spacing = INT: 

```
Standard vertical spacing between components and labels beneath them.

---

```
PROC corner_to_center = (STRING corner, INT x, y, width, height, REF INT 
xcen, ycen) VOID: 

```
Convert a corner coordinate to a center coordinate.

---

## UI Internals

```
PROC get_metric_info = ( STRING from_file, STRING key, REF []INT values 
) INT: 

```
Get texture metric information for texture key from a texture atlas file from_file.

---

```
PROC read_component_metrics = (STRING talfile) VOID: 

```
Read component metrics from a texture atlas file.

---

```
PROC instantiate = (PIPE p) VOID: 

```
Instantiate all defined components (put drawing cmds in DSURF display list).

---

```
PROC update_component = (PIPE p, ANYBUTTONREF component, STRING event, 
STRING sub_tag) VOID: 

```
Update the appearance and state of a component. Output only components need not he handled here.

---

```
PROC find_component = (STRING tag, REF USERPROC this_proc) INT: 

```
Find the index of a component given its tag and also return its USERPROC.

---

```
PROC check_new_tag = (STRING tag) BOOL: 

```
Return TRUE if tag hasn't been used yet.

---

```
PROC check_old_tag = (STRING tag) INT: 

```
Return 0 if tag hasn't been used or its index in the components array otherwise.

---

```
PROC get_geometry_status = (STRING tag, REF GEOMETRY g) BOOL: 

```
Get position and size information for a named component.

---

```
PROC get_geometry = (STRING tag, REF GEOMETRY g) VOID: 

```
As get_geometry_status(), but ignore status.

---

```
PROC print_geometry = (REF GEOMETRY g) VOID: 

```
Print a geometry object.

---

```
PROC move_component_status = (STRING tag, STRING corner, INT xc, yc) BOOL: 

```
Move the location of a named component so that 'corner' is at (xc,yc).
Corner must be one of bl (bottom left), tl (top left), br (bottom right), tr (top right), c (center)

---

```
PROC move_component = (STRING tag, STRING corner, INT xc, yc) VOID: 

```
As move_component_status(), but ignore status.

---

```
PROC ui_colour = (PIPE p, STRING colour_name) VOID: 

```
Named UI colour selection.

---

```
PROC ui_colour_base_font = (PIPE p, STRING colour_name) VOID: 

```
Named colour selection and change to base font (0).

---

```
PROC open_gui = (STRING title, INT width, height) PIPE: 

```
Higher level function to start DSURF and do 'standard' things.

---

```
PROC run_event_loop = (PIPE p) VOID: 

```
Run the event loop.

---

## Statistics
PROC declarations: 164\
  OP declarations: 0\
      Total lines: 2524\
    Lines of code: 1850\

# Global Statistics
```
  Global PROC declarations: 317
    Global OP declarations: 42
        Global total lines: 7573
Global total lines of code: 5391
```
