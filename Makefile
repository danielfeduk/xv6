CFLAGS= -MD

QEMU=qemu-system-x86_64 -no-reboot

all: fs.img kern

user:
	$(MAKE) -C user

kern:
	$(MAKE) -C kern xv6.img

kern/xv6.img: kern

.PHONY: user kern

fs.img: #mkfs README user
	#$(eval TMP := $(shell mktemp -d))
	#cp user/out/* $(TMP)
	#cp README $(TMP)
	#cd $(TMP) && $(CURDIR)/mkfs $(CURDIR)/fs.img *
	#rm -rf $(TMP)

clean: 
	rm -f *.o *.d fs.img mkfs .gdbinit
	$(MAKE) -C user clean
	$(MAKE) -C kern clean

# run in emulators

bochs : fs.img kern/xv6.img
	if [ ! -e .bochsrc ]; then ln -s dot-bochsrc .bochsrc; fi
	bochs -q

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 2
endif
QEMUOPTS = -machine pc -drive file=kern/xv6.img,index=0,media=disk,format=raw  -smp sockets=$(CPUS),cores=1,threads=1 -m 512 -serial mon:stdio $(QEMUEXTRA)


qemu: fs.img kern/xv6.img
	$(QEMU) $(QEMUOPTS)

qemu-memfs: xv6memfs.img
	$(QEMU) -drive file=xv6memfs.img,index=0,media=disk,format=raw -smp sockets=$(CPUS),cores=1,threads=1 -m 256

qemu-nox: fs.img kern/xv6.img
	$(QEMU) -nographic $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: fs.img kern/xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-nox-gdb: fs.img kern/xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUGDB)

-include *.d
