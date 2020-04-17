# Gemmini Device Driver

**Author:** Max Banister
**Email:** maxbanister@berkeley.edu

Companion software to [GemminiOS](https://github.com/MaxBanister/gemmini-os

## Build Steps with Linux

This module must be built statically with Linux, since it uses RISC-V architecture specific functions and not Linux's publicly exported functions. As such, it is reliant on the internal functions of the RISC-V port and may be unstable with future kernel releases. Last tested with 5.5.

### Building a Dynamically Loaded Module (Not Recommended)

It isn't currently possible to build a dynamically loadable module. However, for testing purposes there are reasons one may wish to do so, so I have included a `Makefile` for this purpose. To build, simply run:

```
make CONFIG_GEMMINI=m CROSS_COMPILE=riscv64-unknown-linux-gnu- all
```

To install it, `scp` the resultant `gemmini_driver.ko` to your RISC-V system, and run:

```
sudo modprobe gemmini_driver.ko
```

To check to make sure it's installed, type:

```
lsmod | grep gemmini
```

### Building a Statically Linked Module with Linux

The normal way of using the module will be building it in to a custom kernel. To do so, first follow the instructions at https://github.com/tactcomplabs/xbgas-tools/blob/master/README.md to clone Linux, create a BusyBox archive, and build Spike and Berkeley Bootloader (bbl) if you haven't already, then follow these steps:

1. In a scratch directory, clone the device driver.
```
git clone https://github.com/MaxBanister/gemmini-device-driver
```

2. Navigate to the top level of the repo, and run:
```
cp gemmini_driver.c /PATH_TO_LINUX/drivers/gemmini
cp Kconfig /PATH_TO_LINUX/drivers/gemmini
cp Makefile /PATH_TO_LINUX/drivers/gemmini
cp .config /PATH_TO_LINUX/drivers/gemmini
```

Then, make sure that the Kbuild system knows about our custom module:
```
sed -i '/^endmenu/i source \"drivers/gemmini/Kconfig\"\n' Kconfig
```


Then, `cd` into the Linux repository and prepare to build. If you have an `initramfs` archive you want to include for use with Spike, be sure to copy it over and modify the config to point to it. For example, run `make ARCH=riscv menuconfig` and navigate to `General Settings` -> `Initial RAM filesystem and RAM disk (initramfs/initrd) support` to point to your filesystem archive.

3. Once you have a filesystem set up, make with:
```
make -j16 ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu-
```

4. Press ENTER to confirm all the new config options if necessary.

5. At this point, you should have a `vmlinux` binary that is ready for use with Spike. Now, follow the instructions at https://github.com/MaxBanister/gemmini-os to clone Berkeley Bootloader and apply the patches to enable two heterogenous binaries to be launched. Once you are finished, you should have a `bbl` binary that is ready for use with Spike. To put this version of `bbl` on your path, you can run `make install` in that repo, although this will potentially override whatever version of `bbl` you have currently installed.

6. To test in Spike, simply run:
```
spike -p2 vmlinux
```

The `-p2` option feeds a two-hart device tree into Spike, which runs the modified version of bbl on both harts, which subsequently runs Linux on hart 0 and GemminiOS on hart 1. The modified version of bbl hardcodes the assumption that Gemmini firmware will run on hart 1, since this was designed for use with the Beagle chip. Changing this in GemminiOS should be facile, but it is not recommended.

This should drop you into the ash shell, from which you can run a test program that utilizes the device driver to send commands to the Gemmini hart.
