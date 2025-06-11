#------------------------------------------------------------------------------

import os
from sys import argv, stdin, stdout, stderr
#------------------------------------------------------------------------------

license_header= """// SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)
//
// SPDX-License-Identifier: Apache-2.0
"""

current_comment = []

def print_reset_current_comment() -> None:

    global current_comment

    stderr.write(f"Current comment: {"".join(current_comment)}")
    current_comment = []


def to_current_comment(c: str) -> None:

    global current_comment

    current_comment.append(c)

#------------------------------------------------------------------------------

OUT_OF_COMMENT = 1
START_SLASH = 2
START_FIRST_AST = 3
IN_SIMPLE_COMMENT = 4
IN_ML_COMMENT = 5
END_AST = 6
LINE_START = 10

def state_to_string(state) -> str:

    if state == OUT_OF_COMMENT:
        return "OUT_OF_COMMENT"
    elif state == START_SLASH:
        return "START_SLASH"
    elif state == START_FIRST_AST:
        return "START_FIRST_AST"
    elif state == IN_SIMPLE_COMMENT:
        return "IN_SIMPLE_COMMENT"
    elif state == IN_ML_COMMENT:
        return "IN_ML_COMMENT"
    elif state == END_AST:
        return "END_AST"
    elif state == LINE_START:
        return "LINE_START"
#-------------------------------------------------------------------------------

in_multiline_comment = False

def is_comment(line: str) -> bool:

    global in_multiline_comment

    stderr.write(f"Line: '{line}'   multiline comment: {in_multiline_comment} ")

    comment_found = in_multiline_comment

    state = IN_ML_COMMENT if in_multiline_comment else LINE_START

    for c in line:

        stderr.write(f"{c} : {state_to_string(state)} -> ")
        if c == '/':
            if state == OUT_OF_COMMENT:
                state = START_SLASH

            elif state == START_SLASH:
                state = IN_SIMPLE_COMMENT
                comment_found = True

            elif state == START_FIRST_AST:
                state = START_SLASH

            elif state == IN_SIMPLE_COMMENT:
                to_current_comment(c)

            elif state == IN_ML_COMMENT:
                to_current_comment(c)

            elif state == END_AST:
                state = LINE_START
                in_multiline_comment = False
                print_reset_current_comment()

            elif state == LINE_START:
                state = START_SLASH


        elif c == '*':

            if state == IN_ML_COMMENT:
                state = END_AST

            if state == OUT_OF_COMMENT:
                pass

            elif state == START_SLASH:
                state = START_FIRST_AST

            elif state == START_FIRST_AST:
                in_multiline_comment = True
                comment_found = True
                state = IN_ML_COMMENT

            elif state == IN_SIMPLE_COMMENT:
                to_current_comment(c)

            elif state == IN_ML_COMMENT:
                to_current_comment(c)
                state = END_AST

            elif state == END_AST:
                to_current_comment(c)
                state = END_AST

            elif state == LINE_START:
                state = OUT_OF_COMMENT

        else:

            if state == LINE_START:
                state = OUT_OF_COMMENT

            elif state == START_SLASH or state == START_FIRST_AST:
                state = OUT_OF_COMMENT

            elif state != OUT_OF_COMMENT:
                to_current_comment(c)
                if state == END_AST:
                    state = IN_ML_COMMENT

        stderr.write(f"{state_to_string(state)} |")

    result = comment_found and ((state == IN_SIMPLE_COMMENT)  or (state == IN_ML_COMMENT) or (state == END_AST) or (state == LINE_START))
    stderr.write(f"Comment found: {comment_found} Multiline comment: {in_multiline_comment}   Retval {result}\n")
    return result

#------------------------------------------------------------------------------

still_header = True

#------------------------------------------------------------------------------

def is_comment_header(line: str) -> bool:

    global still_header

    line = line.lstrip().rstrip()

    stderr.write(f"Stripped line: {line}\n")

    if not still_header:
        return False

    elif is_comment(line):
        return True

    else:

        stderr.write("Not any longer in header")
        still_header = False
        return False

#------------------------------------------------------------------------------

if __name__ == "__main__":

    for line in stdin:
        if is_comment_header(line):
            stderr.write(f"Header: {line}\n")
        else:
            stdout.write(f"Not header: {line}\n")


    #for line in stdin:
    #    if not is_comment_header(line):
    #        stdout.write(line)


