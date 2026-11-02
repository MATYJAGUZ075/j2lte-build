#!/usr/bin/env python3
# Fix makekdebugevents.py: TabError, escape sequences, IndentationError
import sys
import os

FILEPATH = 'ios-port/xnu-4570.71.2-source/bsd/kern/makekdebugevents.py'

def main():
    if not os.path.exists(FILEPATH):
        print(f"ERROR: File not found: {FILEPATH}")
        print(f"Current directory: {os.getcwd()}")
        print(f"Files in current dir: {os.listdir('.')}")
        if os.path.exists('ios-port'):
            print(f"ios-port contents: {os.listdir('ios-port')}")
        if os.path.exists('ios-port/xnu-4570.71.2-source/bsd/kern'):
            print(f"kern contents: {os.listdir('ios-port/xnu-4570.71.2-source/bsd/kern')}")
        return 1
    
    print(f"Fixing {FILEPATH}")
    
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
    
    # Verify the fix
    with open(FILEPATH, 'r') as f:
        verify_content = f.read()
    
    if '\t' in verify_content:
        print("WARNING: Tabs still present after fix!")
    else:
        print("OK: No tabs found")
    
    if '\\s' in verify_content and '\\\\s' not in verify_content:
        print("WARNING: Unescaped \\s still present!")
    else:
        print("OK: Escape sequences fixed")
    
    # Check line 24 (0-indexed: 23) for proper indentation
    lines = verify_content.splitlines(keepends=True)
    if len(lines) > 24:
        line24 = lines[23]  # 0-indexed
        if line24.startswith('    ') or line24.startswith('\t'):
            print(f"OK: Line 24 properly indented: {repr(line24[:30])}")
        else:
            print(f"WARNING: Line 24 not properly indented: {repr(line24[:30])}")
    
    print('Fixed makekdebugevents.py')
    return 0

if __name__ == '__main__':
    sys.exit(main())