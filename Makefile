
plat ?= 3530
libs := libxml-2.0
include ${parentsdir}/top.mk

objs := algo.o cam.o comm.o net.o ui.o utils.o mcu.o

test-teacher: teacher
	$(call targetsh, "./teacher")

test-lighttpd:
	$(call targetsh, "lighttpd -f lighttpd.conf -D")

$(objs): algo.h

dump-teacher: teacher
	$(OBJDUMP) -d teacher

teacher: $(objs) teacher.o

student: $(objs) student.o

sersrv: $(objs) sersrv.o

clean:
	rm -rf *.o teacher student sersrv

