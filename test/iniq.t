#!/bin/sh

test_description='Test iniq'

. ./sharness/sharness.sh

CONF="$SHARNESS_TEST_DIRECTORY"/test.conf

test_expect_success 'List sections in file' '
test "$(iniq $CONF)" = "DEFAULT
section1
multi
indented"
'

test_expect_success 'List sections from stdin' '
test "$(cat $CONF | iniq)" = "DEFAULT
section1
multi
indented"
'

test_expect_success 'Get keys from section' '
test "$(iniq -p section1.keyA $CONF)" = "a" &&
test "$(iniq -p section1.keyB $CONF)" = "b"
'

test_expect_success 'Get keys inherited from DEFAULT' '
test "$(iniq -p section1.default $CONF)" = "true"
'

test_expect_success 'Exclude DEFAULT' '
test "$(iniq -d $CONF)" = "section1
multi
indented" &&
test "$(iniq -d -p section1. $CONF)" = "keyA
keyB"
'

test_expect_success 'Get section with multiple declarations' '
test "$(iniq -d -p multi. $CONF)" = "key1
key2"
'

test_expect_success 'Get keys not in any section' '
test "$(iniq -p .. $CONF)" = "no_section" &&
test "$(iniq -p .no_section $CONF)" = "true"
'

test_expect_success 'Format section output' '
test "$(iniq -d -f ^%s$ $CONF)" = "^section1$
^multi$
^indented$"
'

test_expect_success 'Format key/value output' '
test "$(iniq -d -f ^%k$ -p section1 $CONF)" = "^keyA$
^keyB$" &&
test "$(iniq -d -f ^%v$ -p section1 $CONF)" = "^a$
^b$" &&
test "$(iniq -d -f %v:%k -p section1 $CONF)" = "a:keyA
b:keyB"
'

test_expect_success 'Get keys with custom separators' '
test "$(iniq -s "!$" -f %v -p custom-separators $CONF)" = "1
2"
'

# inih must be built with INI_ALLOW_MULTILINE=0
test_expect_success 'Get indented lines' '
test "$(iniq -d -p indented $CONF)" = "line1=1
line2=2"
'

test_done
