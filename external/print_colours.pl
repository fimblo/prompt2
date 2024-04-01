#!/usr/bin/perl
printf(" R   G   B   FG                   BG\n");
while (<>) {
  chomp;
  s/^\s+//g;
  my @words = map { lc } split /[ \t]+/;
  next if @words > 4;
  printf("%3d %3d %3d ", @words[0,1,2]);
  printf("%s[38;2;%d;%d;%dm%s[48;2;0;0;0m %-20s%s[0m", chr(27), @words[0,1,2], chr(27),$words[3],chr(27));
  my @rev = map { 255 - $_ } @words[0,1,2];
  printf("%s[38;2;%d;%d;%dm%s[48;2;%d;%d;%dm %-20s%s[0m\n", chr(27), @rev, chr(27), @words ,chr(27));
}
