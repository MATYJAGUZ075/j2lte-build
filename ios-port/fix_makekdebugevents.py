#!/usr/bin/env python3
"""
Fix makekdebugevents.py:
1. TabError: replace all tabs with 4 spaces
2. Invalid escape sequence \s -> \\s
3. IndentationError after 'for' statement - ensure proper 4-space indent
"""
import sys

FILEPATH = 'ios-port/xnu-4570.71.2-source/bsd/kern/makekdebugevents.py'

def main():
    with open(FILEPATH, 'r') as f:
        content = f.read()
    
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
    
    print('Fixed makekdebugevents.py')
    return 0

if __name__ == '__main__':
    sys.exit(main())