#include "cart.h"

#include "arkaled.h"
#include "game.h"
#include "pong.h"
#include "text_demo.h"

#include <stdio.h>
#include <string.h>

static int which;

static void skip(const char **p)
{
    while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r')
        (*p)++;
}

static int read_string(const char **p, char *out, int cap)
{
    const char *s = *p;
    int n = 0;

    if (*s != '"')
        return 0;
    s++;
    while (*s != '\0' && *s != '"') {
        if (n + 1 < cap)
            out[n++] = *s;
        s++;
    }
    if (*s != '"')
        return 0;
    out[n] = '\0';
    *p = s + 1;
    return 1;
}

static const char *known(const char *name)
{
    if (strcmp(name, "pong") == 0 || strcmp(name, "paddle") == 0)
        return "pong";
    if (strcmp(name, "space-shooter") == 0)
        return "space-shooter";
    if (strcmp(name, "arkaled") == 0)
        return "arkaled";
    if (strcmp(name, "text-demo") == 0)
        return "text-demo";
    return NULL;
}

const char *cart_id(const char *json)
{
    char key[32];
    char value[32];
    const char *p;

    if (json == NULL)
        return NULL;
    p = json;
    while (*p != '\0') {
        skip(&p);
        if (*p != '"') {
            if (*p == '\0')
                break;
            p++;
            continue;
        }
        if (!read_string(&p, key, (int)sizeof key)) {
            p++;
            continue;
        }
        skip(&p);
        if (*p != ':' || strcmp(key, "cart") != 0)
            continue;
        p++;
        skip(&p);
        if (!read_string(&p, value, (int)sizeof value))
            return NULL;
        return known(value);
    }
    return NULL;
}

static void select_name(const char *name)
{
    if (name == NULL)
        which = 0;
    else if (strcmp(name, "pong") == 0 || strcmp(name, "paddle") == 0)
        which = 2;
    else if (strcmp(name, "space-shooter") == 0)
        which = 1;
    else if (strcmp(name, "arkaled") == 0)
        which = 3;
    else if (strcmp(name, "text-demo") == 0)
        which = 4;
    else
        which = 0;
}

int cart_read_file(const char *path)
{
    FILE *f;
    char buf[512];
    size_t n;

    f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = '\0';
    select_name(cart_id(buf));
    return which != 0;
}

int cart_loaded(void)
{
    return which != 0;
}

const char *cart_name(void)
{
    if (which == 1)
        return "space-shooter";
    if (which == 2)
        return "pong";
    if (which == 3)
        return "arkaled";
    if (which == 4)
        return "text-demo";
    return NULL;
}

const char *cart_eeprom(void)
{
    if (which == 3)
        return "tools/sim/arkaled.eep";
    return NULL;
}

void cart_reset(void)
{
    if (which == 1)
        game_reset();
    else if (which == 2)
        pong_reset();
    else if (which == 3)
        arkaled_reset();
    else if (which == 4)
        text_demo_reset();
}

void cart_tick(const Input *in)
{
    if (which == 1)
        game_tick(in);
    else if (which == 2)
        pong_tick(in);
    else if (which == 3)
        arkaled_tick(in);
    else if (which == 4)
        text_demo_tick(in);
}
