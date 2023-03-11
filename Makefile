include config.mk

Sources := $(shell find syncplay/ -name '*.cc')
Obj := $(subst syncplay/,,$(Sources:%.cc=%.o))

ifeq ($(os),windows)
all: bin/syncplay.exe
debug: bin/windows/debug/syncplay.exe
else
all: bin/syncplay
debug: bin/$(os)/debug/syncplay
endif

include scripts/$(os).mk
include scripts/build.mk
include scripts/test.mk

bin/$(Target): bin/$(os)/$(Target)
	ln -f $< $@

clean:
	rm -rf bin

doc/wprowadzenie.html: doc/wprowadzenie.md
	pandoc -o $@ $< -s --toc

.PHONY: clean all

# $(shell mkdir -p $(subst syncplay/,bin/$(os)/,$(shell find syncplay/* -type d)))
$(shell mkdir -p bin/$(os)/replxx/)
# $(shell mkdir -p $(subst syncplay/,bin/$(os)/debug/,$(shell find syncplay/* -type d)))
