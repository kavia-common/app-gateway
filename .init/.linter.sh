#!/bin/bash\n\nSRC_DIR=\"app-gateway\" \nfor file in $(find
 \"$SRC_DIR\"\
    \ -type f \\( -name \"*.c\" -o -name \"*.cpp\
" -o -name \"*.cc\" -o -name \"*.cxx\"\
    \ \\)); do\n    echo
 \"Analyzing $file\"\n    clang --analyze -I Thunder/Source\
    \ \
"$file\"\ndone\n
