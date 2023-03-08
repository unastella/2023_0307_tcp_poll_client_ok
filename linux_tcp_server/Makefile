CC = gcc
CFLAG = -g -O0 -Wall -lpthread

SRCS = $(wildcard *.c */*.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
NAME = $(wildcard *.c)

TARGET = $(patsubst %.c, %, $(NAME))

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAG)

%.o:%.c
	$(CC) -o $@ -c $< -g

clean:
	rm -rf $(TARGET) $(OBJS)
