COMPILER = gcc
OBJS = main.o luastar.o llexer.o lmem.o lparse.o lop.o lutil.o
EXEC_NAME = output
CFLAGS = 

.PHONY:
all: $(EXEC_NAME)

# executable
$(EXEC_NAME): $(OBJS)
	@$(COMPILER) $(OBJS) -o $(EXEC_NAME)

# build em' all
%.o: %.c
	@$(COMPILER) $(CFLAGS) -c $< -o $@

# clean
.PHONY:
clean:
	@rm -r *.o $(EXEC_NAME)

