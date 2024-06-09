# Objects
This Forth has an object system, whose features include:
o many builtin convenience classes including strings, arrays, maps and so on
o (mostly) automatic reference counting


single inheritance

multiple interfaces

all classes ultimately derive from Object

Object class has 2 members:
  methods pointer
  reference count

super
  how it works

reference counting
  assignment to an object var normally increments refcount
  refcounting is atomic
  adding an object to a container object also increments refcount
  assignment without incrementing refcount
  clearing references
  unref

how new works
  new:

makeObject and mko

life cycle of an object - or does this belong under refcounting?

object variable suffixes and varops


## how .new works
  new:
  ;


## how super works

