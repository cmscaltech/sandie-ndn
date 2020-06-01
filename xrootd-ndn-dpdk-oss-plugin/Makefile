##############################################################################
# Named Data Networking plugin for XRootD                                    #
# Copyright © 2019-2020 California Institute of Technology                   #
#                                                                            #
# Author: Catalin Iordache <catalin.iordache@cern.ch>                        #
#                                                                            #
# This program is free software: you can redistribute it and/or modify       #
# it under the terms of the GNU General Public License as published by       #
# the Free Software Foundation, either version 3 of the License, or          #
# (at your option) any later version.                                        #
#                                                                            #
# This program is distributed in the hope that it will be useful,            #
# but WITHOUT ANY WARRANTY; without even the implied warranty of             #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              #
# GNU General Public License for more details.                               #
#                                                                            #
# You should have received a copy of the GNU General Public License          #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.     #
##############################################################################

GOCMD 	  := go
GOINSTALL := $(GOCMD) install
GOFLAGS   := #-a

export CGO_CFLAGS_ALLOW := '.*'

rootdir := $(realpath .)

CC 	:= gcc
CFLAGS   := -Werror -Wno-error=deprecated-declarations -m64 -O3 -g -march=native
CXXFLAGS := -std=c++17 $(CFLAGS)
INCLUDE  := -I$(rootdir)/../../

OBJDIR := obj
LIBDIR := libs

all: goget godeps consumer producer | $(STATIC_COMMON)

goget:
	go get -d -t ./...

godeps:
	rake cgoflags

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

# Build libcommon static lib
STATIC_COMMON	     := libcommon.a
STATIC_COMMON_SRCDIR := xrdndndpdk-common
STATIC_COMMON_SRCS   := $(shell find $(STATIC_COMMON_SRCDIR)  -name '*.c')
STATIC_COMMON_OBJS   := $(patsubst $(STATIC_COMMON_SRCDIR)/%.c, $(OBJDIR)/%.o, $(STATIC_COMMON_SRCS))

$(OBJDIR)/%.o: $(STATIC_COMMON_SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

$(STATIC_COMMON): $(STATIC_COMMON_OBJS) | $(LIBDIR)
	@ar rcs $(LIBDIR)/$(STATIC_COMMON) $^

# Build libfilesystem static lib
LOGGER_SRCDIR := xrdndndpdk-logger
LOGGER_SRCS   := $(shell find $(LOGGER_SRCDIR)  -name '*.cc')
LOGGER_OBJS   := $(patsubst $(LOGGER_SRCDIR)/%.cc, $(OBJDIR)/%.o, $(LOGGER_SRCS))

STATIC_FILESYSTEM 	 := libfilesystem.a
STATIC_FILESYSTEM_SRCDIR := xrdndndpdkproducer
STATIC_FILESYSTEM_SRCS 	 := $(shell find $(STATIC_FILESYSTEM_SRCDIR)  -name 'filesys*.cc')
STATIC_FILESYSTEM_OBJS 	 := $(patsubst $(STATIC_FILESYSTEM_SRCDIR)/filesys%.cc, $(OBJDIR)/filesys%.o, $(STATIC_FILESYSTEM_SRCS))

$(OBJDIR)/%.o: $(LOGGER_SRCDIR)/%.cc | $(OBJDIR)
	clang++ $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/filesys%.o: $(STATIC_FILESYSTEM_SRCDIR)/filesys%.cc | $(OBJDIR)
	clang++ $(CXXFLAGS) -c $< -o $@

$(STATIC_FILESYSTEM): $(LOGGER_OBJS) $(STATIC_FILESYSTEM_OBJS) | $(LIBDIR)
	@ar rcs $(LIBDIR)/$(STATIC_FILESYSTEM) $^

producer: | godeps $(STATIC_COMMON) $(STATIC_FILESYSTEM)
	$(GOINSTALL) $(GOFLAGS) ./xrdndndpdkproducer-cmd

consumer: | godeps $(STATIC_COMMON)
	$(GOINSTALL) $(GOFLAGS) ./xrdndndpdkconsumer-cmd

.PHONY: all clean
clean:
	@$(RM) -rf $(OBJDIR) $(LIBDIR) ../../../bin/xrdndndpdkproducer-cmd ../../../bin/xrdndndpdkconsumer-cmd
	go clean -cache -testcache
