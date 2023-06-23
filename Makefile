# Variables to override
#
# CC            C compiler
# CROSSCOMPILE	crosscompiler prefix, if any
# CFLAGS	compiler flags for compiling all C files
# ERL_CFLAGS	additional compiler flags for files using Erlang header files
# ERL_EI_INCLUDE_DIR include path to ei.h (Required for crosscompile)
# ERL_EI_LIBDIR path to libei.a (Required for crosscompile)
# LDFLAGS	linker flags for linking all binaries
# ERL_LDFLAGS	additional linker flags for projects referencing Erlang libraries


MIX = mix
PREFIX = $(MIX_APP_PATH)/priv
DEFAULT_TARGETS ?= $(PREFIX) $(PREFIX)/scenic_driver_debug

# # Look for the EI library and header files
# # For crosscompiled builds, ERL_EI_INCLUDE_DIR and ERL_EI_LIBDIR must be
# # passed into the Makefile.
# ifeq ($(ERL_EI_INCLUDE_DIR),)
# ERL_ROOT_DIR = $(shell erl -eval "io:format(\"~s~n\", [code:root_dir()])" -s init stop -noshell)
# ifeq ($(ERL_ROOT_DIR),)
# 	$(error Could not find the Erlang installation. Check to see that 'erl' is in your PATH)
# endif
# ERL_EI_INCLUDE_DIR = "$(ERL_ROOT_DIR)/usr/include"
# ERL_EI_LIBDIR = "$(ERL_ROOT_DIR)/usr/lib"
# endif

# # Set Erlang-specific compile and linker flags
# ERL_CFLAGS ?= -I$(ERL_EI_INCLUDE_DIR)
# ERL_LDFLAGS ?= -L$(ERL_EI_LIBDIR) -lei


CFLAGS = -O3 -std=c99

ifndef MIX_ENV
	MIX_ENV = dev
endif

ifdef DEBUG
	CFLAGS += -pedantic -Weverything -Wall -Wextra -Wno-unused-parameter -Wno-gnu
endif

ifeq ($(MIX_ENV),dev)
	CFLAGS += -g
endif

ifneq ($(OS),Windows_NT)
	CFLAGS += -fPIC

	ifeq ($(shell uname),Darwin)
		LDFLAGS += -framework Cocoa -Wno-deprecated
	else
		LDFLAGS += -lm -lrt
	endif
endif

SRCS = c_src/main.c c_src/comms.c c_src/unix_comms.c \
c_src/script.c c_src/image.c c_src/font.c \
c_src/tommyds/src/tommyhashlin.c c_src/tommyds/src/tommyhash.c

calling_from_make:
	mix compile

all: $(DEFAULT_TARGETS)

$(PREFIX):
	mkdir -p $@

$(PREFIX)/scenic_driver_debug: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	$(RM) -rf $(PREFIX)

.PHONY: all clean calling_from_make

