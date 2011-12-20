
export cross=/home/ncast/buildroot/output/host/opt/ext-toolchain/bin/arm-none-linux-gnueabi-
CC = $(cross)gcc
CFLAGS := -I. -O2 -Wall #-Werror
objs := algo.o cam.o comm.o net.o xml.o utils.o

#a: a.o 

teacher: $(objs) teacher.o

student: $(objs) student.o

sersrv: $(objs) sersrv.o

clean:
	rm -rf *.o


 
