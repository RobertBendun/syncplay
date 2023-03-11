MAKEFLAGS="-j $(grep -c ^processor /proc/cpuinfo)"

MAJOR := 0
MINOR := 1
PATCH := 0
COMMIT := gc$(shell git rev-parse --short HEAD 2>/dev/null)

ifeq ($(COMMIT),gc)
	COMMIT = gcunknown
endif

VERSION := $(MAJOR).$(MINOR).$(PATCH)-dev+$(COMMIT)

CXXFLAGS:=$(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror=switch -Werror=return-type -Werror=unused-result
CPPFLAGS:=$(CPPFLAGS) -DSyncplay_Version='"$(VERSION)"' \
	-Ilib/expected/ -I. -Ilib/rtmidi/ -Ilib/link/include -Ilib/asio/include/ -Ilib/replxx/include  -DREPLXX_STATIC
LDFLAGS=-flto
LDLIBS= -lpthread

RELEASE_FLAGS=-O2
DEBUG_FLAGS=-O0 -ggdb -fsanitize=undefined -DDebug

ifeq ($(shell uname),Darwin)
os=macos
else
os=linux
endif
