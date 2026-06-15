#!/usr/bin/env perl

use strict;

my $line;
printf "const char %s[] = {\n   ", $ARGV[0];
while($line = <STDIN>) {
   my $i = 0;
   my $len = length($line);
   while($i < $len) {
      printf "0x%02x, ", ord(substr($line, $i, 1));
      ++$i;
      if($i % 16 == 0 && $i < $len) {
         printf "\n   ";
      }
   }
   printf "\n   ";
}
printf "0x00 };\n";
