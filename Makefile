
export cross=/home/ncast/buildroot/output/host/opt/ext-toolchain/bin/arm-none-linux-gnueabi-
export sysroot=/home/ncast/buildroot/output/host/usr/arm-unknown-linux-gnueabi/sysroot/
export fs=/home/ncast/simplefs-3530

inc := ${sysroot}/usr/include
lib := ${sysroot}/usr/lib
CC := $(cross)gcc
CFLAGS := -I. -O2 -Wall -I${inc}/libxml2 #-Werror
LDFLAGS := --sysroot=${sysroot} -lpthread -lm -lxml2  
objs := algo.o cam.o comm.o net.o ui.o utils.o mcu.o

#all:
#	$(cross)ld -V
#a: a.o 

test-teacher: teacher
	sudo cp teacher ${fs}/root
	sudo su ncast -c \
		"cd ~; make open-and-telnet-3530 \
		cmd='cd /root/; ./teacher; \
		echo; echo testeof' eof='^testeof'"

teacher: $(objs) teacher.o

student: $(objs) student.o

sersrv: $(objs) sersrv.o

clean:
	rm -rf *.o

