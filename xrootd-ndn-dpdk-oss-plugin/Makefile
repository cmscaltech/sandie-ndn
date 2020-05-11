################################################################################
# Named Data Networking plugin for XRootD				       #
# Copyright © 2019 California Institute of Technology		               #
#                                        				       #
# Author: Catalin Iordache <catalin.iordache@cern.ch>			       #
#                                        				       #
# This program is free software: you can redistribute it and/or modify	       #
# it under the terms of the GNU General Public License as published by	       #
# the Free Software Foundation, either version 3 of the License, or   	       #
# (at your option) any later version.    				       #
#                                        				       #
# This program is distributed in the hope that it will be useful,     	       #
# but WITHOUT ANY WARRANTY; without even the implied warranty of      	       #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       	       #
# GNU General Public License for more details.       			       #
#                                        				       #
# You should have received a copy of the GNU General Public License   	       #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.       #
################################################################################

GOCMD 	  := go
GOINSTALL := $(GOCMD) install
GOFLAGS   := #-a

CC 	:= gcc
CFLAGS  := -Werror -Wno-error=deprecated-declarations -m64 -O3 -g -march=native
rootdir = $(realpath .)
INCLUDE := -I/usr/local/include/dpdk -I/usr/include/dpdk -I$(rootdir)/../../

STATIC_COMMON	     := libxrdndndpdk-common.a
STATIC_COMMON_SRCDIR := xrdndndpdk-common

OBJDIR := obj
LIBDIR := libs

SRC := $(shell find $(STATIC_COMMON_SRCDIR)  -name '*.c')
OBJ := $(patsubst $(STATIC_COMMON_SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))


all: goget godeps consumer producer | $(STATIC_COMMON)

goget:
	go get -d -t ./...

godeps:
	rake cgoflags

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(STATIC_COMMON_SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

$(LIBDIR):
	mkdir -p $(LIBDIR)

$(STATIC_COMMON): $(OBJ) | $(LIBDIR)
	@ar rcs $(LIBDIR)/$(STATIC_COMMON) $^

producer: | $(STATIC_COMMON)
	$(GOINSTALL) $(GOFLAGS) ./xrdndndpdkproducer-cmd

consumer: | $(STATIC_COMMON)
	$(GOINSTALL) $(GOFLAGS) ./xrdndndpdkconsumer-cmd


.PHONY: all clean
clean:
	@$(RM) -rf $(OBJDIR) $(LIBDIR) ../../../bin/xrdndndpdkproducer-cmd ../../../bin/xrdndndpdkconsumer-cmd
	go clean -cache -testcache
