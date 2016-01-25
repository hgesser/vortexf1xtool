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
#include <unistd.h>
#include <sys/stat.h>

typedef unsigned long uint;

#define PEEK(addr) (*(addr))

#define DUMPDIRNAMESIZE 512
char dumpdirname[DUMPDIRNAMESIZE];
int current_filesize;


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

void cpmdirentry_to_unixname (struct cpmdirentry *dir, char *unixname) {
  // convert 'abcdef   .e  ' to 'abcdef.e'
  // and     'abcdef   .   ' to 'abcdef'
  unixname[8]='.';
  strncpy (unixname, dir->filename, 8);
  strncpy (unixname+9, dir->extension, 3);
  trunc2ascii (unixname); asciilower (unixname);

  // remove blanks in filename
  short blanks = 0;
  for (short i = 7; i>0; i--) {
    if (unixname[i] == ' ')
      blanks++;
    else
      break;
  }
  if (blanks > 0) {
    for (short i = 8; i < 13; i++)
      unixname[i-blanks] = unixname[i];
  }

  // remove blanks in extension
  short len = strlen (unixname);
  for (short i = 1; i < 4; i++) {
    if (unixname[len-i] == ' ') 
      unixname[len-i] = 0;  // terminate
    else
      break;
  }
  
  // remove trailing dot
  len = strlen (unixname);
  if (unixname[len-1] == '.')
    unixname[len-1] = 0;
}


#define CPM_RECORDSIZE  128
#define CPM_BLOCKSIZE   4096       //  -> 180 blocks on one 720 KB disk
#define CPM_DIRSTART    (9*1024)
#define CPM_DATAZONE    (13*1024)
#define CPM_DIR_ENTRIES 128
#define CPM_MAX_BLOCKS_PER_EXTENT 4
#define CPM_IMAGE_FILESIZE (720*1024)

#define DIR_ENTRY_FIRST     0
#define DIR_ENTRY_CONTINUES 1

#define ACTION_DISPLAY 1
#define ACTION_DUMP    2

void handle_direntry (struct cpmdirentry *dir, void *fulldir, 
                      short action, short cont);

void find_more_direntries (struct cpmdirentry *dir, void *fulldir, short action) {
  struct cpmdirentry *d;
  d = (struct cpmdirentry *)fulldir;
  
  int filesize = dir->numrecords * CPM_RECORDSIZE;

  for (int i = 0; i < CPM_DIR_ENTRIES; i++) {
    if ((d+i)->user != 0xe5 &&      // e5 = no entry
        (d+i) != dir &&             // ignore first entry
        memcmp (d+i, dir, 12) == 0) { // same user + filename
      handle_direntry (d + i, d, action, DIR_ENTRY_CONTINUES);
      filesize += (d+i)->numrecords * CPM_RECORDSIZE;
    }
  }
  
  current_filesize = filesize;   // global
  
  if (action == ACTION_DISPLAY) {
    printf (" +              total: %7d\n", filesize);
  }
}

void handle_direntry (struct cpmdirentry *dir, void *fulldir, 
                      short action, short cont) {
  char filename[] = "        .   ";
  char unixname[13];
  char attr[] = "--";
  if ((dir->extension[0] & 128) == 128) attr[0]='r';  // read-only
  if ((dir->extension[1] & 128) == 128) attr[1]='s';  // system
  strncpy (filename, dir->filename, 8);
  strncpy (filename+9, dir->extension, 3);
  trunc2ascii (filename); asciilower (filename);
  cpmdirentry_to_unixname (dir, unixname);

  current_filesize = dir->numrecords * CPM_RECORDSIZE;   // global

  // display file info
  if (action == ACTION_DISPLAY) {
    if (cont == DIR_ENTRY_FIRST)
      printf ("%02d %-12s %s (%d) %7d   ", dir->user, unixname, attr, 
              dir->extentcounter, dir->numrecords * CPM_RECORDSIZE);
    else
      printf (" +                 (%d) %7d   ", 
              dir->extentcounter, dir->numrecords * CPM_RECORDSIZE);
  
    for (short i = 0; i < 16; i++) {
      if (dir->alloc[i] != 0) {
        if (i % CPM_MAX_BLOCKS_PER_EXTENT == 0) 
          printf (" [%d] ", i / CPM_MAX_BLOCKS_PER_EXTENT +
            (dir->extentcounter / CPM_MAX_BLOCKS_PER_EXTENT) * 
               CPM_MAX_BLOCKS_PER_EXTENT);
        printf ("%3d ", dir->alloc[i]);
      }
    }
    printf ("\n");
  }

  // dump file
  int fd;
  if (action == ACTION_DUMP) {
    printf ("DUMPING... %s\n", unixname);
    char dumpfilename[256];
    strcpy (dumpfilename, dumpdirname);
    strcat (dumpfilename, unixname);
    printf ("dumping to %s\n", dumpfilename);
    fd = open (dumpfilename, O_WRONLY | O_CREAT, 0666);
    lseek (fd, (dir->extentcounter / CPM_MAX_BLOCKS_PER_EXTENT) * 
           CPM_MAX_BLOCKS_PER_EXTENT * CPM_BLOCKSIZE, SEEK_SET);
    for (short i = 0; i < 16; i++) {
      if (dir->alloc[i] != 0) {
        write (fd, ((char*)fulldir) - CPM_DIRSTART + CPM_DATAZONE
               +(dir->alloc[i]-1) * CPM_BLOCKSIZE, CPM_BLOCKSIZE);
      }
    }

  }

  if (dir->alloc[15] != 0
      && dir->extentcounter < CPM_MAX_BLOCKS_PER_EXTENT) {
    find_more_direntries (dir, fulldir, action);
  }
  
  if (action == ACTION_DUMP) {
    ftruncate (fd, current_filesize);   // cut to multiple of record size
    close (fd);
  }
}

void help (char *prog) {
  printf ("%s: %s (ls|dump) file [files]\n", prog, prog);
}

void handle_vortex_image (char *vortexfilename, short action) {
  if (action == ACTION_DISPLAY) {
    printf ("Directory listing for '%s'\n", vortexfilename);
  }

  int fd = open (vortexfilename, O_RDONLY);
  char *file = mmap (0, CPM_IMAGE_FILESIZE, PROT_READ, MAP_SHARED, fd, 0);
  if (file == MAP_FAILED) {
    printf ("Error: cannot open %s\n", vortexfilename);
    close (fd);
    return;
  }
  
  file += CPM_DIRSTART;
  struct cpmdirentry *d;
  d = (struct cpmdirentry *)file;

  // 4K directory = 128 entries
  if (action == ACTION_DISPLAY) {
    printf ("Us Name         RS  #E     Size   [E] 4K Blocks\n");
    printf ("-------------------------------------------------------------------------------\n");
  } else
  if (action == ACTION_DUMP) {
    int len = strlen(vortexfilename);
    strcpy (dumpdirname, vortexfilename);
    strcat (dumpdirname, ".d/");
    int res = mkdir (dumpdirname, 0770);
    if (res != 0) {
      printf ("Error: cannot create %s directory\n", dumpdirname);
      return;
    }
  }

  for (int i = 0; i < CPM_DIR_ENTRIES; i++) {
    if ((d+i)->user != 0xe5 &&                            // e5 = no entry
        (d+i)->extentcounter < CPM_MAX_BLOCKS_PER_EXTENT) // 0..3: first extent
      handle_direntry (d + i, d, action, DIR_ENTRY_FIRST);
  }

  // clean up
  munmap (file, CPM_IMAGE_FILESIZE);
  close (fd);
}

int main (int argc, char *argv[]) {
  short action;
  if (argc == 1) {                      // no argument
    help (argv[0]);
    exit (0);
  }
  
  if (strcmp(argv[1], "help") == 0) {   // help
    help (argv[0]);
    exit (0);
  } else
  if (strcmp(argv[1], "ls") == 0) {     // ls
    action = ACTION_DISPLAY;
  } else
  if (strcmp(argv[1], "dump") == 0) {   // dump
    action = ACTION_DUMP;
    printf ("dump not implemented yet\n");
  } else {
    printf ("%s: %s (ls|dump) file [files]\n", argv[0], argv[0]);
    exit (1);
  }
  
  

  for (short i = 2; i < argc; i++)
    handle_vortex_image (argv[i], action);
}