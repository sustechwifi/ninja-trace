obj-m += config_etm.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

EXTRAVERSION =

if [ "$karch" = "arm" ] && [ -f "arch/arm/kernel/module.lds" ]; then
    copy_mkdir "$target_dir" "./arch/arm/kernel/module.lds"
fi
