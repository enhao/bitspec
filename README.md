# BitSpec

[![Build Status](https://travis-ci.org/enhao/bitspec.svg?branch=master)](https://travis-ci.org/enhao/bitspec)


BitSpec is a devmem helper, support following functions by configuration:

* display the bits description
* transform the notation system of bits for display
* verify the bits value

## Installation

```
$ git clone --recursive https://github.com/enhao/bitspec.git
$ cd bitspec
$ make
```

To cross compile, just set CC on the make command-line.

```
$ make CC=arm-linux-gcc
```

## Usage

```
$ bitspec  CONF  HEX
```

## Configuration

The configuration is INI format. A setting contains a section heading that provides a name or description for specified bits, and the following lines contain the bits and the notiation system for dispaly. the check value is optional, and the format is must be same as the notiation system. For example,

```
[Revision Number]
bits = 3:0
notation = 10
checkvalue = 1
```

Revsion Number is bit 3 to bit 0, decimal display, and valid value is 1.

```
[Link Status]
bits = 2

# Notation System:
#   2  Binary
#  10  Decimal
#  16  Hexadecimal
notation = 2
```

Link Status is bit 2, binary display.

## Example

Read PHY regsiter 3 by devmem

```
$ devmem 0x100A03C0 32 0x00120103 && devmem 0x100A03C0 32 0x00020103 && devmem 0x100A03C4
0x0D910000
```

Edit the configuration with spec

```
# register3.conf
[Revision Number]
bits = 19:16
notation = 10

[Model Number]
bits = 25:20
notation = 2
checkvalue = 011001

[OUI]
bits = 31:26
notation = 2
checkvalue = 000011
```

Pipe the output to bitspec

```
$ bitspec register3.conf 0x0D910000
Input: 0XD910000
[  VIEW]  Revision Number:  bit[19:16] = 1
[    OK]  Model Number:  bit[25:20] = 011001, checkvalue = 011001
[    OK]  OUI:  bit[31:26] = 000011, checkvalue = 000011
```

or 

```
$ devmem 0x100A03C0 32 0x00120103 &&  devmem 0x100A03C0 32 0x00020103 && devmem 0x100A03C4 | xargs bitspec register3.conf
Input: 0XD910000
[  VIEW]  Revision Number:  bit[19:16] = 1
[    OK]  Model Number:  bit[25:20] = 011001, checkvalue = 011001
[    OK]  OUI:  bit[31:26] = 000011, checkvalue = 000011
```


