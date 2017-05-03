MAKEFLAGS += -rR --include-dir=$(CURDIR) --no-print-directory

PHONY := _all
_all:

$(CURDIR)/Makefile Makefile: ;

INCLUDE	:= -Iinclude

CROSS_COMPILE	:= /home/kart/Downloads/buildroot-2017.02/output/host/usr/bin/arm-none-linux-gnueabi-

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
RM = rm -f

CFLAGS	+= -mcpu=cortex-a9 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -O2
LDFLAGS += -O2 -std=c++11
LIBS = -lv4l2 -ljpeg -lv4lconvert

libs-y	:= lib/
libs-y	:= $(sort $(libs-y))
dirs	:= $(patsubst %/,%,$(libs-y))
libs-y	:= $(patsubst %/,%/build-in.o,$(libs-y))

srcs	:= $(wildcard *.c)
objs	:= $(patsubst %.c,%.o,$(srcs))

$(foreach v, $(dirs), $(eval $(v)/src := $(wildcard $(v)/*.c)))
$(foreach v, $(dirs), $(eval $(v)/obj := $(patsubst %.c,%.o,$(value $(v)/src))))
$(foreach v, $(dirs), $(eval $(v)/build-in.o : $(value $(v)/obj)))

_all : lib/build-in.o $(objs) barcode_v4l2

%.o : %.c
	@echo "  $(notdir $(CC)) $@"
	@$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $< -std=gnu99

$(libs-y) : FORCE
	@echo "  $(notdir $(AR)) $@"
	@$(AR) -rcs $@ $(filter-out FORCE,$^)

barcode_v4l2 : FORCE image_process.o
	@echo "  $(notdir $(CXX)) $@"
	@$(CXX) -o $@ $(objs) $(libs-y)  $(LDFLAGS) -lv4l2 image_process.o $(LIBS) -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lzbar

image_process.o: image_process.cpp
	@echo "  $(notdir $(CXX)) $@"
	@$(CXX) -o $@ $< -c -std=c++11

PHONY += scp
scp:
	scp barcode_v4l2 root@192.168.0.125:~

PHONY += FORCE
FORCE:

PHONY += clean
clean:
	$(RM) $(foreach v, $(dirs), $(wildcard $(v)/*.o))
	$(RM) $(objs)
	$(RM) barcode_v4l2
	$(RM) image_process.o

.PHONY : $(PHONY)

