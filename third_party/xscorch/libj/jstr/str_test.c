/* $Header: /fridge/cvs/xscorch/libj/jstr/str_test.c,v 1.9 2009-04-26 17:39:30 jacob Exp $ */
/*

   libj - str_test.c             Copyright (C) 2001-2003 Justin David Smith
   justins (a) chaos2.org        http://chaos2.org/

   Tests the jstr functions


   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation, version 2 of the License ONLY.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this library; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

   This file is part of LIBJ.

*/
#define  LIBJ_ALLOW_LIBC_STRING    1
#include <libjstr.h>



int   success  =  0;
int   failure  =  0;



void test_int(const char *msg, int actual, int expected) {

   if(actual == expected) {
      printf("%16s: pass\n", msg);
      ++success;
   } else {
      printf("%16s: fail - expected %d, got %d\n", msg, expected, actual);
      ++failure;
   }

}



void test_ptr(const char *msg, const char *actual, const char *expected) {

   if(actual == expected) {
      printf("%16s: pass\n", msg);
      ++success;
   } else {
      printf("%16s: fail - expected %p, got %p\n", msg, expected, actual);
      ++failure;
   }

}



void test_str(const char *msg, const char *actual, const char *expected) {

   if(strcmp(actual, expected) == 0) {
      printf("%16s: pass\n", msg);
      ++success;
   } else {
      printf("%16s: fail - buffers differ, expected \"%s\", got \"%s\"\n", msg, expected, actual);
      ++failure;
   }

}




int main() {

   char *q;
   const char *p;
   char s1[0x1000];
   char s2[0x1000];
   char s3[0x1000];
   char s4[0x1000];
   rkstate rk;

   /* Test comparison functions */
   test_int("compare1",    strcomp("abcdefg", "abcdefg"), 0);
   test_int("compare2",    strcomp("abcdefg", "abcdefghij"), -1);
   test_int("compare3",    strcomp("abcdefghij", "abcdefg"), +1);
   test_int("compare4",    strcomp("abcdefg", "abceefg"), -1);
   test_int("compare5",    strcompn("abcdefg", "abceefg", 3), 0);
   test_int("compare6",    strcompn("abcdefg", "abceefg", 4), -1);
   test_int("compare7",    strcompn("foo", "bar", 0), 0);
   test_int("compare8",    strcomp("foo", NULL), +1);
   test_int("compare9",    strcomp(NULL, NULL), 0);
   test_int("compare10",   strcompn(NULL, NULL, 100000), 0);
   test_int("equal1",      strequal("abcdefg", "abcdefg"), true);
   test_int("equal2",      strequal("abcdefg", "abcdefghijklm"), false);
   test_int("equal3",      strequaln("abcdefg", "abcdefghijklm", 7), true);
   test_int("equal4",      strequaln("abcdefg", "abcdefghijklm", 8), false);
   test_int("equal5",      strequaln(NULL, "abcdefghijklm", 0), false);
   test_int("equal6",      strequaln(NULL, "abcdefghijklm", 1), false);
   test_int("equal6",      strequaln(NULL, NULL, 0), true);
   test_int("equal7",      strequaln(NULL, NULL, 100000), true);

   /* Test scanning functions */
   p = "abdefghijklmnopabcababc";
   strcpy(s1, p);
   test_int("prescan1",    strequal(s1, p), true);
   test_ptr("scan1",       strscan(s1, NULL), s1);
   test_ptr("scan2",       strscan(s1, "ab"), s1);
   test_ptr("scan3",       strscan(s1, "abc"), s1 + 15);
   test_ptr("scan4",       strscan(s1, "cba"), NULL);
   test_ptr("rkstrpat1a",  rkstrpat(&rk, s1, NULL), s1);
   test_ptr("rkstrpat1b",  rkstrnext(&rk), s1);
   test_ptr("rkstrpat1c",  rkstrnext(&rk), s1);
   test_ptr("rkstrpat2a",  rkstrpat(&rk, s1, "ab"), s1);
   test_ptr("rkstrpat2b",  rkstrnext(&rk), s1 + 15);
   test_ptr("rkstrpat2c",  rkstrnext(&rk), s1 + 18);
   test_ptr("rkstrpat2d",  rkstrnext(&rk), s1 + 20);
   test_ptr("rkstrpat2e",  rkstrnext(&rk), NULL);
   test_ptr("rkstrpat3a",  rkstrpat(&rk, s1, "abc"), s1 + 15);
   test_ptr("rkstrpat3b",  rkstrnext(&rk), s1 + 20);
   test_ptr("rkstrpat3c",  rkstrnext(&rk), NULL);
   test_ptr("rkstrpat4",   rkstrpat(NULL, NULL, s1), NULL);
   test_ptr("rkstrpat5a",  rkstrpat(&rk, s1, NULL), s1);
   test_ptr("rkstrpat5b",  rkstrnext(&rk), s1);
   test_ptr("rkstrpat5c",  rkstrnext(&rk), s1);
   test_ptr("rkstrpat6",   rkstrpat(NULL, NULL, NULL), NULL);
   test_ptr("kmpstrpat1a", (q = kmpstrpat(s1, "abc")), s1 + 15);
   test_ptr("kmpstrpat1b", (q = kmpstrpat(q, "abc")), s1 + 15);
   test_ptr("kmpstrpat1c", (q = kmpstrpat(q + 1, "abc")), s1 + 20);
   test_ptr("kmpstrpat1d", (q = kmpstrpat(q + 1, "abc")), NULL);
   test_ptr("kmpstrpat2",  kmpstrpat(NULL, s1), NULL);
   test_ptr("kmpstrpat3",  kmpstrpat(s1, NULL), s1);

   /* Test copy functions */
   p = "abcdefgdfmbdlfmvlkmvklmvkl;dfvkl;dfkvl;fdmlkv;dsfkmvsdfvmvmkvmkldsfmvklmvlk;mvkl;dfsmvkl;dfskml;sdfvdvmvkml;dfskvl;";
   strcpy(s1, p);
   p = "xxxxxxx";
   strcpy(s2, p);
   strcpy(s3, s2);
   strcat(s3, s1);
   strncpy(s3, s2, strlen(s2));
   test_int("strlenn1",    strlenn(NULL), 0);
   test_int("strlenn2",    strlenn(s1), 115);
   test_int("strlenn3",    strlenn(s2), 7);
   test_int("strlenn4",    strlenn(s3), 122);
   test_int("strnlenn1",   strnlenn(NULL, 100), 0);
   test_int("strnlenn2",   strnlenn(s1, 100), 100);
   test_int("strnlenn3",   strnlenn(s1, 115), 115);
   test_int("strnlenn4",   strnlenn(s1, 150), 115);
   memset(s4, 0xff, sizeof(s4));
   test_str("strcopy1a",   strcopy(s4, s1), s1);
   test_int("strcopy1b",   *(unsigned char *)(s4 + strlen(s1) + 2), (unsigned int)0xff);
   memset(s4, 0xfe, 8);
   test_str("strcopyn1a",  strcopyn(s4, s1, 7), "abcdefg");
   test_str("strcopyn1b",  s4, "abcdefg");
   test_str("strcopyn1c",  s4 + 8, s1 + 8);
   test_str("strcopyb1a",  strcopyb(s4, s2, 7), "xxxxxx");
   test_str("strcopyb1b",  s4 + 7, "");
   test_str("strcopyb1c",  s4 + 8, s1 + 8);
   test_str("strcopy2",    strcopy(s4, NULL), "");
   test_ptr("strcopy3",    strcopy(NULL, s1), NULL);
   test_str("strcopyb2",   strcopyb(s4, NULL, sizeof(s4)), "");
   test_ptr("strcopyb3",   strcopyb(NULL, s1, sizeof(s1)), NULL);
   strcpy(s4, s1);
   test_str("strcopyb4",   strcopyb(s4, s2, 0), s1);
   test_str("strcopyb5",   strcopyb(s4, s3, 1), "");
   test_str("strcopyn2",   strcopyn(s4, s2, 0), "");
   test_str("strcopyn3",   strcopyn(s4, s2, 1), "x");
   test_str("strconcat1a", strcopy(s4, s2), s2);
   test_str("strconcat1b", strconcat(s4, s1), s3);
   test_ptr("strconcat2",  strconcat(NULL, s1), NULL);
   test_str("strconcat3a", strcopy(s4, s1), s1);
   test_str("strconcat3b", strconcat(s4, NULL), s1);
   test_str("strconcatb1a",   strcopyb(s4, s2, sizeof(s4)), s2);
   test_str("strconcatb1b",   strconcatb(s4, s1, sizeof(s4)), s3);
   test_str("strconcatb2a",   strcopyb(s4, s2, sizeof(s4)), s2);
   test_str("strconcatb2b",   strconcatb(s4, s1, strlen(s4) + 1), s2);
   test_str("strconcatb3a",   strcopyb(s4, "abcd", 4), "abc");
   test_str("strconcatb3b",   strconcatb(s4, "efg", 6), "abcef");
   test_str("strconcatb4a",   strcopyb(s4, "abcd", 5), "abcd");
   test_str("strconcatb4b",   strconcatb(s4, "efg", 4), "abcd");
   test_ptr("strconcatb5",    strconcatb(NULL, s1, 1000), NULL);
   test_str("strconcatb6a",   strcopyb(s4, s1, sizeof(s4)), s1);
   test_str("strconcatb6b",   strconcatb(s4, NULL, sizeof(s4)), s1);
   test_str("strconcatb7a",   strcopyb(s4, "defghij", sizeof(s4)), "defghij");
   test_str("strconcatb7b",   strconcatb(s4, "yyyyyyyy", 2), "defghij");
   test_str("strconcatn1a",   strcopy(s4, "abcdefg"), "abcdefg");
   test_str("strconcatn1b",   strconcatn(s4, "xxxxxxxx", 0), "abcdefg");
   test_str("strconcatn2a",   strcopy(s4, "defghij"), "defghij");
   test_str("strconcatn2b",   strconcatn(s4, "yyyyyyyy", 1), "defghijy");
   test_str("strconcatn3a",   strcopy(s4, "abcdefg"), "abcdefg");
   test_str("strconcatn3b",   strconcatn(s4, "zzzzzzzz", 1000), "abcdefgzzzzzzzz");
   test_str("strconcatn4a",   strcopy(s4, "defghij"), "defghij");
   test_str("strconcatn4b",   strconcatn(s4, NULL, 1000), "defghij");
   test_ptr("strconcatn5",    strconcatn(NULL, "abcdefg", 5), NULL);
   test_str("strconcatbn1a",  strcopy(s4, "abcdefg"), "abcdefg");
   test_str("strconcatbn1b",  strconcatbn(s4, "yyyyyyyy", 8, 1000), "abcdefg");
   test_str("strconcatbn2a",  strcopy(s4, "defghij"), "defghij");
   test_str("strconcatbn2b",  strconcatbn(s4, "zzzzzzzz", 10, 1000), "defghijzz");
   test_str("strconcatbn3a",  strcopy(s4, "abcdefg"), "abcdefg");
   test_str("strconcatbn3b",  strconcatbn(s4, "xxxxxxxx", 16, 1000), "abcdefgxxxxxxxx");
   test_str("strconcatbn4a",  strcopy(s4, "defghij"), "defghij");
   test_str("strconcatbn4b",  strconcatbn(s4, "zzzzzzzz", 32, 4), "defghijzzzz");
   test_str("strconcatbn5a",  strcopy(s4, "abcdefg"), "abcdefg");
   test_str("strconcatbn5b",  strconcatbn(s4, "xxxxxxxx", 12, 4), "abcdefgxxxx");
   test_str("strconcatbn6a",  strcopy(s4, "defghij"), "defghij");
   test_str("strconcatbn6b",  strconcatbn(s4, "yyyyyyyy", 12, 4), "defghijyyyy");
   test_ptr("strconcatbn7",   strconcatbn(NULL, "abcdefg", 12, 12), NULL);
   test_str("strconcatbn8a",  strcopy(s4, "abcdefg"), "abcdefg");
   test_str("strconcatbn8b",  strconcatbn(s4, "abcdefg", 1, 12), "abcdefg");
   test_str("strconcatbn9a",  strcopy(s4, "defghij"), "defghij");
   test_str("strconcatbn9b",  strconcatbn(s4, NULL, 100, 100), "defghij");

   /* Test sbprintf */
   memset(s4, 0xff, sizeof(s4));
   test_str("sbprintf1a",  sbprintf(s4, 12, "abcd%s", "efg"), "abcdefg");
   test_int("sbprintf1b",  *(unsigned char *)(s4 + 9), 0xff);
   memset(s4, 0xfe, sizeof(s4));
   test_str("sbprintf2a",  sbprintf(s4, 8, "abcd%d", 123), "abcd123");
   test_int("sbprintf2b",  *(unsigned char *)(s4 + 9), 0xfe);
   memset(s4, 0xfd, sizeof(s4));
   test_str("sbprintf3a",  sbprintf(s4, 7, "abcd%03x", 0x42), "abcd04");
   test_int("sbprintf3b",  *(unsigned char *)(s4 + 8), 0xfd);

   /* Test sbprintf_append */
   memset(s4, 0xff, sizeof(s4));
   test_str("sbprintf_app1a",  sbprintf(s4, 12, "abcd%s", "efg"), "abcdefg");
   test_int("sbprintf_app1b",  *(unsigned char *)(s4 + 9), 0xff);
   test_str("sbprintf_app1c",  sbprintf_concat(s4, 12, "hi%sl", "jk"), "abcdefghijk");
   test_int("sbprintf_app1d",  *(unsigned char *)(s4 + 12), 0xff);
   test_str("sbprintf_app1e",  sbprintf_concat(s4, 12, "oops"), "abcdefghijk");
   test_int("sbprintf_app1f",  *(unsigned char *)(s4 + 12), 0xff);

   /* Test misc info functions */
   strcpy(s1, "now is \n  the\t    time, for\n all good men!!!");
   strcpy(s2, "\n\n\n");
   test_int("strnumwords1",   strnumwords(s1),  8);
   test_int("strnumlines1",   strnumlines(s1),  3);
   test_int("strnumlines2",   strnumlines(s2),  4);

   /* String replacement functions */
   p = "abcab$replcabcabca$replbc";
   strcpy(s1, p);
   strcpy(s2, s1);
   test_int("strreplace1",    strreplaceb(NULL, "$repl", "XXX", 1000), 0);
   test_int("strreplace2a",   strreplaceb(s2, "$repl", "XXX", 0), 0);
   test_str("strreplace2b",   s2, s1);
   test_str("strreplace3a",   strcopy(s2, s1), p);
   test_int("strreplace3b",   strreplaceb(s2, "$repl", "", sizeof(s2)), 2);
   test_str("strreplace3c",   s2, "abcabcabcabcabc");
   test_str("strreplace4a",   strcopy(s2, s1), p);
   test_int("strreplace4b",   strreplaceb(s2, "$repl", NULL, sizeof(s2)), 2);
   test_str("strreplace4c",   s2, "abcabcabcabcabc");
   test_str("strreplace5a",   strcopy(s2, s1), p);
   test_int("strreplace5b",   strreplaceb(s2, "$repl", "X", sizeof(s2)), 2);
   test_str("strreplace5c",   s2, "abcabXcabcabcaXbc");
   test_str("strreplace6a",   strcopy(s2, s1), p);
   test_int("strreplace6b",   strreplacen(s2, "$repl", "X", strlen(s1)), 2);
   test_str("strreplace6c",   s2, "abcabXcabcabcaXbc");
   test_str("strreplace7a",   strcopy(s2, s1), p);
   test_int("strreplace7b",   strreplaceb(s2, "$repl", "XXXXX", sizeof(s2)), 2);
   test_str("strreplace7c",   s2, "abcabXXXXXcabcabcaXXXXXbc");
   test_str("strreplace8a",   strcopy(s2, s1), p);
   test_int("strreplace8b",   strreplacen(s2, "$repl", "XXXXX", strlen(s1)), 2);
   test_str("strreplace8c",   s2, "abcabXXXXXcabcabcaXXXXXbc");
   test_str("strreplace9a",   strcopy(s2, s1), p);
   test_str("strreplace9b",   s1, p);
   test_int("strreplace9c",   strreplaceb(s2, "$repl", "XXXXXXXX", sizeof(s2)), 2);
   test_str("strreplace9d",   s1, p);
   test_str("strreplace9e",   s2, "abcabXXXXXXXXcabcabcaXXXXXXXXbc");
   /* The replacement cases which truncate start here */
   memset(s2, 0xff, sizeof(s2));
   test_str("strreplace10a",  strcopy(s2, s1), p);
   test_int("strreplace10b",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 1), 2);
   test_str("strreplace10c",  s2, "abcabXXXXXXXXcabcabcaXXXXX");
   test_int("strreplace10d",  *(unsigned char *)(s2 + strlen(s1) + 2), 0xff);
   memset(s2, 0xfe, sizeof(s2));
   test_str("strreplace10e",  strcopyn(s2, s1, strlen(s1)), p);
   test_int("strreplace10f",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 1), 2);
   test_str("strreplace10g",  s2, "abcabXXXXXXXXcabcabcaXXXXX");
   test_int("strreplace10h",  *(unsigned char *)(s2 + strlen(s1) + 2), 0xfe);
   memset(s2, 0xfd, sizeof(s2));
   test_str("strreplace10i",  strcopy(s2, s1), p);
   test_int("strreplace10j",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1)), 1);
   test_str("strreplace10k",  s2, "abcabXXXXXXXXcabcabca$rep");
   test_int("strreplace10l",  *(unsigned char *)(s2 + strlen(s1) + 1), 0xfd);
   memset(s2, 0xfc, sizeof(s2));
   test_str("strreplace10m",  strcopyn(s2, s1, strlen(s1)), p);
   test_int("strreplace10n",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1)), 1);
   test_str("strreplace10o",  s2, "abcabXXXXXXXXcabcabca$rep");
   test_int("strreplace10p",  *(unsigned char *)(s2 + strlen(s1) + 1), 0xfc);
   memset(s2, 0xfb, sizeof(s2));
   test_str("strreplace11a",  strcopy(s2, s1), p);
   test_int("strreplace11b",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 4), 2);
   test_str("strreplace11c",  s2, "abcabXXXXXXXXcabcabcaXXXXXXXX");
   test_int("strreplace11d",  *(unsigned char *)(s2 + strlen(s1) + 5), 0xfb);
   memset(s2, 0xfa, sizeof(s2));
   test_str("strreplace11e",  strcopyn(s2, s1, strlen(s1) + 4), p);
   test_int("strreplace11f",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 4), 2);
   test_str("strreplace11g",  s2, "abcabXXXXXXXXcabcabcaXXXXXXXX");
   test_int("strreplace11h",  *(unsigned char *)(s2 + strlen(s1) + 5), 0xfa);
   test_str("strreplace12a",  strcopy(s2, s1), p);
   test_int("strreplace12b",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 5), 2);
   test_str("strreplace12c",  s2, "abcabXXXXXXXXcabcabcaXXXXXXXXb");
   test_str("strreplace12d",  strcopyn(s2, s1, strlen(s1) + 5), p);
   test_int("strreplace12e",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 5), 2);
   test_str("strreplace12f",  s2, "abcabXXXXXXXXcabcabcaXXXXXXXXb");
   test_str("strreplace13a",  strcopy(s2, s1), p);
   test_int("strreplace13b",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 6), 2);
   test_str("strreplace13c",  s2, "abcabXXXXXXXXcabcabcaXXXXXXXXbc");
   test_str("strreplace13d",  strcopyn(s2, s1, strlen(s1) + 6), p);
   test_int("strreplace13e",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 6), 2);
   test_str("strreplace13f",  s2, "abcabXXXXXXXXcabcabcaXXXXXXXXbc");
   test_str("strreplace14a",  strcopy(s2, s1), p);
   test_int("strreplace14b",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 7), 2);
   test_str("strreplace14c",  s2, "abcabXXXXXXXXcabcabcaXXXXXXXXbc");
   test_str("strreplace14d",  strcopyn(s2, s1, strlen(s1) + 7), p);
   test_int("strreplace14e",  strreplacen(s2, "$repl", "XXXXXXXX", strlen(s1) + 7), 2);
   test_str("strreplace14f",  s2, "abcabXXXXXXXXcabcabcaXXXXXXXXbc");
   test_str("strreplace15a",  strcopy(s2, s1), p);
   test_int("strreplace15b",  strreplacen(s2, "$repl", "XXXXXXXX", 16), 1);
   test_str("strreplace15c",  s2, "abcabXXXXXXXXcab");
   test_str("strreplace15d",  strcopyn(s2, s1, 16), "abcab$replcabcab");
   test_int("strreplace15e",  strreplacen(s2, "$repl", "XXXXXXXX", 16), 1);
   test_str("strreplace15f",  s2, "abcabXXXXXXXXcab");
   test_str("strreplace16a",  strcopy(s2, s1), p);
   test_int("strreplace16b",  strreplacen(s2, "$repl", "XXXXXXXX", 8), 1);
   test_str("strreplace16c",  s2, "abcabXXX");
   test_str("strreplace16d",  strcopyn(s2, s1, 8), "abcab$re");
   test_int("strreplace16e",  strreplacen(s2, "$repl", "XXXXXXXX", 8), 0);
   test_str("strreplace16f",  s2, "abcab$re");
   test_str("strreplace17a",  strcopy(s2, s1), p);
   test_int("strreplace17b",  strreplacen(s2, "$repl", "XXXXXXXX", 4), 0);
   test_str("strreplace17c",  s2, p);
   test_str("strreplace17d",  strcopyn(s2, s1, 4), "abca");
   test_int("strreplace17e",  strreplacen(s2, "$repl", "XXXXXXXX", 4), 0);
   test_str("strreplace17f",  s2, "abca");
   test_str("strreplace18a",  strcopy(s2, s1), p);
   test_int("strreplace18b",  strreplacen(s2, "$repl", "XXXXXXXX", 5), 1);
   test_str("strreplace18c",  s2, "abcab");
   test_str("strreplace19a",  strcopy(s2, s1), p);
   test_int("strreplace19b",  strreplacen(s2, "$repl", "XXXXXXXX", 6), 1);
   test_str("strreplace19c",  s2, "abcabX");

   /* Briefly test trim ops */
   #define  WS1   "           "
   #define  WS2   "ab   cd e    fh dflvmd;  moogle"
   #define  WS3   "\n  \t \t \n   \t"
   #define  WS    (WS1 WS2 WS3)
   strcpy(s1, WS);
   test_str("trim1",       trim(s1), WS2);
   strcpy(s1, WS);
   test_str("ltrim1",      ltrim(s1), WS2 WS3);
   strcpy(s1, WS);
   test_str("rtrim1",      rtrim(s1), WS1 WS2);
   strcpy(s1, WS2);
   test_str("trim2",       trim(s1), WS2);
   strcpy(s1, WS2 WS3);
   test_str("ltrim2",      ltrim(s1), WS2 WS3);
   strcpy(s1, WS1 WS2);
   test_str("rtrim2",      rtrim(s1), WS1 WS2);
   test_ptr("trim3",       trim(NULL), NULL);
   test_ptr("ltrim3",      ltrim(NULL), NULL);
   test_ptr("rtrim3",      rtrim(NULL), NULL);
   strcpy(s1, WS1 WS2 WS3 "#" WS1 WS2 WS3);
   test_str("trimcomment1",   trimcomment(s1, '#'), WS);
   test_ptr("trimcomment2",   trimcomment(NULL, '#'), NULL);

   /* Test assignments */
   strcpy(s1, "   abc   =   def   ");
   test_int("getassign1a", getassign(s2, s3, s1), true);
   test_str("getassign1b", s2, "abc");
   test_str("getassign1c", s3, "def");
   test_int("getassign2a", getassign(s2, s3, "moogle32"), false);
   test_str("getassign2b", s2, "");
   test_str("getassign2c", s3, "");
   test_int("getassign3a", getassign(s2, s3, NULL), false);
   test_str("getassign3b", s2, "");
   test_str("getassign3c", s3, "");
   test_int("getassign4",  getassign(s3, NULL, s1), false);

   /* Test getarg interface */
   strcpy(s1, "arg0|  arg1  |arg2  |  arg33333|4|");
   test_int("getnumargs1", getnumargs(s1, '|'), 6);
   test_int("getnumargs2", getnumargs(NULL, '|'), 0);
   test_int("getnumargs3", getnumargs("", '|'), 0);
   test_int("getnumargs4", getnumargs("moogle", '|'), 1);
   test_str("getarg1",     getarg(s2, s1, 0, '|'), "arg0");
   test_str("getarg2",     getarg(s2, s1, 1, '|'), "arg1");
   test_str("getarg3",     getarg(s2, s1, 7, '|'), "");
   test_str("getarg4",     getarg(s2, NULL, 7, '|'), "");
   test_ptr("getarg5",     getarg(NULL, s1, 1, '|'), NULL);
   test_str("getargb1",    getargb(s2, s1, 0, '|', 4), "arg");
   test_str("getargb2",    getargb(s2, s1, 1, '|', 4), "arg");
   test_str("getargb3",    getargb(s2, NULL, 1, '|', 4), "");
   test_ptr("getargb4",    getargb(NULL, s1, 1, '|', 100), NULL);

   /* Summary */
   printf("Total: %d   Success: %d  Fail: %d\n", success + failure, success, failure);
   return(failure > 0 ? 1 : 0);

}
