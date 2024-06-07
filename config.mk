# Prefix platform
PLATFORM=x86
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
OUT_BIN = output

# compiling and linking flags
C_CFLAGS += -I./ -Os
AFLAGS=
CFLAGS= $(C_CFLAGS)
LDFLAGS = -lc -lm 
