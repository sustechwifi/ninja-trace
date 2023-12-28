make
sudo insmod config_etm.ko 
sudo dmesg

cd force_rmmod
make
insmod force_rmmod.ko modname=config_etm
rmmod config_etm
rmmod force_rmmod

cd ..
rm -r force_rmmod/.tmp_versions
rm ./force_rmmod/.cache.mk
rm ./force_rmmod/.force_rmmod.o.cmd
rm ./force_rmmod/.force_rmmod.ko.cmd
rm ./force_rmmod/.force_rmmod.mod.o.cmd
rm ./force_rmmod/force_rmmod.ko
rm ./force_rmmod/force_rmmod.mod.c
rm ./force_rmmod/Module.symvers
rm ./force_rmmod/modules.order
rm ./force_rmmod/force_rmmod.mod.o
rm ./force_rmmod/force_rmmod.o

rm -r ./.tmp_versions
rm ./.cache.mk
rm ./.config_etm.ko.cmd
rm ./.config_etm.mod.o.cmd
rm ./config_etm.ko
rm ./.config_etm.o.cmd
rm ./config_etm.mod.c
rm ./config_etm.mod.o
rm ./config_etm.o
rm ./Module.symvers
rm ./modules.order

