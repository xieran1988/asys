
export cross=/home/ncast/buildroot/output/host/opt/ext-toolchain/bin/arm-none-linux-gnueabi-
export sysroot=/home/ncast/buildroot/output/host/usr/arm-unknown-linux-gnueabi/sysroot/
export fs=/home/ncast/simplefs-3530

inc := ${sysroot}/usr/include
CC := $(cross)gcc
dump := $(cross)objdump
CFLAGS := \
	-I. -O3 -P \
	-mfloat-abi=softfp -mfpu=neon -ftree-vectorize \
	-mcpu=cortex-a8 -mtune=cortex-a8 -Wall \
	-fPIC \
	-I${inc}/libxml2 -I${inc} -Wall 
LDFLAGS := --sysroot=${sysroot} -lpthread -lm -lxml2 -ljpeg
objs := algo.o cam.o comm.o net.o ui.o utils.o mcu.o

#all:
#	$(cross)ld -V

test-teacher: teacher
	sudo cp teacher ${fs}/root
	sudo cp index.html ${fs}/root
	sudo su ncast -c \
		"cd ~; make open-and-telnet-3530 \
		cmd='cp index.html /tmp; ./teacher; \
		echo; echo testeof' eof='^testeof'"

test-lighttpd:
	sudo cp lighttpd.conf ${fs}/root
	sudo su ncast -c \
		"cd ~; make open-and-telnet-3530 \
		cmd='lighttpd -f lighttpd.conf -D ; \
		echo; echo testeof' eof='^testeof'"

$(objs): algo.h

dump-teacher: teacher
	$(dump) -d teacher

mysrc.so: CFLAGS += -DMYSRC
mysrc.so: cam.o utils.o
	$(CC) -shared -o $@ $^
	cp $@ ~/emafs2/usr/lib

teacher: $(objs) teacher.o

student: $(objs) student.o

sersrv: $(objs) sersrv.o


clean:
	rm -rf *.o teacher student sersrv

