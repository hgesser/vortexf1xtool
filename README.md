# vortexf1xtool
The `vortex` tools lets you access 720 KB CP/M disk images (Vortex VDOS format on double-density, DD, disks) as used by Schneider / Amstrad CPC home computers with an external floppy drive (Vortex F1-X, either the 5.25" or the 3.5" version). Note that no other CP/M formats are supported though it should be possible to quickly modify the code so that other formats could work as well.

## Usage

You can call the `vortex` with either an `ls` command option or a `dump` command option, followed by the names(s) of one or more floppy disk image files. The `ls` command will displays information about the files stored on that disk image, such as follows:

```
[esser@macbookpro:~]$ vortex ls cpc-004.dsk 
Directory listing for 'cpc-004.dsk'
Us Name         RS  #E     Size   [E] 4K Blocks
-------------------------------------------------------------------------------
00 copytool.com -- (0)    3840    [0]   1 
00 files-0.bin  -s (1)     128    [0]   3   4   5   6  [1]   7 
00 files-01.bin -s (1)     128    [0]   8   9  10  11  [1]  12 
00 files-1.bin  -s (2)    9728    [0]  13  14  15  16  [1]  17  18  19  20  [2]  21  22  23 
00 files-2.bin  -s (2)    9472    [0]  24  25  26  27  [1]  28  29  30  31  [2]  32  33  34 
00 files-3.bin  -s (2)    5888    [0]  35  36  37  38  [1]  39  40  41  42  [2]  43  44 
00 ts-prog.com  -- (0)    8704    [0]  45  46  47 
00 ts-prog.dat  -s (3)   16384    [0]  48  49  50  51  [1]  52  53  54  55  [2]  56  57  58  59  [3]  60  61  62  63 
 +                 (7)   14336    [4]  64  65  66  67  [5]  68  69  70  71  [6]  72  73  74  75  [7]  76  77  78  79 
 +              total:   30720
00 zprog.bas    -- (0)    7040    [0]  80  81 
00 zprog.prg    -s (2)    4224    [0]  82  83  84  85  [1]  86  87  88  89  [2]  90  91 
00 zprog.scr    -s (0)   14336    [0]  92  93  94  95 
...
```

When using the `dump` command option, the tool will create a new subdirectory (named `whatever.d/` if the image file was called `whatever`) and copy all files from the CP/M image to that directory:

```
[esser@macbookpro:~]$ vortex dump cpc-004.dsk 
...
[esser@macbookpro:~]$ ls -l cpc-004.dsk/
-rw-r--r--   1 esser  staff   3840 25 Jan 06:37 copytool.com
-rw-r--r--   1 esser  staff    128 25 Jan 06:37 files-0.bin
-rw-r--r--   1 esser  staff    128 25 Jan 06:37 files-01.bin
-rw-r--r--   1 esser  staff   9728 25 Jan 06:37 files-1.bin
-rw-r--r--   1 esser  staff   9472 25 Jan 06:37 files-2.bin
-rw-r--r--   1 esser  staff   5888 25 Jan 06:37 files-3.bin
-rw-r--r--   1 esser  staff   8704 25 Jan 06:37 ts-prog.com
-rw-r--r--   1 esser  staff  30720 25 Jan 06:37 ts-prog.dat
-rw-r--r--   1 esser  staff   7040 25 Jan 06:37 zprog.bas
-rw-r--r--   1 esser  staff   4224 25 Jan 06:37 zprog.prg
-rw-r--r--   1 esser  staff  14336 25 Jan 06:37 zprog.scr
...
```

## How to get images?

If you have physical media, the easiest approach is to use an old computer that still supports 5.25" drives (in BIOS settings you will find the drive type "5.25 inch, 1200 KByte"). Install an old Linux system on that computer's hard disk; I used a Linux system with kernel 2.x, the main point was that `/dev` is not a virtual filesystem but a regular directory filled with hundreds of device files. If so, there should be two files `/dev/fd0h720` (for the `A:` drive) and `/dev/fd1h720` (the `B:` drive). Using these filenames guarantees that the floppy disks are read in the proper way (as double-density, DD, disks -- not SD or HD). Modern Linux systems will only provide the generic `/dev/fd0` and `/dev/fd1` which might or might not work.

For reading, `dd` or `ddrescue` work fine. I used dd:

```
dd if=/dev/fd0h720 of=cpc-0xx.dsk
```

and transfered the files to a different machine where I used the `vortex` tool to extract the files.
