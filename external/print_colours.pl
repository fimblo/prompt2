#!/usr/bin/env perl
#
# This script visualises colour names alongside their RGB values in
# the terminal.

use FindBin;
use strict;
use warnings;

my $script_dir = $FindBin::Bin;
my $rgb_file = "$script_dir/rgb.txt";

open(my $fh, '<', $rgb_file) or die "Cannot open $rgb_file: $!";

printf(" R   G   B   FG (black bg)        FG (white bg)        BG (black fg)        BG (white fg)\n");
while (<$fh>) {
  chomp;
  s/^\s+//g;
  my @words = map { lc } split /[ \t]+/;
  next if @words > 4;           # skip multi-word colours

  # print rgb colour values
  printf("%3d %3d %3d ", @words[0,1,2]);


  # print colour name in that colour on black background
  printf("%s[38;2;%d;%d;%dm%s[48;2;0;0;0m %-20s%s[0m",
         chr(27),               # esc
         @words[0,1,2],         # r, g, and b
         chr(27),
         $words[3],             # colour name
         chr(27));

  # print colour name in that colour on white background
  printf("%s[38;2;%d;%d;%dm%s[48;2;255;255;255m %-20s%s[0m",
         chr(27),               # esc
         @words[0,1,2],         # r, g, and b
         chr(27),
         $words[3],             # colour name
         chr(27));

  # print colour name in black on colour background
  printf("%s[38;2;0;0;0m%s[48;2;%d;%d;%dm %-20s%s[0m",
         chr(27),               # esc
         chr(27),
         @words[0,1,2],         # r, g, and b
         $words[3],             # colour name
         chr(27));

  # print colour name in white on colour background
  printf("%s[38;2;255;255;255m%s[48;2;%d;%d;%dm %-20s%s[0m\n",
         chr(27),               # esc
         chr(27),
         @words[0,1,2],         # r, g, and b
         $words[3],             # colour name
         chr(27));
}
