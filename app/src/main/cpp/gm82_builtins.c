#include "gml_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

int gm82_native_call(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out) {
    if (!name || !out) return 0;

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

    // Math & Geometry built-ins
    if (!strcmp(name, "point_distance_3d") && count == 6) {
        double dx = args[3].real - args[0].real;
        double dy = args[4].real - args[1].real;
        double dz = args[5].real - args[2].real;
        *out = gml_value_real(sqrt(dx * dx + dy * dy + dz * dz));
        return 1;
    }
    if (!strcmp(name, "degtorad") && count == 1) {
        *out = gml_value_real(args[0].real * 0.017453292519943295);
        return 1;
    }
    if (!strcmp(name, "radtodeg") && count == 1) {
        *out = gml_value_real(args[0].real * 57.29577951308232);
        return 1;
    }
    if (!strcmp(name, "sqr") && count == 1) {
        *out = gml_value_real(args[0].real * args[0].real);
        return 1;
    }
    if (!strcmp(name, "sign") && count == 1) {
        double v = args[0].real;
        *out = gml_value_real(v > 0.0 ? 1.0 : (v < 0.0 ? -1.0 : 0.0));
        return 1;
    }
    if (!strcmp(name, "clamp") && count == 3) {
        double v = args[0].real, lo = args[1].real, hi = args[2].real;
        *out = gml_value_real(v < lo ? lo : (v > hi ? hi : v));
        return 1;
    }
    if (!strcmp(name, "dot_product") && count == 4) {
        *out = gml_value_real(args[0].real * args[2].real + args[1].real * args[3].real);
        return 1;
    }
    if (!strcmp(name, "angle_difference") && count == 2) {
        double diff = fmod(args[0].real - args[1].real + 180.0, 360.0);
        if (diff < 0) diff += 360.0;
        *out = gml_value_real(diff - 180.0);
        return 1;
    }

    // Motion built-in functions
    if (!strcmp(name, "motion_set") && count == 2) {
        double dir = args[0].real;
        double spd = args[1].real;
        double rad = dir * 0.017453292519943295;
        if (userdata) {
            gm82_instance *inst = (gm82_instance *)userdata;
            inst->direction = dir;
            inst->speed = spd;
            inst->hspeed = spd * cos(rad);
            inst->vspeed = -spd * sin(rad);
            gm82_instance_sync_vars_to_vm(inst);
        }
        *out = gml_value_real(1.0);
        return 1;
    }
    if (!strcmp(name, "instance_find") && count == 2) {
        *out = gml_value_real(-4.0); // noone constant (-4 in GM8.2)
        return 1;
    }
    if (!strcmp(name, "motion_add") && count == 2) {
        double dir = args[0].real;
        double spd = args[1].real;
        double rad = dir * 0.017453292519943295;
        if (userdata) {
            gm82_instance *inst = (gm82_instance *)userdata;
            inst->hspeed += spd * cos(rad);
            inst->vspeed -= spd * sin(rad);
            inst->speed = sqrt(inst->hspeed * inst->hspeed + inst->vspeed * inst->vspeed);
            if (inst->speed > 0.00001) {
                inst->direction = atan2(-inst->vspeed, inst->hspeed) / 0.017453292519943295;
                if (inst->direction < 0.0) inst->direction += 360.0;
            }
            gm82_instance_sync_vars_to_vm(inst);
        }
        *out = gml_value_real(1.0);
        return 1;
    }
    if (!strcmp(name, "move_towards_point") && count == 3) {
        double tx = args[0].real;
        double ty = args[1].real;
        double spd = args[2].real;
        if (userdata) {
            gm82_instance *inst = (gm82_instance *)userdata;
            double dx = tx - inst->x;
            double dy = ty - inst->y;
            double dir = atan2(-dy, dx) * 180.0 / 3.14159265358979323846;
            if (dir < 0) dir += 360.0;
            double rad = dir * 0.017453292519943295;
            inst->direction = dir;
            inst->speed = spd;
            inst->hspeed = spd * cos(rad);
            inst->vspeed = -spd * sin(rad);
            gm82_instance_sync_vars_to_vm(inst);
        }
        *out = gml_value_real(1.0);
        return 1;
    }
    if (!strcmp(name, "move_snap") && count == 2) {
        double hsnap = args[0].real;
        double vsnap = args[1].real;
        if (userdata && hsnap > 0.0 && vsnap > 0.0) {
            gm82_instance *inst = (gm82_instance *)userdata;
            inst->x = floor((inst->x + hsnap * 0.5) / hsnap) * hsnap;
            inst->y = floor((inst->y + vsnap * 0.5) / vsnap) * vsnap;
            gm82_instance_sync_vars_to_vm(inst);
        }
        *out = gml_value_real(1.0);
        return 1;
    }

    // Instance & Collision built-ins
    if (!strcmp(name, "place_free") && count == 2) {
        *out = gml_value_bool(1);
        return 1;
    }
    if (!strcmp(name, "place_empty") && count == 2) {
        *out = gml_value_bool(1);
        return 1;
    }
    if (!strcmp(name, "place_meeting") && count == 3) {
        *out = gml_value_bool(0);
        return 1;
    }
    if (!strcmp(name, "position_meeting") && count == 3) {
        *out = gml_value_bool(0);
        return 1;
    }
    if (!strcmp(name, "instance_position") && count == 3) {
        *out = gml_value_real(-4.0); // noone constant (-4 in GM8.2)
        return 1;
    }
    if (!strcmp(name, "instance_exists") && count == 1) {
        *out = gml_value_bool(1);
        return 1;
    }
    if (!strcmp(name, "instance_number") && count == 1) {
        *out = gml_value_real(1.0);
        return 1;
    }

    // Drawing & Text built-ins
    if (!strcmp(name, "draw_sprite") || !strcmp(name, "draw_sprite_ext") || !strcmp(name, "draw_text") ||
        !strcmp(name, "draw_line") || !strcmp(name, "draw_rectangle") || !strcmp(name, "draw_circle") ||
        !strcmp(name, "draw_set_color") || !strcmp(name, "draw_set_alpha")) {
        *out = gml_value_real(1.0);
        return 1;
    }
    if (!strcmp(name, "string_width") && count == 1) {
        const char *s = (args[0].kind == GML_V_STRING && args[0].string) ? args[0].string : "";
        *out = gml_value_real((double)(strlen(s) * 8)); // 8px per character retro font width
        return 1;
    }
    if (!strcmp(name, "string_height") && count == 1) {
        *out = gml_value_real(16.0); // 16px font height
        return 1;
    }

    return 0;
}
