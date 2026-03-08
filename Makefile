CC = gcc
CFLAGS = -Wall -g

# Target to build the executable
hw5: hw5.c fs.h stat.h types.h
	$(CC) $(CFLAGS) -o hw5 hw5.c

# Clean up build files
clean:
	rm -f hw5 *.o