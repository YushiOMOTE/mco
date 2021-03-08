LD   = $(PREFIX)ld
CC   = $(PREFIX)gcc
TFTPDIR = ~/tftpboot
MACRO_OPS = $(addprefix -D, $(MACROS))

CFLAGS = -m64 -mno-red-zone -nostdinc -g -O -ffreestanding -fno-builtin -fno-stack-protector -fno-strict-aliasing -Wall -Werror -Iinclude $(MACRO_OPS)

target=kvi.bin
tftpdir=./
tftpdir-host=$(TFTPDIR)

objs-s = loader.o
objs-c = init.o main.o timer.o
objs-c += alloc.o page.o
objs-c += putchar.o printf.o
objs-c += acpi.o apic.o
objs-c += drivers/pci.o drivers/pmt.o drivers/hpet.o
objs-c += drivers/ixgbe.o

objs = $(objs-s) $(objs-c)
srcs = $(patsubst %.o,%.c,$(objs-c)) $(patsubst %.o,%.S,$(objs-s))

clean-files  = $(objs) *~ include/*~ $(target) .depends

msg-install  = "  INSTALL $(target) to $(tftpdir-host)"
msg-clean    = "  CLEAN   $(clean-files)"
msg-link     = "  LD      $@"
msg-compile  = "  CC      $<"
msg-assemble = "  AS      $<"

default:
	@$(MAKE) $(target)

install:
	@$(MAKE) $(target)
	@cp $(target) $(tftpdir-host)
	@echo $(msg-install)

clean:
	@rm -rf $(clean-files)
	@echo $(msg-clean)

depends: .depends

.depends: $(srcs)
	@rm -f $@
	@$(CC) $(CFLAGS) -MM $^ > $@

-include .depends

$(target): kvi.ld $(objs)
	@$(LD) -o $@ -T $^
	@echo $(msg-link)

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo $(msg-compile)

%.o: %.S
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo $(msg-assemble)

run:
	@$(MAKE) $(target)
	@qemu-system-x86_64 -vnc :8 -gdb tcp::1234,server,nowait -display none -m 1G -smp 4 -netdev user,id=net0,tftp=$(tftpdir),bootfile=$(target) -device e1000,netdev=net0 -enable-kvm -cpu host,+x2apic -chardev stdio,mux=on,id=stdio,signal=on -mon chardev=stdio,mode=readline,default -device isa-serial,chardev=stdio

