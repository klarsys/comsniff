
CFLAGS = -g
OBJS = $(patsubst %.c, %.o, $(wildcard *.c))

comsniff.exe: $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	$(RM) $(OBJS) comsniff.exe
