# Makefile
# GNU makefile for example Ising model code
# Questions/comments to gruberja@gmail.com (Jason Gruber)

# includes
incdir = ../../../include/

# compilers/flags
compiler = g++
pcompiler = mpic++
flags = -O3 -I $(incdir)
pflags = $(flags) -include mpi.h

# the program
ising: ising.cpp
	$(compiler) $(flags) $< -o $@ -lz

parallel: ising.cpp
	$(pcompiler) $(pflags) $< -o $@ -lz

clean:
	rm -f ising parallel
