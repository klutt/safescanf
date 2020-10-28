// Compile with gcc -Wall -Wextra -pedantic -std=c17 -D_POSIX_C_SOURCE=2

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

// https://en.wikipedia.org/wiki/Variadic_macro#cite_note-3
// https://groups.google.com/group/comp.std.c/browse_thread/thread/77ee8c8f92e4a3fb/346fc464319b1ee5
#define PP_NARG(...) \
PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
_1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
_11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
_21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
_31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
_41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
_51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
_61,_62,_63,N,...) N
#define PP_RSEQ_N() \
63,62,61,60, \
59,58,57,56,55,54,53,52,51,50, \
49,48,47,46,45,44,43,42,41,40, \
39,38,37,36,35,34,33,32,31,30, \
29,28,27,26,25,24,23,22,21,20, \
19,18,17,16,15,14,13,12,11,10, \
9,8,7,6,5,4,3,2,1,0

// Counts the number of assignments that should be made by scanf and alike.
// Assumes a valid format string. If it's not valid, behavior is undefined.

int count_assignments(const char *fmt) {
    int ret = 0;

    // Note that n is removed, because it suppresses assignments
    static const char specifiers[] = "diouxaefgcspAEFGX";
    static const char singlelength[] = "hljztL";
    static const char doublelength[] = "hl";

    while(*fmt) {
        if(*fmt == '%') { 
            fmt++;

            // Skip width modification
            while(isdigit(*fmt)) fmt++;

            // Check length modification
            if(strchr(singlelength, *fmt)) {
                fmt++;
                if(strchr(doublelength, *fmt)) {
                    fmt++;
                    goto READ_SPECIFIER;
                }
                goto READ_SPECIFIER;
            }
            if(*fmt == '[') {
                fmt++;
                if(*fmt == ']') fmt++;
                else if(*fmt == '^') fmt+=2;
                while(*fmt != ']') fmt++;
                ret++;
                goto END;
            }
            goto READ_SPECIFIER;
        }

        goto END;
        
    READ_SPECIFIER:
        if(strchr(specifiers, *fmt)) 
            ret++;
    END:
        fmt++;
    }

    return ret;
}

// Calculate how many arguments a format string should have by creating a .c file which
// is compiled and the number of warnings are counted. This value is treated as the number
// of arguments the format strings wants.
size_t get_no_warnings(const char *fmt)
{
    FILE *fp = fopen("warning.c", "w");
    if(!fp) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(fp, "#include <stdio.h>\n\nint main(void) {\n\tscanf(\"%s\");\n}\n", fmt);
    fclose(fp);

    fp = popen("gcc warning.c -Wall -Wextra -pedantic 2>&1 | grep \"warning: format\" | wc -l", "r");

    if(!fp) {
        perror("Error executing command");
        exit(EXIT_FAILURE);
    }

    char output[100];
    fgets(output, sizeof(output), fp);
    size_t ret;
    if(sscanf(output, "%zu", &ret) != 1) {
        perror("Error reading output");
        exit(EXIT_FAILURE);
    }
    return ret;
}

#define safescanf(F,...) safescanfaux(PP_NARG(__VA_ARGS__),F,__VA_ARGS__)

void safescanfaux(size_t n, const char *fmt, ...) {
    size_t ca = count_assignments(fmt);
    if(ca != n) {
        fprintf(stderr, "Wrong number of arguments. Format string expected: %zu but got %zu\n", ca, n);
        exit(EXIT_FAILURE);
    }

    va_list args;
    va_start(args, fmt);
    int r = vscanf(fmt, args);
    va_end(args);
    if(ca != (size_t)r) {
        fprintf(stderr, "scanf returns number of successful assignments. Returned: %d but expected %zu", r, n);
        exit(EXIT_FAILURE);
    }
}
    

int main(void)
{
    struct test_case {
        const char *fmt;
        const unsigned n;
    } test_cases[] = {
        { "foo", 0 },
        { "%s", 1 },
        { "%d%d", 2 },
        { "%lld", 1 },
        { "%%", 0 },
        { "%d%%%d", 2 },
        { "%2333d%c%33f", 3 },
        { "%[bar]", 1 }, 
        { "%*d", 0 },
        { "%*22d", 0 },
        { "%d%p%c", 3 },
        { "%[%]aa]%d", 2},
        { "%[]%d[]", 1},
        { "%[^]%d]", 1},
        { "%[^a]%d]", 2}
    };

    for(size_t i=0; i<sizeof test_cases/sizeof test_cases[0]; i++) {
        struct test_case tc = test_cases[i];
        size_t count = count_assignments(tc.fmt);
        size_t warnings = get_no_warnings(tc.fmt);
        printf("%s %u %zu %zu\n", tc.fmt, tc.n, count, warnings);
        assert(count == tc.n);
        assert(count == warnings);
    }

    int x, y;
    safescanf("%d %d", &x, &y);
    printf("%d %d\n", x, y);
    
}

