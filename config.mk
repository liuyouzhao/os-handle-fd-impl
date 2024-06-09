# Prefix platform
PLATFORM=linux
BUILD=build
BOARD       =
CPU         =

# Makefile main logic
ROOT=$(shell pwd)
PRE_ROOT=$(ROOT)

# toolchain gcc prefix
CC    =gcc
AS    =as
LD    =gcc
OC    =cp
LDIR  =
LDIR2 =


# output binary path
OUT = out
OUT_DIR = ./$(OUT)/$(PLATFORM)
OUT_BIN = main

# compiling and linking flags
C_CFLAGS += -I./src/include
C_CFLAGS += -I./src/include/arch/$(PLATFORM)
C_CFLAGS += -I./src/include/kernel
C_CFLAGS += -I./src/include/lib 
C_CFLAGS += -I./src/tests
C_CFLAGS += -Os
AFLAGS=
CFLAGS= $(C_CFLAGS)
LDFLAGS = -lc -lm -lpthread
