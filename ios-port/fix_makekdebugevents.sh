#!/bin/bash
# Fix makekdebugevents.py: TabError, escape sequences, IndentationError
set -euo pipefail

FILE="ios-port/xnu-4570.71.2-source/bsd/kern/makekdebugevents.py"

if [[ ! -f "$FILE" ]]; then
    echo "ERROR: File not found: $FILE"
    ls -la ios-port/
    ls -la ios-port/xnu-4570.71.2-source/bsd/kern/ 2>/dev/null || echo "kern dir not found"
    exit 1
fi

echo "Fixing $FILE..."

# 1. Replace all tabs with 4 spaces
sed -i '' $'s/\t/    /g' "$FILE"

# 2. Fix invalid escape sequence \s -> \\s
sed -i '' 's/\\s/\\\\s/g' "$FILE"

# 3. Fix indentation after 'for' statements
# Use sed to fix lines after 'for ...:' that don't have proper indentation
sed -i '' -e '/^[[:space:]]*for [^;]*:[[:space:]]*$/{
    n
    s/^[[:space:]]*/    /
}' "$FILE"

echo "Fixed makekdebugevents.py"