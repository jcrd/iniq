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
#define DEFAULT_SECTION "DEFAULT"
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
static int include_default = 0;
static int disable_default = 0;
static int combine_sections = 0;
static int number_sections = 0;
static char *path_sep = ".";
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
print_pair(const char *fmt, struct pair *p, int keys, int sep)
{
    char *key, *val;
    int qkey = quote_str(p->key, &key);
    int qval = quote_str(p->value, &val);
    char *k, *v;

    if (!fmt) {
        if (keys)
            printf("%s", key);
        else
            printf("%s=%s", key, val);
        if (sep > 0)
            printf("%c", sep);
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
    if (sep > 0)
        printf("%c", sep);

    free((void *)fmt);
    if (qkey) free(key);
    if (qval) free(val);
}

static int
print_pairs(const char *fmt, struct section *s, struct section *d, int keys,
        int sep, int dry_run)
{
    int i = 0;

    if (d) {
        // print keys inherited from DEFAULT if key is not redefined in section
        for (struct pair *dp = d->pairs; dp; dp = dp->next) {
            for (struct pair *sp = s->pairs; sp; sp = sp->next) {
                if (streq(dp->key, sp->key))
                    goto outer;
            }
            if (!dry_run) {
                if (i > 0)
                    printf("%c", sep);
                print_pair(fmt, dp, keys, -1);
            }
            i++;
        outer: continue;
        }

        if (!dry_run && i > 0 && s->pairs)
            printf("%c", sep);
    }

    for (struct pair *p = s->pairs; p; p = p->next, i++)
        if (!dry_run)
            print_pair(fmt, p, keys, p->next ? sep : -1);

    return i;
}

static int
print_sections(const char *fmt)
{
    if (fmt && !strstr(fmt, "%s"))
        die("invalid format string: use %%s for section\n");

    int i = 0;

    for (struct section *s = sections; s; s = s->next, i++) {
        if (!strlen(s->name) ||
                (!include_default && streq(s->name, DEFAULT_SECTION)))
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
            print_pair(fmt ? fmt : "%v", p, 0, '\n');
            return 1;
        }
    }

    return 0;
}

static int
print_output(const char *fmt, struct section *d)
{
    int i = 0;

    for (struct section *s = sections; s; s = s->next, i++) {
        if (!include_default && streq(s->name, DEFAULT_SECTION))
            continue;
        struct pair p = {"section", s->name, NULL};
        print_pair(fmt, &p, 0, -1);
        // dry run to count keys
        if (print_pairs(fmt, s, d, 0, -1, 1))
            printf("%c", ' ');
        print_pairs(fmt, s, d, 0, ' ', 0);
        printf("\n");
    }

    return i;
}

static int
handler(void *user, const char *section, const char *key, const char *value)
{
    // unused user data
    (void)user;

    int default_section = streq(section, DEFAULT_SECTION);
    struct section *s = NULL;
    struct section *n;

    if (disable_default && default_section)
        return 1;

    for (n = sections; n; n = n->next) {
        if (streq(n->name, section))
            s = n;
    }

    if (!s || !(key || default_section || combine_sections)) {
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

    if (!key)
        return 1;

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
get_section(const char *name, unsigned int i)
{
    // keys with no section are stored under "" section in inih
    if (streq(name, NO_SECTION))
        name = "";

    for (struct section *s = sections; s; s = s->next) {
        if (streq(s->name, name) && i-- == 0)
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
          "  -d          Include DEFAULT section in section list\n"
          "  -D          Disable inheriting of DEFAULT section\n"
          "  -s SEPS     Key/value pair separators (default: '=:')\n"
          "  -m          Parse multi-line entries\n"
          "  -c          Combine sections with the same name\n"
          "  -P SEP      Path separator character (default: '.')\n"
          "  -p PATH     Path specifying sections/keys to print\n"
          "  -n          Get number of sections with the name given in PATH\n"
          "  -i NUM      Index of section in PATH\n"
          "  -f FORMAT   Print output according to FORMAT\n"
          "                where %s = section, %k = key, %v = value\n"
          "  -o          Output sections, keys, and values\n"
          "  -v          Show version\n",
          code ? stderr : stdout);

    exit(code);
}

static unsigned int
strtoui(const char *str)
{
    char *endptr;
    unsigned int i = strtoul(str, &endptr, 10);
    if (*endptr != '\0')
        die("invalid integer: %s\n", str);
    return i;
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
    unsigned int section_index = 0;
    unsigned int output = 0;
    int opt;

    while ((opt = getopt(argc, argv, "hqdDs:mcP:p:ni:f:ov")) != -1) {
        switch (opt) {
        case 'h': print_usage(EXIT_SUCCESS); break;
        case 'q': quiet = 1; break;
        case 'd': include_default = 1; break;
        case 'D': disable_default = 1; break;
        case 's': c.seps = optarg; break;
        case 'm': c.multi = 1; break;
        case 'c': combine_sections = 1; break;
        case 'P': path_sep = optarg; break;
        case 'p': path = optarg; break;
        case 'n': number_sections = 1; break;
        case 'i': section_index = strtoui(optarg); break;
        case 'f': fmt = optarg; break;
        case 'o': output = 1; break;
        case 'v': printf("%s\n", VERSION); exit(EXIT_SUCCESS);
        }
    }

    const char *file = argv[optind];

    atexit(cleanup);

    if (optind < argc) {
        if (ini_parse(file, handler, c, NULL) < 0)
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
        size_t len = 0;
        char buf[BUFSIZ];
        char *s;

        while ((s = strsep(&p, path_sep))) {
            if (streq(s, "") && len == 0) {
                // path doesn't specify section
                section = NO_SECTION;
                // only real sections inherit DEFAULT section
                disable_default = 1;

                /* anticipate a blank key. if the next char is not ., the path is
                   either . (keys is reverted to 0 below) or specifies a key (keys
                   is ignored). if path is .., keys will be set to 1 below */
                if (*p != *path_sep)
                    keys = 1;
            } else {
                len += snprintf(buf + len, BUFSIZ - len, "%s", s);
                if (buf[len - 1] == '\\') {
                    buf[len - 1] = *path_sep;
                    if (len < BUFSIZ)
                        continue;
                }
            }
            break;
        }

        if (len > 0)
            section = buf;

        // key will be blank when path is . or ..
        if ((key = strsep(&p, path_sep)) && streq(key, "")) {
            // path doesn't specify key
            key = NULL;
            keys = !keys;
        }
    }

    if (section) {
        if (number_sections) {
            unsigned int i = 0;
            while (get_section(section, i))
                i++;
            printf("%d\n", i);
            exit(i > 0 ? EXIT_SUCCESS : EXIT_FAILURE);
        }
        if (!(s = get_section(section, section_index)))
            die("%s: section '%s' (index %d) not found\n", file, section,
                    section_index);
    }

    if (!disable_default)
        d = get_section(DEFAULT_SECTION, 0);

    if (output)
        exit(print_output(fmt, d) > 0 ? EXIT_SUCCESS : EXIT_FAILURE);

    if (key) {
        if (!print_value(fmt, s, key)) {
            if (section) {
                if (!d || !print_value(fmt, d, key))
                    die("%s: key '%s' not found in section '%s'\n", file, key,
                            section);
            } else {
                die("%s: key '%s' not found\n", file, key);
            }
        }
    } else if (section) {
        print_pairs(fmt, s, d, keys, '\n', 0);
        printf("\n");
    } else if (!print_sections(fmt)) {
        die("%s: no sections\n", file);
    }
}
