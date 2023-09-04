# Define the compiler to use
CC = gcc

# Define any compile-time flags
CFLAGS = -Wall

# Define the source files
SOURCES = main.c

# Define the output executable
OUTPUT = program

# The default target that 'make' will execute if no targets are specified
all: $(OUTPUT)

# The rule to produce the output.
# It specifies that the output depends on the source files
$(OUTPUT): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTPUT)

# A target to clean up any produced files
clean:
	rm -f $(OUTPUT)
