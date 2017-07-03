
CFLAGS = -g
OBJS = comsniff.o

comsniff.exe: $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	$(RM) $(OBJS) comsniff.exe
