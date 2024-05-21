#!/usr/bin/env bats  # -*- mode: shell-script -*-
bats_require_minimum_version 1.5.0

# To run a test manually:
# cd path/to/project/root
# bats test/test.bats


# Binary to test
TEST_FUNCTION="$BATS_TEST_DIRNAME/../bin/test-term-attributes"


# --------------------------------------------------
@test "adding and retrieving from the attribute dict (both key and value)" {
  # Given 
  # - a key value pair
  key='foo'
  value='bar'

  # When we test 
  run -0 $TEST_FUNCTION create_attribute_dict "$key" "$value"
  
  # Then
  # - it should return the key-value pair with an arrow between
  test "$output" = 'foo->bar'
}


# --------------------------------------------------
@test "adding and retrieving from the attribute dicts (empty strings)" {
  # Given 
  # - empty key/values
  key=''
  value=''

  # When we test 
  run -0 $TEST_FUNCTION create_attribute_dict "$key" "$value"
  
  # Then
  # - it should return just an arrow because the key and value are the empty string
  test "$output" = '->'
}


# --------------------------------------------------
@test "get_attribute_combo() with valid attribute should succeed" {
  # Given 
  # - an attribute name
  attr='bold'

  # When we test 
  run -0 $TEST_FUNCTION get_attribute_combo "$attr"

  # Then
  # - it should return the terminal escape sequence for bold
  test "$output" = '\[\e[1m\]'
}

# --------------------------------------------------
@test "get_attribute_combo() with a complex attribute should succeed" {
  # Given 
  # - an complex attribute name
  attr='bold,fg-yellow'

  # When we test 
  run -0 $TEST_FUNCTION get_attribute_combo "$attr"

  # Then
  # - it should return the terminal escape sequence for bold
  test "$output" = '\[\e[1;38;2;255;255;0m\]'
}

# --------------------------------------------------
@test "get_attribute_combo() with invalid attribute should fail" {
  # Given 
  # - an invalid attribute name
  attr='potatosareyummy'

  # When we test , then it should return exit code 1
  run -1 $TEST_FUNCTION get_attribute_combo "$attr"
}

# --------------------------------------------------
@test "replace_attribute_tokens() with one valid attribute" {
  # Given 
  # - a valid attribute
  attr='%{bold}'

  # When we test
  run -0 $TEST_FUNCTION replace_attribute_tokens "$attr"

  # Then 
  # - it should translate the attribute to a valid escape sequence
  test "$output" =  '\[\e[1m\]'
}

# --------------------------------------------------
@test "replace_attribute_tokens() with a string containing an attribute should replace it" {
  # Given 
  # - a valid attribute
  attr='%{bold}THIS is a string %{}'

  # When we test
  run -0 $TEST_FUNCTION replace_attribute_tokens "$attr"

  # Then 
  # - it should translate the attribute to a valid escape sequence
  test "$output" =  '\[\e[1m\]THIS is a string \[\e[0m\]'
}

# --------------------------------------------------
@test "replace_attribute_tokens() with a string containing a complex attribute should replace it" {
  # Given 
  # - a valid attribute
  attr='%{bold, fg yellow}THIS is a string %{}'

  # When we test
  run -0 $TEST_FUNCTION replace_attribute_tokens "$attr"

  # Then 
  # - it should translate the attribute to a valid escape sequence
  test "$output" =  '\[\e[1;33m\]THIS is a string \[\e[0m\]'
}

# --------------------------------------------------
@test "replace_attribute_tokens() with a string containing many attributes should replace them all" {
  # Given 
  # - a valid attribute
  attr='%{bold}THIS is a bold string %{}%{dim}THIS is a dim string%{}'

  # When we test
  run -0 $TEST_FUNCTION replace_attribute_tokens "$attr"
  echo $output
  # Then 
  # - it should translate the attribute to a valid escape sequence
  test "$output" =  '\[\e[1m\]THIS is a bold string \[\e[0m\]\[\e[2m\]THIS is a dim string\[\e[0m\]'
}

# --------------------------------------------------
@test "replace_attribute_tokens() should replace broken attributes with 'ERROR'" {
  # Given 
  # - an invalid attribute
  attr='%{boldTHIS is a bold string %{}%{dim}THIS is a dim string%{}'

  # When we test
  run -0 $TEST_FUNCTION replace_attribute_tokens "$attr"
  echo $output
  # Then 
  # - it should translate the attribute to a valid escape sequence
  test "$output" =  'ERROR\[\e[2m\]THIS is a dim string\[\e[0m\]'
}

