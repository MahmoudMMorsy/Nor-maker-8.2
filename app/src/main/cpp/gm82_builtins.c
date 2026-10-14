#include "gml_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

int gm82_native_call(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out) {
    if (!name || !out) return 0;
    (void)userdata;

    if (!strcmp(name, "point_distance") && count == 4) {
        double dx = args[2].real - args[0].real;
        double dy = args[3].real - args[1].real;
        *out = gml_value_real(sqrt(dx * dx + dy * dy));
        return 1;
    }
    if (!strcmp(name, "point_direction") && count == 4) {
        double dx = args[2].real - args[0].real;
        double dy = args[1].real - args[3].real;
        double angle = atan2(dy, dx) * 180.0 / 3.14159265358979323846;
        if (angle < 0) angle += 360.0;
        *out = gml_value_real(angle);
        return 1;
    }
    if (!strcmp(name, "string_length") && count == 1) {
        const char *s = (args[0].kind == GML_V_STRING && args[0].string) ? args[0].string : "";
        *out = gml_value_real((double)strlen(s));
        return 1;
    }
    if (!strcmp(name, "string_copy") && count == 3) {
        const char *s = (args[0].kind == GML_V_STRING && args[0].string) ? args[0].string : "";
        int start = (int)args[1].real;
        int len = (int)args[2].real;
        size_t nstr = strlen(s);
        if (start < 1) start = 1;
        if (len < 0) len = 0;
        size_t begin = (size_t)(start - 1);
        if (begin > nstr) begin = nstr;
        if ((size_t)len > nstr - begin) len = (int)(nstr - begin);
        char *part = malloc((size_t)len + 1);
        if (part) {
            memcpy(part, s + begin, (size_t)len);
            part[len] = 0;
            *out = gml_value_string(part);
            free(part);
        } else {
            *out = gml_value_string("");
        }
        return 1;
    }
    if (!strcmp(name, "string_pos") && count == 2) {
        const char *needle = (args[0].kind == GML_V_STRING && args[0].string) ? args[0].string : "";
        const char *haystack = (args[1].kind == GML_V_STRING && args[1].string) ? args[1].string : "";
        const char *found = needle[0] ? strstr(haystack, needle) : haystack;
        *out = gml_value_real(found ? (double)(found - haystack + 1) : 0.0);
        return 1;
    }

    return 0;
}
