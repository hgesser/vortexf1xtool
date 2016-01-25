all: vortex

vortex: vortex.c
	gcc -std=c99 -o vortex vortex.c

