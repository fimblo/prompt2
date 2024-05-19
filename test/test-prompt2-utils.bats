#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats


# Binary to test
TEST_FUNCTION="$BATS_TEST_DIRNAME/../bin/test-prompt2-utils"


# --------------------------------------------------
@test "trim() removes enclosing whitespace" {
  # Given
  # - a string with leading/trailing whitespace
  word='test'

  # When we test 
  run -0 $TEST_FUNCTION trim "  $word  "

  # Then
  # - the string should be trimmed
  test "$output" = "$word"
}

# --------------------------------------------------
@test "trim() does nothing if no whitespace" {
  # Given
  # - a string with leading/trailing whitespace
  word='test'

  # When we test 
  run -0 $TEST_FUNCTION trim $word

  # Then
  # - the string should be trimmed
  test "$output" = "$word"
}

# --------------------------------------------------
@test "spacefiller() correctly creates a whitespace string" {
  # Given
  number_of_spaces=6

  # When we test 
  run -0 $TEST_FUNCTION spacefiller $number_of_spaces

  # Then
  # - we should get a string with 6 spaces in it
  test "$output" = "      "
}

# --------------------------------------------------
@test "spacefiller() with 0 spaces works" {
  # Given
  number_of_spaces=0

  # When we test 
  run -0 $TEST_FUNCTION spacefiller $number_of_spaces

  # Then
  # - we should get an empty string
  test "$output" = ""
}

# --------------------------------------------------
@test "spacefiller() with negative arg fails " {
  skip "This fails. FIXME"
  # Given
  number_of_spaces=-1

  # When we test 
  run -0 $TEST_FUNCTION spacefiller $number_of_spaces
  
  # Then
  # - we should get an empty string
  test "$output" = ""
}

# --------------------------------------------------
@test "to_lower() with upcase arg works" {
  # Given
  string="ALOHA"

  # When we test 
  run -0 $TEST_FUNCTION to_lower $string

  # Then
  # - we should get a lowercase version
  test "$output" = "aloha"
}

# --------------------------------------------------
@test "to_lower() with lowcase arg does nothing" {
  # Given
  string="aloha"

  # When we test 
  run -0 $TEST_FUNCTION to_lower $string

  # Then
  # - it should do nothing
  test "$output" = $string
}

# --------------------------------------------------
@test "to_lower() with empty string arg does nothing" {
  # Given an empty string
  string=""

  # When we test 
  run -0 $TEST_FUNCTION to_lower "$string"

  # Then
  # - it should do nothing
  test "$output" = "$string"
}

# --------------------------------------------------
@test "safe_strcat() does what it should on happy path" {
  # Given two strings
  string1="foo"
  string2="bar"

  # When we test 
  run -0 $TEST_FUNCTION safe_strcat "$string1" "$string2" 10

  # Then
  # - the two string should be joined
  test "$output" = "${string1}${string2}"
}

# --------------------------------------------------
@test "safe_strcat() errors if the resulting string is larger than allowed" {
  # Given two strings
  string1="foo"
  string2="bar"

  # When we test 
  run -1 $TEST_FUNCTION safe_strcat "$string1" "$string2" 5

  # Then
  # - the two string should be joined
  test $status -eq 1
}

# --------------------------------------------------
@test "truncate_with_ellipsis() truncates to the right length" {
  # Given a string and a limit
  string="01234567890123456789"
  limit=10

  # When we test 
  run -0 $TEST_FUNCTION truncate_with_ellipsis "$string" $limit

  # Then
  # - the string should be ten chars long
  test ${#output} -eq $limit
}

# --------------------------------------------------
@test "truncate_with_ellipsis() truncates with an ellipsis" {
  # Given a string and a limit
  string="01234567890123456789"
  limit=10

  # When we test 
  run -0 $TEST_FUNCTION truncate_with_ellipsis "$string" $limit

  # Then
  # - the last three chars should be '...'
  [[ "${output: -3}" == '...' ]]
}

# --------------------------------------------------
@test "truncate_with_ellipsis() with a limit of zero returns the empty string" {
  # Given a string and a limit
  string="01234567890123456789"
  limit=0

  # When we test 
  run -0 $TEST_FUNCTION truncate_with_ellipsis "$string" $limit

  # Then
  # - the output should be the empty string
  test "$output" = ""
}

# --------------------------------------------------
@test "replace_literal_newline() replaces \n with a newline correctly" {
  # Given a string with a newline sequence in it

  # When we test 
  run -0 $TEST_FUNCTION replace_literal_newlines 'first\nsecond'

  # Then
  # - the output should have a literal newline character in it
  test "$output" = $'first\nsecond'
}

# --------------------------------------------------
@test "replace_literal_newline() does nothing to a newline-free string" {
  # Given a string with a newline sequence in it

  # When we test 
  run -0 $TEST_FUNCTION replace_literal_newlines 'no-newlines'

  # Then
  # - the output should have a literal newline character in it
  test "$output" = 'no-newlines'
}

# --------------------------------------------------
@test "has_nonexpanding_tokens() returns true (widget at start)" {
  # Given a string with a widget in in
  string='@{foo}@{CWD}@{SPC}lalala'

  # When we test 
  run $TEST_FUNCTION has_nonexpanding_tokens "$string"

  # Then
  # - it should return 0
  test $output = 0
}

# --------------------------------------------------
@test "has_nonexpanding_tokens() returns true (widget in middle)" {
  # Given a string with a widget in in
  string='asdf@{CWD}@{foo}@{SPC}lalala'

  # When we test 
  run $TEST_FUNCTION has_nonexpanding_tokens "$string"

  # Then
  # - it should return 0
  test $output = 0
}

# --------------------------------------------------
@test "has_nonexpanding_tokens() returns true (widget at end)" {
  # Given a string with a widget in in
  string='asdf@{CWD}@{SPC}lalala@{foo}'

  # When we test 
  run $TEST_FUNCTION has_nonexpanding_tokens "$string"

  # Then
  # - it should return 0
  test $output = 0
}

# --------------------------------------------------
@test "has_nonexpanding_tokens() returns false (no widget)" {
  # Given a string with no non-expanding widgets in in
  string='asdlalala'

  # When we test 
  run -0 $TEST_FUNCTION has_nonexpanding_tokens "$string"


  # Then
  # - it should return 1
  test $output = 1
}

# --------------------------------------------------
@test "has_nonexpanding_tokens() returns false (only expanding widgets)" {
  # Given a string with a widget in in
  string='asdl@{CWD}@{SPC}alala'

  # When we test 
  run -0 $TEST_FUNCTION has_nonexpanding_tokens "$string"

  # Then
  # - it should return 1
  test $output = 1
}

# --------------------------------------------------
@test "remove_widget_token() successfully removes widget" {
  # Given a string with a widget in in
  string='1@{FOFO}2'

  # When we test 
  run -0 $TEST_FUNCTION remove_widget_token $string '@{FOFO}'

  # Then
  # - it should return the string sans the token (global)
  test $output = '12'
}

# --------------------------------------------------
@test "remove_widget_token() successfully removes widgets (plural)" {
  # Given a string with a widget in in
  string='1@{FOFO}2@{FOFO}'

  # When we test 
  run -0 $TEST_FUNCTION remove_widget_token $string '@{FOFO}'

  # Then
  # - it should return the string sans the token (global)
  test $output = '12'
}

# --------------------------------------------------
@test "remove_widget_token() removes nothing (no widget)" {
  # Given a string without the specified widget
  string='1@{some}2@{string}'

  # When we test 
  run -0 $TEST_FUNCTION remove_widget_token $string '@{FOFO}'

  # Then
  # - it should do nothing to the string
  test $output = $string
}

# --------------------------------------------------
@test "shorten_path() shortens correctly (limit > 3)" {
  # Given a long path
  string='/one/two/three/four/five/six'

  # When we test 
  run -0 $TEST_FUNCTION shorten_path $string 10

  # Then
  # - it should return the right side of the path, shortened
  #   with an ellipsis to 10 characters
  test $output = '...ive/six'
}

# --------------------------------------------------
@test "shorten_path() shortens correctly (limit < 4)" {
  skip "These should return the empty string, but doesn't FIXME"
  # Given a long path
  string='/one/two/three/four/five/six'

  for n in 0 1 2 3 ; do
    # When we test 
    echo "Trying with limit set to '$n'"
    run -0 $TEST_FUNCTION shorten_path $string $n

    # Then
    # - it should return an empty string
    test $output = ''
  done
}

# --------------------------------------------------
@test "are_escape_sequences_properly_formed() passes well-formed sequence" {
  # Given a valid escape sequence
  string='\[\e[0m\]'

  # When we test 
  run -0 $TEST_FUNCTION are_escape_sequences_properly_formed "$string"

  # Then
  # - it should return 0 (success)
  test $output -eq 0
}

# --------------------------------------------------
@test "are_escape_sequences_properly_formed() detects malformed sequence (1)" {
  # Given malformed escape sequences
  # - not closing a bracket
  sequence='\\['

  # When we test 
  run -0 $TEST_FUNCTION are_escape_sequences_properly_formed "$sequence"

  # Then
  # - it should return -1 (error)
  test "$output" -eq -1
}

# --------------------------------------------------
@test "are_escape_sequences_properly_formed() detects malformed sequence (2)" {
  # Given malformed escape sequences
  # - closing a bracket without opening one
  sequence='\\]' 

  # When we test 
  output=$($TEST_FUNCTION are_escape_sequences_properly_formed "$sequence" 2> /dev/null)
  exitcode=$?
  
  # Then
  # - it should return -1 (error) and exit 0
  test "$output" -eq -1
  test $exitcode -eq 0
}

# --------------------------------------------------
@test "are_escape_sequences_properly_formed() detects malformed sequence (3)" {
  # Given malformed escape sequences
  # - opening a bracket before closing the previous one
  sequence='\\[foo\\['

  # When we test 
  output=$($TEST_FUNCTION are_escape_sequences_properly_formed "$sequence" 2> /dev/null)
  exitcode=$?

  # Then
  # - it should return -1 (error) and exit 0
  test "$output" -eq -1
  test $exitcode -eq 0
}

# --------------------------------------------------
@test "are_escape_sequences_properly_formed() detects malformed sequence (4)" {
  # Given malformed escape sequences
  # - escape sequence outside of brackets
  sequence='\\e[0m'

  # When we test 
  output=$($TEST_FUNCTION are_escape_sequences_properly_formed "$sequence" 2> /dev/null)
  exitcode=$?

  # Then
  # - it should return -1 (error) and exit 0
  test "$output" -eq -1
  test $exitcode -eq 0
}

# --------------------------------------------------
@test "count_visible_chars() should count correctly (no invisibles)" {
  # Given 
  # - a string with no invisible chars
  string='hello'

  # When we test 
  run -0 $TEST_FUNCTION count_visible_chars  $string

  # Then
  # - it should return 5
  test "$output" -eq 5
}
    
# --------------------------------------------------
@test "count_visible_chars() should count correctly (escape sequence)" {
  # Given 
  # - a string with an escape sequence in it
  string="\\[\\e[0m\\]hello"

  # When we test 
  run -0 $TEST_FUNCTION count_visible_chars  "$string"

  echo $output
  # Then
  # - it should return 5
  test "$output" -eq 5
}
    
# --------------------------------------------------
@test "count_visible_chars() should count correctly (widget)" {
  # Given 
  # - a string with an escape sequence in it
  string="@{SYS}hello"

  # When we test 
  run -0 $TEST_FUNCTION count_visible_chars  "$string"

  echo $output
  # Then
  # - it should return 5
  test "$output" -eq 5
}

# --------------------------------------------------
@test "count_visible_chars() should count correctly (attribute)" {
  # Given 
  # - a string with an escape sequence in it
  string="%{fg blue}hello"

  # When we test 
  run -0 $TEST_FUNCTION count_visible_chars  "$string"

  echo $output
  # Then
  # - it should return 5
  test "$output" -eq 5
}
    

# --------------------------------------------------
@test "term_width correctly states the width of terminal" {
  # Given 
  # - we are in a terminal
  if ! tty-s ; then
    skip "Terminal width is not determinable in this environment"
  fi

  # When we test 
  run -0 $TEST_FUNCTION term_width

  echo $output
  # Then
  # - it should return 5
  test "$output" -eq $(tput cols)
}
    
