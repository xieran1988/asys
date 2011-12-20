
pwd ?= $(shell pwd)
appname := $(shell basename $(pwd))
bakfile := /root/$(appname)-$(shell date +%F-%R.tar.bz2 | sed 's,:,,')
uibakfile := /root/trackui-$(shell date +%F-%R.tar.bz2 | sed 's,:,,')

armcc := arm-none-linux-gnueabi-gcc

cflags := -I$(aroot)/include -I. -O2 -Wall #-Werror
inc := $(wildcard $(aroot)/include/*.h) $(wildcard $(pwd)/*.h)

srcs := $(wildcard $(pwd)/*.c $(aroot)/*.c)
objs := $(srcs:.c=.o)
armobjs := $(srcs:.c=_arm.o)
armexe := $(pwd)/arm$(appname)

all: $(armexe)

runarm: $(armexe)
	echo "$(armexe)" > $(aroot)/autorun
	armsync $(armexe) $(aroot)/autorun
	armkillall
	armsh "$(armexe)"

$(armexe): $(armobjs)
	$(armcc) -lm -lpthread $^ -o $@

%_arm.o: %.c $(inc)
	$(armcc) -c $(cflags) $< -o $@
	
backup:
	tar -jcf $(bakfile) .
	du -sh $(bakfile)
	tar -jcf $(uibakfile) /root/Track
	du -sh $(uibakfile)

clean:
	-rm -rf $(pwd)/*.o $(armexe)


