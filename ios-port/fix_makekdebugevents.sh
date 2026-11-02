#!/bin/bash
# Fix makekdebugevents.py: TabError, escape sequences, IndentationError
set -euo pipefail

FILE="ios-port/xnu-4570.71.2-source/bsd/kern/makekdebugevents.py"

if [[ ! -f "$FILE" ]]; then
    echo "ERROR: File not found: $FILE"
    echo "Current directory: $(pwd)"
    echo "Files in ios-port/:"
    ls -la ios-port/ 2>/dev/null || echo "ios-port not found"
    exit 1
fi

echo "Fixing $FILE..."

# 1. Replace all tabs with 4 spaces
sed -i '' $'s/\t/    /g' "$FILE"

# 2. Fix invalid escape sequence \s -> \\s
sed -i '' 's/\\s/\\\\s/g' "$FILE"

# 3. Fix indentation after 'for' statements
awk '
/^[[:space:]]*for [^;]*:[[:space:]]*$/ {
    print
    getline nextline
    if (nextline ~ /^[[:space:]]*[^[:space:]]/ && nextline !~ /^[[:space:]]{4}/) {
        sub(/^[[:space:]]*/, "    ", nextline)
    }
    print nextline
    next
}
{ print }
' "$FILE" > "$FILE.tmp" && mv "$FILE.tmp" "$FILE"

echo "Fixed makekdebugevents.py"