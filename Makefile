CC = gcc
LD = gcc
CFLAGS  = -std=gnu99 -O2 -Wall -g
LDFLAGS	= -lOpenCL -lm

EXE = fractals
CFILES	=	$(wildcard *.c)
OBJECTS	=	$(CFILES:.c=.o)

all: $(EXE)

$(EXE) : $(OBJECTS)
	$(LD) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean :	
	rm -f $(EXE) $(OBJECTS) *.ppm
