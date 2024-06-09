# FOAM - Forth, Objects and Moar

FOAM is an extension of the FORTH programming language created by Charles Moore.

FOAM deviates from standard FORTHs by accepting additional complexity in the pursuit of improved approachability and productivity for the majority of current programmers.

FOAM defines a virtual machine which executes 32-bit opcodes, which have an 8-bit opcode-type, and a 24-bit value field.  This is somewhat related to 'indirect threaded' implementations.

A guiding idea behind the design of the virtual machine for FOAM is using fewer operators to complete a task, this is intended to offset the extra cost of the inner interpreter, compared to direct threaded implementations.

FOAM can be used in 32-bit or 64-bit modes on Windows, Macintosh, Linux, or Raspberry-pi.

## ANSI Forth compatability
ANSI Forth is not central to this implementation, but it can be configured to have complete ANSI 2012 Forth compatability.

kFFAnsi -> features

## Syntax extensions

### Comments
You can do end-of-line comments with '\ ' as in ansi forth
Inline comments can be done with '/* ...STUFF... */', these must be on the same line.
Note that */ by itself is the forth multiply/divide operator.
You can turn on ANSI compliant comments which uses parentheses by ***TODO***

### Literals

#### Integers
You can specify that an integer constant is 64-bit by appending an 'L' or 'l' to it.

On 64-bit systems this usually doesn't make a difference.

#### Hexadecimal integers
Hex literals can be specified by starting them with '0x'.
Hex literals can end in 'L' or 'l' to specify a 64-bit value.

#### Floating Point
A string of decimal digits which contains a period and an optional trailing exponent.
Examples:
1.45        // valid 32-bit fp literal
1.45l       // valid 64-bit fp literal
12.3e2      // valid 32-bit fp literal with value 1230.0
123e2       // not an fp literal, does not contain a period

#### Character
Character constants have backticks around them and can contain 1 to 8 characters.
Special character values can be entered with backslashes, much like C/C++.

#### String
String constants have double quotes around them.
Most operators use C-style null terminated strings, with a single pointer to the first character on TOS.

#### User defined literals
Users can add literals of their own by adding an operator to the 'literals' vocabulary

TODO: show how
### Add and Subtract
You can do add to (or subtract from) the value on TOS:

0x55+ // add 85 to value on TOS

3-  // subtract 3 from value on TOS

### Array accessor
You can compute the address of an element of an array:

    INDEX ARRAY_BASE_ADDRESS TYPENAME[]

TYPENAME can be the name of a structure, class or native type.

### Structure accessors
You can define structures like so:

struct: rgb
  ubyte red  ubyte green   ubyte blue
;struct

struct vertex
  rgb color
  int x   int y   int z
  ptrTo vertex myBuddy
;struct

If you define a field which is a pointer to a struct, the TODO
### Parentheses

### Variable action suffixes

### String

### 64-bit versus 32-bit

## Native Datatypes

### Integer types
#### byte
#### ubyte
short
ushort
int
uint
long
ulong
cell and ucell

### Floating point types
float
sfloat

### Strings

## Virtual Machine

## Supported Platforms

The FOAM builds have 2 main flavors:
o generic: builds where all builtin ops are defined in C++
o native: builds where core ops are defined in assembler

### Windows
Windows is the platform where most of the development of FOAM is done.

There are native builds for both 32-bit and 64-bit.

### Linux
There are native x86 builds for both 32-bit and 64-bit.
The 32-bit native build hasn't been used or tested in a while.

### Raspberry Pi
The only build is the native 32-bit build.

### Macintosh OSX
OSX is mostly the same as Linux.

OSX hasn't been updated or tested in a long while, there was a native x86 32-bit build, but it needs to be brought up to date.

There is a native x86 64-build which is up to date.

There is no native ARM/Apple silicon 64-bit build.

# Objects and Classes
This Forth has an object system, whose features include:
o many builtin convenience classes including strings, arrays, maps and so on
o (mostly) automatic reference counting

## how .new works

## how super works

# Threads and Fibers
A thread corresponds to a system thread - multiple threads can be using the processor at any moment.
Each Forth Thread has one or more Fibers - Each Thread has one active Fiber at any time.
Each Fiber has its own Forth state, including parameter stack, return stack, and interpreter pointer.
Fibers can give up being active in the thread by doing a 'yield', or can become inactive by doing any operation which will require the Fiber to wait for something.

# Client/Server

* Item 1
* Item 2
  * Item 1a
  * Item 2a
     * Item 1b
     * Item 2b

*Italics*

_This will also be italic_

**Bold text**

__This will also be bold__

***Bold and Italics***

_You **can** combine them_

~~Striked Text~~

***~~Italic, bold, and strikethrough1~~***


