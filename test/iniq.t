#!/bin/sh

test_description='Test iniq'

. ./sharness/sharness.sh

cd "$SHARNESS_TEST_DIRECTORY" || exit 1

test_expect_success 'List sections in file' '
test "$(iniq test.conf)" = "section1"
'

test_expect_success 'List sections from stdin' '
test "$(cat test.conf | iniq)" = "section1"
'

test_expect_success 'Get keys from section' '
test "$(iniq -p section1.keyA test.conf)" = "a" &&
test "$(iniq -p section1.keyB test.conf)" = "b"
'

test_expect_success 'Get keys inherited from DEFAULT' '
test "$(iniq -p section1.default test.conf)" = "true"
'

test_expect_success 'Include DEFAULT in section list' '
test "$(iniq -d test.conf)" = "DEFAULT
section1"
'

test_expect_success 'Disable DEFAULT' '
test "$(iniq -D -p section1. test.conf)" = "keyA
keyB"
'

test_expect_success 'Get keys not in any section' '
test "$(iniq -p .. test.conf)" = "no_section
free" &&
test "$(iniq -p .no_section test.conf)" = "true"
'

test_expect_success 'Format section output' '
test "$(iniq -d -f ^%s$ test.conf)" = "^DEFAULT$
^section1$"
'

test_expect_success 'Format key/value output' '
test "$(iniq -D -f ^%k$ -p section1 test.conf)" = "^keyA$
^keyB$" &&
test "$(iniq -D -f ^%v$ -p section1 test.conf)" = "^a$
^b$" &&
test "$(iniq -D -f %v:%k -p section1 test.conf)" = "a:keyA
b:keyB"
'

test_expect_success 'List sections with the same name' '
test "$(iniq multi.conf)" = "multi
multi"
'

test_expect_success 'Combine sections with the same name' '
test "$(iniq -c -p multi. multi.conf)" = "default1
default2
key1
key2
key3"
'

test_expect_success 'Get number of sections' '
test "$(iniq -p multi -n multi.conf)" = "2"
'

test_expect_success 'Get section by index' '
test "$(iniq -D -p multi. -i 0 multi.conf)" = "key1" &&
test "$(iniq -D -p multi. -i 1 multi.conf)" = "key2
key3"
'

test_expect_success 'Get keys with custom separators' '
test "$(iniq -s "!$" -f %v -p custom-separators custom.conf)" = "1
2"
'

test_expect_success 'Get indented lines' '
test "$(iniq -p indented indented.conf)" = "line1=1
line2=2"
'

test_expect_success 'Get multi-line entry' '
test "$(iniq -p indented -m indented.conf)" = "line1=1
line1=line2=2"
'

test_expect_success 'Escape section name' '
test "$(iniq -p escape\\.this\\.section.key escape.conf)" = "true"
'

test_expect_success 'Use alternative path separator' '
test "$(iniq -P , -p escape.this.section,key escape.conf)" = "true"
'

test_expect_success 'Output all' '
test "$(iniq -o test.conf)" = "section= default=true no_section=true free=1
section=section1 default=true keyA=a keyB=b"
'

test_expect_success 'Output all (include DEFAULT)' '
test "$(iniq -o -d test.conf)" = "section= default=true no_section=true free=1
section=DEFAULT default=true
section=section1 default=true keyA=a keyB=b"
'

test_expect_success 'Output all (with format)' '
test "$(iniq -f "%k:%v" -o -D test.conf)" = "section: no_section:true free:1
section:section1 keyA:a keyB:b"
'

test_expect_success 'Output all (multi)' '
test "$(iniq -o -D multi.conf)" = "section=multi key1=1
section=multi key2=2 key3=3"
'

test_expect_success 'Output all (keyless)' '
test "$(iniq -o -d keyless.conf)" = "section=DEFAULT inherited=true
section=keyless inherited=true"
'

test_expect_success 'Output all (keyless, disable DEFAULT)' '
test "$(iniq -o -D keyless.conf)" = "section=keyless"
'

test_done

# vim: ft=sh
