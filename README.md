libopencm3-skel
---------------

This is a minimal LED blink application that uses [libopencm3](https://github.com/libopencm3/libopencm3) and
targets a Blue Pill board (clone) based on the STM32F103C8T6 microcontroller. The project name has nothing to
do with Halloween decorations; instead, it's much closer to what `/etc/skel` has been traditionally used for
in the Linux world. It's meant to be used as a template or base for other applications using the same library.

There are many other templates out there, including [the official one](https://github.com/libopencm3/libopencm3-template)
from libopencm3. However, this is _my_ template, and it also serves as a place to keep notes about the build
environment and various configuration options.

In a true open source spirit, the assumption is that the Blue Pill board is attached to an inexpensive variant
of Raspberry Pi (such as Model 1 or Pi Zero) over the SWD interface, and the microcontroller is programmed
using [OpenOCD](https://openocd.org/). This is not only a cost effective and feature rich way to program and
debug the microcontroller, but also a very good option for small low-power embedded systems, where the
Raspberry Pi runs the heavy workload leveraging the flexibility and power of Linux, while the Blue Pill is
used as a sidekick for real-time applications or hardware features that are not available on the Raspberry Pi
(such as ADC). Another benefit of this approach is In-System Programming (ISP) of the STM32 microcontroller.

# Hardware Configuration

For programming/debugging, the recommended connections between the Raspberry Pi and Blue Pill are:

| Pi GPIO# | Pi Pin# | STM32 pin |
|:--------:|:-------:|:---------:|
|    22    |   15    |   SWCLK   |
|    23    |   16    |   SWDIO   |
|    24    |   18    |   NRST    |

The connections are defined in `openocd/rpi1.cfg` and can be reconfigured. Any GPIO pins can be used on the
Raspberry Pi because OpenOCD drives the pins directly (bit-banging).

Earlier Raspberry Pi Models up to and including Model 4 have only one "true" UART interface, and in embedded
applications it is typically used for the Pi's serial console. That UART interface is the PL011, and it is a
capable, broadly 16550-compatible UART. The other one is called "mini UART", and there are caveats associated
with enabling it properly, such as locking down the VPU clock rate (which increases power consumption), and
disabling Bluetooth (on models that have it).

However, there is another option that uses the best of both worlds: the firmware based UART (rpi-fw-uart).It
uses one of the VPU cores to implement a bit-banging UART driver, leaving the CPU free for regular Linux
workloads and the "mini UART" for Bluetooth. This is documented in `/boot/overlays/README` on any modern
RPi OS installation (search for "rpi-fw-uart" in the file).

As per the documentation, the options below must be set in `/boot/firmware/config.txt`:
```
dtparam=audio=off
isp_use_vpu0=1
dtoverlay=rpi-fw-uart,txd0_pin=17,rxd0_pin=18
```

Note: `dtparam=audio` is typically already defined but set to `on`. The txd/rxd pins are arbitrary (any pins
can be used, since it's a bit-bang UART implementation), and the pins above are simply my choice.

**CAVEAT**: In many embedded applications the display output is not used, and in that case the GPU memory is
set to a minimum (e.g. `gpu_mem=16` in `config.txt`), since it "eats into" the available RAM. However, doing
that has the side effect of enabling the cut-down firmware, which lacks the software UART driver. See the
[documentation](https://www.raspberrypi.com/documentation/computers/config_txt.html) for details. **Make sure
`gpu_mem` is set to at least 32 for rpi-fw-uart to work.**

With this configuration in place, USART1 on the Blue Pill can be wired to the Raspberry Pi as described below,
to be able to access the STM32 "console" on the Pi.

| Pi GPIO# | Pi Pin# | STM32 pin |
|:--------:|:-------:|:---------:|
|    17    |   11    |    PA10   |
|    18    |   12    |    PA9    |

Command example on the Pi side:
```
picocom -b 115200 -e o /dev/ttyRFU0
```

# Build Configuration

## MCU type

Build options are configured in `firmware/Makefile`. That's where the target device (MCU) is configured. The
default is STM32F103C8T6 (the MCU used on the original Blue Pill). However, Blue Pill clones come in two
variants, and the other variant is STM32F103C6T6, which is even cheaper. The two MCU types are very similar,
and the table below summarizes the main differences.

| Difference | STM32F103C6T6 | STM32F103C8T6 |
|------------|:-------------:|:-------------:|
| Flash      |      32K      |      64K      |
| RAM        |      10K      |      20K      |
| Timers     |       3       |       4       |
| UARTs      |       2       |       3       |

## Linker Options

libopencm3 relies on [newlib](https://sourceware.org/newlib/) as the standard C library. Not all embedded CPUs
are the same, and some have much more flash than others, and some have a FPU while others don't. On platforms
where a FPU is not available, such as the STM32F103, floating point is implemented in software, but that takes
up a considerable amount of flash space (relative to the little amount of flash these MCUs have).

### Linker Garbage Collection

A very common approach to reducing the final image size (and therefore the amount of required flash) is to:
* Place each function and global variable in its own section at compile time. This is achieved through the
  `-ffunction-sections` and `-fdata-sections` compiler flags. It is implemented in the libopencm3 make files.
* Instruct the linker to drop the unused sections at link time. This is achieved through the `-Wl,--gc-sections`
  linker flag. It is **not** implemented in the libopencm3 make files, so it **must** be configured in the
  application make file to reduce the final image size.

### Floating Point Code and printf() Functions

On MCUs that don't have a FPU (such as the STM32F103 on Blue Pill), the software emulation code is not pulled
into the final image by default. It is only pulled when there is at least one piece of code that uses floating
point arithmetic. The tooling is smart enough to do that. But the **big caveat** is that by default, newlib
provides a full-fledged implementation of the printf() family of functions. These are not only much larger
themselves, but also pull in the FPU emulation code because of their internal handling of float related format
specifiers.

The result is that a barebones application like this, which has very little code itself, generates an image
that is too large to fit in the 32K of flash of the C6T6. If printf() is still needed, there is a way to "ask"
the library (by means of linker flags) to provide a trimmed-down implementation without floating point
support. The printf() functions themselves are smaller, and also they don't pull in the FPU emulation code,
overall resulting in a much smaller image. This is achieved through the `-specs=nano.specs` linker flag,
which also substitutes the dynamic allocator implementation with a much smaller one.

It is worth noting that libopencm3 itself doesn't pull in the FPU emulation code, so it's all about what the
application uses.

### Size Analysis and Comparison

The command below can be used to analyze the resulting image size.
```
arm-none-eabi-size firmware/firmware.elf
```

This is the default, when none of the linker flags described above is used.
```
   text	   data	    bss	    dec	    hex	filename
  35924	   1760	    412	  38096	   94d0	firmware/firmware.elf
```

By adding `-specs=nano.specs` to use the no floating point version of printf(), the size becomes:
```
   text	   data	    bss	    dec	    hex	filename
   9684	    104	    340	  10128	   2790	firmware/firmware.elf
```

By adding `-Wl,--gc-sections` to enable link time garbage collection, the size becomes:
```
   text	   data	    bss	    dec	    hex	filename
   5700	    104	    336	   6140	   17fc	firmware/firmware.elf
```

By not using printf() at all (commenting out the printf() line in the application), the size becomes:
```
   text	   data	    bss	    dec	    hex	filename
   1780	     12	      0	   1792	    700	firmware/firmware.elf
```

# Building

## Prerequisites

```
ln -s ../libopencm3
```

Fedora (cross-compiling on the work PC):
```
dnf install arm-none-eabi-gcc arm-none-eabi-newlib
```

Raspberry Pi OS (cross-compiling locally on the Pi):
```
apt install
```

## Starting the build
```
make
```

# Flashing

- Copy `openocd/rpi1.cfg` to the Raspberry Pi board (unless building locally on the Pi).
- Make sure OpenOCD is installed on the Pi.
- Run the command below on the Pi.

```
sudo openocd -c "bindto 0.0.0.0" -f rpi1.cfg -c "transport select swd" -c "adapter speed 1000" -f target/stm32f1x.cfg
```

- Run locally (on the work PC or on the Pi if cross-compiling locally):
```
telnet <pi_host> 4444
program firmware.elf verify reset
```

# Debugging

OpenOCD implements its own gdb server, which makes it very easy/convenient to debug the firmware while it's
running live on the Blue Pill. Use the commands below to start a gdb session and attach to the OpenOCD gdb
server. This should be run locally (on the work PC or on the Pi if cross-compiling locally):
```
gdb firmware/firmware.elf
(gdb) target extended-remote <pi_host>:3333
```
