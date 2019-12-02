# iniq [![CircleCI](https://circleci.com/gh/jcrd/iniq.svg?style=svg)](https://circleci.com/gh/jcrd/iniq)

**iniq** is a simple INI file reader for the command line.
It queries an INI file based on the path <_section_><_separator_><_key_> and
allows use of custom separators in the file and formatting of the output.
Sections inherit keys from a special DEFAULT section unless the _-d_ flag is
used.
See below for examples.

## Usage

```
usage: iniq [options] [FILE]

With no FILE, read standard input.

options:
  -h          Show help message
  -q          Suppress error messages
  -d          Exclude DEFAULT section from output
  -s SEPS     Key/value pair separators (default: '=:')
  -m          Parse multi-line entries
  -c          Combine sections with the same name
  -P SEP      Path separator character (default: '.')
  -p PATH     Path specifying sections/keys to print
  -i NUM      Index of section in PATH
  -f FORMAT   Print output according to FORMAT
                where %s = section, %k = key, %v = value
  -v          Show version
```

### Example commands

Given the configuration file _example.conf_:
```
in_section=false
[DEFAULT]
default=true
[section1]
key1=value1
[example.com]
key2=value2
```

Print sections in a file:
```
$ iniq example.conf
DEFAULT
section1
example.com
```

Exclude DEFAULT section:
```
$ iniq -d example.conf
section1
example.com
```

Print sectionless key/value pairs:
```
$ iniq -p . example.conf
in_section=false
```

Print sectionless keys:
```
$ iniq -p .. example.conf
in_section
```

Print value of sectionless key:
```
$ iniq -p .in_section example.conf
false
```

Print key/value pairs in section1, excluding DEFAULT:
```
$ iniq -d -p section1 example.conf
key1=value1
```

Print keys in section1, excluding DEFAULT:
```
$ iniq -d -p section1. example.conf
key1
```

Print value of key1 in section1:
```
$ iniq -p section1.key1 example.conf
value1
```

Use alternative path separator:
```
$ iniq -P , -p example.com,key2 example.conf
value2
```

Print values followed by their keys in section1, using the format _%v:%k_:
```
$ iniq -p section1 -f %v:%k example.conf
true:default
value1:key1
```

Print only the values in section1, using the format _%v_:
```
$ iniq -p section1 -f %v example.conf
true
value1
```

Escape section name containing dots:
```
$ iniq -d -p example\\.com example.conf
key2=value2
```

Configuration files may contain sections with the same name.

Given the configuration file _multi.conf_:
```
[test]
section0=0
[test]
section1=1
```

Print sections:
```
$ iniq multi.conf
test
test
```

Combine sections with the same name:
```
$ iniq -c -p test. multi.conf
section0
section1
```

Get section by index:
```
$ iniq -p test. -i 1 multi.conf
section1
```

## Used by

* [passless](https://github.com/jcrd/passless)
* [hdparmify](https://github.com/jcrd/hdparmify)
* [auto-inhibit](https://github.com/jcrd/auto-inhibit)

## License

This project is licensed under the New BSD License (see [LICENSE](LICENSE)).

## Acknowledgements

* Built with [inih](https://github.com/benhoyt/inih)
