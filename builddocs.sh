#!/bin/bash
if ! command -v a68g >/dev/null 2>&1; then
    echo "Algol 68 Genie is not installed. Cannot continue."
    exit 1
fi
#
# Remove old doc versions.
rm -f vcpdoc.md
rm -f vcpdoc.html
rm -f vcpdoc.pdf
#
# Base Markdown output from A68 source code.
a68g mindoc68.a68 -- -t "VCPGUI Preludes" vcpdoc.ini vcpdoc.md
#
# If Pandoc is installed, generate HTML document.
if command -v pandoc >/dev/null 2>&1; then
    echo "Pandoc is installed. Generating HTML."
    a68g mindoc68.a68 -- -t "VCPGUI Preludes" vcpdoc.ini vcpdoc.html
#
# If LaTeX is installed at the expected location, generate PDF document.
    if [ -f /usr/bin/pdflatex ]; then
        echo "PDFLaTeX is installed where expected. Generating PDF."
        a68g mindoc68.a68 -- -t "VCPGUI Preludes" vcpdoc.ini vcpdoc.pdf
    fi
fi
#
# Put the result documents in doc subdirectory.
if [ -f vcpdoc.md ]; then
    rm -f doc/vcpdoc.md
    mv vcpdoc.md doc
fi
if [ -f vcpdoc.html ]; then
    rm -f doc/vcpdoc.html
    mv vcpdoc.html doc
fi
if [ -f vcpdoc.pdf ]; then
    rm -f doc/vcpdoc.pdf
    mv vcpdoc.pdf doc
fi
#
echo "Done."
