# Special target for building with the host GCC in coverage collecting mode.

TOOLPATH := /usr/bin
# Get the $(CFLAGSENV), $(CXXFLAGSENV), $(LDFLAGSENV)
include $(OPENMRNPATH)/etc/env.mk

# Define this variable if you want to use a specific (suffixed) GCC version
# instead of the system default.
# GCCVERSION=-8

CC = gcc$(GCCVERSION)
CXX = g++$(GCCVERSION)
AR = ar
LD = g++$(GCCVERSION)

HOST_TARGET := 1

STARTGROUP := -Wl,--start-group
ENDGROUP := -Wl,--end-group

ifdef SKIP_COVERAGE
ARCHOPTIMIZATION = -g -O0
else
ARCHOPTIMIZATION = -g -O0 -fprofile-arcs -ftest-coverage
endif

CSHAREDFLAGS = -c -frandom-seed=$(shell echo $(abspath $<) | md5sum  | sed 's/\(.*\) .*/\1/') $(ARCHOPTIMIZATION) $(INCLUDES) -Wall -Werror -Wno-unknown-pragmas -MD -MP -fno-stack-protector -D_GNU_SOURCE -DGTEST

CFLAGS = $(CSHAREDFLAGS) -std=gnu99 $(CFLAGSEXTRA)

CXXFLAGS = $(CSHAREDFLAGS) -std=c++1y -D__STDC_FORMAT_MACROS \
           -D__STDC_LIMIT_MACROS $(CXXFLAGSEXTRA) #-D__LINEAR_MAP__


LDFLAGS = $(ARCHOPTIMIZATION) -Wl,-Map="$(@:%=%.map)"
SYSLIB_SUBDIRS +=
SYSLIBRARIES = -lrt -lpthread -lavahi-client -lavahi-common $(SYSLIBRARIESEXTRA)

ifndef SKIP_COVERAGE
LDFLAGS += -pg
SYSLIBRARIES += -lgcov
endif

ifdef RUN_TSAN
ARCHOPTIMIZATION += -fsanitize=thread
LDFLAGS += -fsanitize=thread
endif


EXTENTION =

