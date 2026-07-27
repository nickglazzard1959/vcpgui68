#!/bin/bash
echo "Run tests on arg_parser.a68"
echo "==========================="
#
echo -e "\n============\n(no args)\n----OK----\n"
a68g arg_parser.a68                          # OK #
echo -e "\n============\n(-h)\n----OK----\n"
a68g arg_parser.a68 -- -h                    # OK #
echo -e "\n============\n(-f -t Yikes)\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes           # OK #
echo -e "\n============\n(-f -t Yikes hello)\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes hello     # OK #
echo -e "\n============\n(-f -t)\n----FAIL----\n"
a68g arg_parser.a68 -- -f -t                 # Fail, missing argument. #
echo -e "\n============\n(-f -t -s gg)\n----FAIL----\n"
a68g arg_parser.a68 -- -f -t -s gg           # Fail, string argument (for -t) starts with -. #
echo -e "\n============\n(-f -t Yikes hello -i 67)\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes hello -i 67  # OK (hello is a positional argument)  #
echo -e "\n============\n(-f -t Yikes hello -i 67s)\n----FAIL----\n"
a68g arg_parser.a68 -- -f -t Yikes hello -i 67s # Fail, malformed integer 67s. #
echo -e "\n============\n(-f -t Yikes hello -i -67)\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes hello -i -67 # OK. Integers can be negative. #
echo -e "\n============\n(-f -t Yikes hello -i 78 *.a68)\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes hello -i 78 *.a68 # OK. Lots of positional arguments. #
echo -e "\n============\n(-f -t Yikes "Hello Kitty!" -i -78 *.a68)\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes "Hello Kitty!" -i -78 *.a68 # OK. Shell handles quoted strings for us. #
echo -e "\n============\n(--help)\n----OK----\n"
a68g arg_parser.a68 -- --help                # OK, long form options work. #
echo -e "\n============\n(-f -t Yikes "Hello Kitty!" -i -78 "*.a68")\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes "Hello Kitty!" -i -78 "*.a68" # OK. #
echo -e "\n============\n(\"Hello Kitty!\")\n----OK----\n"
a68g arg_parser.a68 -- "Hello Kitty!"        # OK, positionals only allowed. #
echo -e "\n============\n(-f -t Yikes \"Hello Kitty!\" -i +78 *.a68)\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes "Hello Kitty!" -i +78 *.a68 # OK. Signed positive integers allowed. #
echo -e "\n============\n(-f -t Yikes \"Hello Kitty!\" -i -78 \"*.a68\" -r -1.5e7)\n----OK----\n"
a68g arg_parser.a68 -- -f -t Yikes "Hello Kitty!" -i -78 "*.a68" -r -1.5e7  # OK. All real number fmts should work. #
echo -e "\n============\n"
