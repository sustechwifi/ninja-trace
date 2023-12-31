make
sudo insmod config_etm.ko 
sudo dmesg

cd force_rmmod
sudo insmod force_rmmod.ko modname=config_etm
sudo rmmod config_etm
sudo rmmod force_rmmod
cd ..




rm ./.config_etm.ko.cmd
rm ./.config_etm.mod.o.cmd
rm ./config_etm.ko
rm ./.config_etm.o.cmd
rm ./config_etm.mod
rm ./.config_etm.mod.cmd
rm ./config_etm.mod.c
rm ./config_etm.mod.o
rm ./config_etm.o
rm ./Module.symvers
rm ./modules.order

