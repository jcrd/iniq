#!/bin/sh

test_description='Test iniq'

. ./sharness/sharness.sh

cd "$SHARNESS_TEST_DIRECTORY" || exit 1

test_expect_success 'List sections in file' '
test "$(iniq test.conf)" = "DEFAULT
section1"
'

test_expect_success 'List sections from stdin' '
test "$(cat test.conf | iniq)" = "DEFAULT
section1"
'

test_expect_success 'Get keys from section' '
test "$(iniq -p section1.keyA test.conf)" = "a" &&
test "$(iniq -p section1.keyB test.conf)" = "b"
'

test_expect_success 'Get keys inherited from DEFAULT' '
test "$(iniq -p section1.default test.conf)" = "true"
'

test_expect_success 'Exclude DEFAULT' '
test "$(iniq -d test.conf)" = "section1" &&
test "$(iniq -d -p section1. test.conf)" = "keyA
keyB"
'

test_expect_success 'Get keys not in any section' '
test "$(iniq -p .. test.conf)" = "no_section" &&
test "$(iniq -p .no_section test.conf)" = "true"
'

test_expect_success 'Format section output' '
test "$(iniq -f ^%s$ test.conf)" = "^DEFAULT$
^section1$"
'

test_expect_success 'Format key/value output' '
test "$(iniq -d -f ^%k$ -p section1 test.conf)" = "^keyA$
^keyB$" &&
test "$(iniq -d -f ^%v$ -p section1 test.conf)" = "^a$
^b$" &&
test "$(iniq -d -f %v:%k -p section1 test.conf)" = "a:keyA
b:keyB"
'

test_expect_success 'List sections with the same name' '
test "$(iniq multi.conf)" = "DEFAULT
multi
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
test "$(iniq -d -p multi. -i 1 multi.conf)" = "key2
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

test_done

# vim: ft=sh
