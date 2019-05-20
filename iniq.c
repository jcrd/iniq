/* This project is licensed under the New BSD License (see LICENSE). */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "inih/ini.h"

#ifndef VERSION
#define VERSION ""
#endif /* VERSION */

#define NO_SECTION "."
#define streq(s1, s2) (strcmp((s1), (s2)) == 0)

struct pair {
    const char *key;
    const char *value;
    struct pair *next;
};

struct section {
    const char *name;
    struct pair *pairs;
    struct section *next;
};

static int quiet = 0;
static int exclude_default = 0;
static char *path_dup = NULL;
static struct section *sections = NULL;

static void
die(const char *fmt, ...)
{
    va_list ap;

    if (!quiet) {
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }

    exit(EXIT_FAILURE);
}

static void
free_section(struct section *s)
{
    struct pair *head = s->pairs;

    while (head) {
        struct pair *p = head;
        head = head->next;

        free((void *)p->key);
        free((void *)p->value);
        free(p);
    }

    free((void *)s->name);
    free(s);
}

static void
cleanup(void)
{
    struct section *head = sections;

    while (head) {
        struct section *s = head;
        head = head->next;
        free_section(s);
    }

    free(path_dup);
}

static int
quote_str(const char *str, char **out)
{
    size_t len = strlen(str);

    // do not quote string if it has no spaces or is already quoted
    if (!str || !strstr(str, " ") ||
        (str[0] == '\'' && str[len-1] == '\'') ||
        (str[0] == '"' && str[len-1] == '"')) {
        *out = (char *)str;
        return 0;
    }

    // +1 for \0, +2 for both quotes
    len += 3;

    *out = malloc(len * sizeof(char));
    snprintf(*out, len, "'%s'", str);

    return 1;
}

static void
print_pair(const char *fmt, struct pair *p, int keys)
{
    char *key, *val;
    int qkey = quote_str(p->key, &key);
    int qval = quote_str(p->value, &val);
    char *k, *v;

    if (!fmt) {
        if (keys)
            printf("%s\n", key);
        else
            printf("%s=%s\n", key, val);
        if (qkey) free(key);
        if (qval) free(val);
        return;
    }

    fmt = strdup(fmt);
    k = strstr(fmt, "%k");
    v = strstr(fmt, "%v");

    if (!k && (keys || !v)) {
        free((void *)fmt);
        if (qkey) free(key);
        if (qval) free(val);
        die("invalid format string: use %%k for key and %%v for value\n");
    }

    // replace the 'k' or 'v' following % with 's' in printf format string
    if (k)
        *(++k) = 's';
    if (v)
        *(++v) = 's';

    if (keys && !v)
        printf(fmt, key);
    // if k's position is greater than v's in fmt, swap their order in printf
    else if ((!k && v) || (k && v && k > v))
        printf(fmt, val, key);
    else
        printf(fmt, key, val);
    printf("\n");

    free((void *)fmt);
    if (qkey) free(key);
    if (qval) free(val);
}

static int
print_pairs(const char *fmt, struct section *s, struct section *d, int keys)
{
    int i = 0;

    if (d) {
        // print keys inherited from DEFAULT if key is not redefined in section
        for (struct pair *dp = d->pairs; dp; dp = dp->next, i++) {
            for (struct pair *sp = s->pairs; sp; sp = sp->next) {
                if (streq(dp->key, sp->key))
                    goto outer;
            }
            print_pair(fmt, dp, keys);
        outer: continue;
        }
    }

    for (struct pair *p = s->pairs; p; p = p->next, i++)
        print_pair(fmt, p, keys);

    return i;
}

static int
print_sections(const char *fmt)
{
    if (fmt && !strstr(fmt, "%s"))
        die("invalid format string: use %%s for section\n");

    int i = 0;

    for (struct section *s = sections; s; s = s->next, i++) {
        if (!strlen(s->name))
            continue;
        printf(fmt ? fmt : "%s", s->name);
        printf("\n");
    }

    return i;
}

static int
print_value(const char *fmt, struct section *s, const char *key)
{
    if (!s)
        return 0;

    for (struct pair *p = s->pairs; p; p = p->next) {
        if (streq(p->key, key)) {
            print_pair(fmt ? fmt : "%v", p, 0);
            return 1;
        }
    }

    return 0;
}

static int
handler(void *user, const char *section, const char *key, const char *value)
{
    // unused user data
    (void)user;

    if (exclude_default && streq(section, "DEFAULT"))
        return 1;

    struct section *s;

    for (s = sections; s; s = s->next) {
        if (streq(s->name, section))
            break;
    }

    if (!s) {
        s = malloc(sizeof(struct section));
        s->name = strdup(section);
        s->pairs = NULL;
        s->next = NULL;

        if (sections) {
            struct section *tail = sections;
            // append so sections are in config order
            while (tail->next)
                tail = tail->next;
            tail->next = s;
        } else {
            sections = s;
        }
    }

    struct pair *p = malloc(sizeof(struct pair));
    p->key = strdup(key);
    p->value = strdup(value);
    p->next = NULL;

    if (s->pairs) {
        struct pair *tail = s->pairs;
        while (tail->next)
            tail = tail->next;
        tail->next = p;
    } else {
        s->pairs = p;
    }

    return 1;
}

static struct section *
get_section(const char *name)
{
    // keys with no section are stored under "" section in inih
    if (streq(name, NO_SECTION))
        name = "";

    for (struct section *s = sections; s; s = s->next) {
        if (streq(s->name, name))
            return s;
    }

    return NULL;
}

static void
print_usage(int code)
{
    fputs("usage: iniq [options] [FILE]\n"
          "\n"
          "With no FILE, read standard input.\n"
          "\n"
          "options:\n"
          "  -h          Show help message\n"
          "  -q          Suppress error messages\n"
          "  -d          Exclude DEFAULT section from output\n"
          "  -s SEPS     Key/value pair separators (default: '=:')\n"
          "  -m          Parse multi-line entries\n"
          "  -p PATH     Path specifying sections/keys to print\n"
          "  -f FORMAT   Print output according to FORMAT\n"
          "                where %s = section, %k = key, %v = value\n"
          "  -v          Show version\n",
          code ? stderr : stdout);

    exit(code);
}

int
main(int argc, char *argv[])
{
    ini_parser_config c = {
        .seps = NULL,
        .multi = 0,
    };
    const char *path = NULL;
    const char *fmt = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "hqds:mp:f:v")) != -1) {
        switch (opt) {
        case 'h':
            print_usage(EXIT_SUCCESS);
            break;
        case 'q':
            quiet = 1;
            break;
        case 'd':
            exclude_default = 1;
            break;
        case 's':
            c.seps = optarg;
            break;
        case 'm':
            c.multi = 1;
            break;
        case 'p':
            path = optarg;
            break;
        case 'f':
            fmt = optarg;
            break;
        case 'v':
            printf("%s\n", VERSION);
            exit(EXIT_SUCCESS);
        }
    }

    atexit(cleanup);

    if (optind < argc) {
        if (ini_parse(argv[optind], handler, c, NULL) < 0)
            die("failed to parse %s\n", argv[optind]);
    } else if (!feof(stdin)) {
        if (ini_parse_file(stdin, handler, c, NULL) < 0)
            die("failed to parse stdin\n");
    } else {
        print_usage(2);
    }

    struct section *s = NULL;
    struct section *d = NULL;
    const char *section = NULL;
    const char *key = NULL;
    int keys = 0;

    if (path) {
        char *p = path_dup = strdup(path);

        if ((section = strsep(&p, ".")) && streq(section, "")) {
            // path doesn't specify section
            section = NO_SECTION;
            // only real sections inherit DEFAULT section
            exclude_default = 1;

            /* anticipate a blank key. if the next char is not ., the path is
               either . (keys is reverted to 0 below) or specifies a key (keys
               is ignored). if path is .., keys will be set to 1 below */
            if (*p != '.')
                keys = 1;
        }

        // key will be blank when path is . or ..
        if ((key = strsep(&p, ".")) && streq(key, "")) {
            // path doesn't specify key
            key = NULL;
            keys = !keys;
        }
    }

    if (section && !(s = get_section(section)))
        die("section %s not found\n", section);

    if (!exclude_default)
        d = get_section("DEFAULT");

    if (key) {
        if (!print_value(fmt, s, key)) {
            if (section) {
                if (!d || !print_value(fmt, d, key))
                    die("key %s not found in section %s\n", key, section);
            } else {
                die("key %s not found\n", key);
            }
        }
    } else if (section) {
        print_pairs(fmt, s, d, keys);
    } else if (!print_sections(fmt)) {
        die("ini has no sections\n");
    }
}
