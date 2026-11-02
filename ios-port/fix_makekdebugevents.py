#!/usr/bin/env python3
"""
Fix makekdebugevents.py:
1. TabError: replace all tabs with 4 spaces
2. Invalid escape sequence \s -> \\s
3. IndentationError after 'for' statement - ensure proper 4-space indent
"""
import sys
import os

FILEPATH = 'ios-port/xnu-4570.71.2-source/bsd/kern/makekdebugevents.py'

def main():
    # Check if file exists
    if not os.path.exists(FILEPATH):
        print(f"ERROR: File not found: {FILEPATH}")
        print(f"Current working directory: {os.getcwd()}")
        print(f"Files in current dir: {os.listdir('.')}")
        if os.path.exists('ios-port'):
            print(f"ios-port contents: {os.listdir('ios-port')}")
        return 1
    
    print(f"Reading file: {FILEPATH}")
    with open(FILEPATH, 'r') as f:
        content = f.read()
    
    print(f"Original file size: {len(content)} chars")
    
    # 1. Replace all tabs with 4 spaces
    content = content.replace('\t', '    ')
    
    # 2. Fix invalid escape sequence \s -> \\s
    content = content.replace('\\s', '\\\\s')
    
    # 3. Fix indentation after 'for' statement
    lines = content.splitlines(keepends=True)
    fixed = []
    for i, line in enumerate(lines):
        if i > 0 and lines[i-1].strip().startswith('for ') and line.strip() and not line.startswith('    ') and not line.startswith('\t'):
            fixed.append('    ' + line.lstrip())
        else:
            fixed.append(line)
    
    content = ''.join(fixed)
    
    with open(FILEPATH, 'w') as f:
        f.write(content)
    
    print('Fixed makekdebugevents.py successfully')
    return 0

if __name__ == '__main__':
    sys.exit(main())