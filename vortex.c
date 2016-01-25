/* vortexf1xtool

   (c) 2016 Hans-Georg Esser
       h.g.esser <at> gmx.de
   Licensed under the GPL 3.0 (see LICENSE)
*/

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long uint;

#define PEEK(addr) (*(addr))


struct cpmdirentry {
  unsigned char user;
  char filename[8];
  char extension[3];
  unsigned char extentcounter;
  unsigned char s1;   // reserved, always 0
  unsigned char s2;
  unsigned char numrecords;
  unsigned char alloc[16];
};


void hexdump (unsigned char *startval, unsigned char *endval) {
  for (unsigned char *i=startval; i < endval; i+=16) {
    printf ("%8p  ", i);                   // address
    for (unsigned char *j = i;  j < i+16;  j++) {      // hex values
      printf ("%02x ", PEEK(j));
      if (j==i+7) printf (" ");
    };
    printf (" ");
    for (unsigned char *j = i;  j < i+16;  j++) {      // characters
      char z = PEEK(j);
      if ((z>=32) && (z<127)) {
        printf ("%c", PEEK(j));
      } else {
        printf (".");
      };
    };    
    printf ("\n");
  };
};


void trunc2ascii (char *str) {
  while (*str != 0) {
    *str = *str & 0x7f;
    str++;
  }
}

void asciilower (char *str) {
  while (*str != 0) {
    if (*str >= 'A' && *str <= 'Z')
      *str = *str -'A' + 'a';
    str++;
  }
}


#define CPM_RECORDSIZE 128
#define CPM_BLOCKSIZE  4096
//  -> 180 blocks on one 720 KB disk


void show_direntry (struct cpmdirentry *dir) {
  char filename[] = "        .   ";
  char attr[] = "--";
  if ((dir->extension[0] & 128) == 128) attr[0]='r';  // read-only
  if ((dir->extension[1] & 128) == 128) attr[1]='s';  // system
  strncpy (filename, dir->filename, 8);
  strncpy (filename+9, dir->extension, 3);
  trunc2ascii (filename); asciilower (filename);
  printf ("%02d %s %s (%d) %7d   ", dir->user, filename, attr, dir->extentcounter,
    dir->numrecords * CPM_RECORDSIZE);
  for (short i = 0; i < 16; i++) {
    if (dir->alloc[i] != 0) {
      if (i%4 == 0) printf (" [%d] ", i/4 + (dir->extentcounter/4)*4);
      printf ("%3d ", dir->alloc[i]);
    }
  }
  printf ("\n");
}

int main (int argc, char *argv[]) {
//  int fd = open ("/Users/esser/dos/cpc-001.dsk", O_RDONLY);
  printf ("Directory listing for '%s'\n", argv[1]);
  int fd = open (argv[1], O_RDONLY);
  char *file = mmap (0, 720*1024, PROT_READ, MAP_SHARED, fd, 0);
  if (file == MAP_FAILED) {
    printf ("fd = %d\n", fd);
    printf ("Pointer: %p\n", file);
    perror ("mmap: ");
    exit (1);
  }
  
  // hexdump (file + 9*1024, file + 9*1024 + 512);


  file += 9*1024;
  struct cpmdirentry *d;
  d = (struct cpmdirentry *)file;

  // 4K directory = 128 entries
  printf ("Us Name         RS  #E     Size   [E] 4K Blocks\n");
  printf ("----------------------------------------------------------------\n");
  for (int i = 0; i < 128; i++) {
    if ((d+i)->user != 0xe5)   // e5 = no entry
      show_direntry (d + i);
  }


}