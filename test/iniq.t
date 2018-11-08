#!/bin/sh

test_description='Test iniq'

. ./sharness/sharness.sh

# leave sharness directory
cd ..

SHARNESS_TEST_DIRECTORY="$(pwd)"

test_expect_success 'List sections in file' '
test "$(iniq test.conf)" = "DEFAULT
section1
multi"
'

test_expect_success 'List sections from stdin' '
test "$(cat test.conf | iniq)" = "DEFAULT
section1
multi"
'

test_expect_success 'Get keys from section' '
test "$(iniq -p section1.keyA test.conf)" = "a" &&
test "$(iniq -p section1.keyB test.conf)" = "b"
'

test_expect_success 'Get keys inherited from DEFAULT' '
test "$(iniq -p section1.default test.conf)" = "true"
'

test_expect_success 'Exclude DEFAULT' '
test "$(iniq -d test.conf)" = "section1
multi" &&
test "$(iniq -d -p section1. test.conf)" = "keyA
keyB"
'

test_expect_success 'Get section with multiple declarations' '
test "$(iniq -d -p multi. test.conf)" = "key1
key2"
'

test_expect_success 'Get keys not in any section' '
test "$(iniq -p .. test.conf)" = "no_section" &&
test "$(iniq -p .no_section test.conf)" = "true"
'

test_expect_success 'Format section output' '
test "$(iniq -d -f ^%s$ test.conf)" = "^section1$
^multi$"
'

test_expect_success 'Format key/value output' '
test "$(iniq -d -f ^%k$ -p section1 test.conf)" = "^keyA$
^keyB$" &&
test "$(iniq -d -f ^%v$ -p section1 test.conf)" = "^a$
^b$" &&
test "$(iniq -d -f %v:%k -p section1 test.conf)" = "a:keyA
b:keyB"
'

test_expect_success 'Get keys with custom separators' '
test "$(iniq -s "!$" -f %v -p custom-separators test.conf)" = "1
2"
'

test_done