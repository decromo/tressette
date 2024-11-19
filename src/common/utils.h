#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "raylib.h"

static inline void arr_remove_shift(size_t arr_size, void *arr_raw, size_t elem_size, int index) {
    // char (*arr)[elem_size];

    assert(index < arr_size);
    memmove(arr_raw + (index*elem_size), arr_raw + ((index+1)*elem_size), elem_size * (arr_size-index-1));
}

// inserts newline ('\n') characters, without splitting words if possible.
// Parameters:
//      len, the number of characters in str
//      str, pointer to the string to format
//      bufsiz, the actual size in bytes of the buffer containing said string
//      line_maxlen, the desired optimal character lenght of a single line
//      over_len_maxchars, the acceptable number of characters a line can go over the desired lenght
//      prec_lf_maxchars, the acceptable amount of whitespace remaining in a line when a precocious linefeed is
//          made on said line (i.e. when the newline character is inserted before reaching line_maxlen characters)
static inline void linefeed_string(int len, char *str, size_t bufsiz, int line_maxlen, int over_len_maxchars, int prec_lf_maxchars) {
    // minus one to reserve space for the null terminator
    size_t bufferBytesLeft = bufsiz-1 - len;

    // execute subroutine until the remaining text is shorter than the space available
    while (len > line_maxlen + over_len_maxchars) {
        int spaceBeforeIdx, spaceAfterIdx;
        char *spaceBefore, *spaceAfter;
        spaceBeforeIdx = spaceAfterIdx = 0;
        spaceBefore = spaceAfter = str;

        char *foundSpace;
        while (spaceAfterIdx < line_maxlen) {
            if (NULL == (foundSpace = strchr(spaceAfter+1, ' '))) { break; }
            spaceBefore = spaceAfter;
            spaceBeforeIdx = spaceAfterIdx;
            spaceAfter = foundSpace;
            spaceAfterIdx = labs(str - spaceAfter) /* / sizeof(*renderMemory->statusStr) */;
        }

        char *breakPtr = NULL;
        int breakIdx = -1;

        if (spaceAfter == str || (line_maxlen - spaceBeforeIdx > prec_lf_maxchars)) {
            // this array addressing should not got out of bounds as per the while condition
            memmove(&str[line_maxlen + 1], &str[line_maxlen], len - line_maxlen - (bufferBytesLeft > 0));
            breakPtr = &str[line_maxlen];
            breakIdx = 30;
            bufferBytesLeft--;
            str[len] = '\0'; // just in case
        }
        else if (spaceAfterIdx > line_maxlen + over_len_maxchars) {
            *spaceBefore = '\n';
            breakPtr = spaceBefore;
            breakIdx = spaceBeforeIdx;
        }
        else {
            *spaceAfter = '\n';
            breakPtr = spaceAfter;
            breakIdx = spaceAfterIdx;
        }

        assert(breakPtr != NULL && breakIdx != -1);
        *breakPtr = '\n';
        str = breakPtr+1;
        len -= breakIdx+1;
    }
}