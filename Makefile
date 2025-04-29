# Compiler to use
CC = gcc

# Compiler flags, this example doesn't need it, but using -pedantic as example
## -pedantic: Enforces strict ISO C compliance
CFLAGS = -pedantic

# Libraries to link with
## -lwayland-client: Link against the Wayland client library
LIBS = -lwayland-client

# Output executable name
EXE = toplevel-list-example

# Source files
SRC = main.c wlr-foreign-toplevel-management-unstable-v1-client-protocol.c

# Object files (replacing .c with .o)
OBJ = $(SRC:.c=.o)

# Path to Wayland protocol XML definitions
PROTOCOLS_PATH = /usr/share/wlr-protocols/unstable

# Name of the protocol XML file to process
WLR_FOREIGN_TOPLEVEL = wlr-foreign-toplevel-management-unstable-v1.xml

# Generated client header file from protocol
CLIENT_HEADER = wlr-foreign-toplevel-management-unstable-v1-client-protocol.h

# Generated private implementation file from protocol
PRIVATE_CODE  = wlr-foreign-toplevel-management-unstable-v1-client-protocol.c

# Default rule: generate protocol code and build the executable
all: generate-protocol $(EXE)

# Rule to generate Wayland protocol source and header from XML
generate-protocol:
	# Generate the client-side header
	wayland-scanner client-header $(PROTOCOLS_PATH)/$(WLR_FOREIGN_TOPLEVEL) $(CLIENT_HEADER)
	# Generate the private C source file for protocol implementation
	wayland-scanner private-code $(PROTOCOLS_PATH)/$(WLR_FOREIGN_TOPLEVEL) $(PRIVATE_CODE)

# Rule to link the object files into the final executable
$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

# Rule to compile .c source files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up generated files
clean:
	rm -f $(OBJ) $(EXE)
