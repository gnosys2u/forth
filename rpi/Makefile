#####################################################################################

# you may need to install readline with this command:
# sudo apt-get install libreadline6-dev libreadline6

#.PHONY:	all clean depends

OBJ = objasm
MAINSRC = .
LIBSRC = ../ForthLib

CC = g++
ASM = as
CDEBUG = -g

DEFINES = -DRASPI -DLINUX -DASM_INNER_INTERPRETER -DINCLUDE_TRACE -DATOMIC_REFCOUNTS -DPRINTF_SUBS_IN_ASM -DSTORAGE_LONGS=4000000

# add '-save-temps=obj' to see intermediate assembly files
CFLAGS = -c $(CDEBUG) $(DEFINES) $(WARNING_FLAGS) -std=c++17 -marm

WARNING_FLAGS = -Wall -Wno-format -Wno-reorder -Wno-unused-variable -Wno-sign-compare -Wno-conversion-null -Wno-pointer-arith
CPPFLAGS = $(CFLAGS) -I $(LIBSRC)

ASMFLAGS = --defsym ASM_INNER_INTERPRETER=1 --defsym PRINTF_SUBS_IN_ASM=1

LDFLAGS = $(CDEBUG)
LD_LIBRARIES = -L $(OBJ) -lforth -lm -lc -lstdc++ -ldl -lpthread -lreadline

SRCS = \
	Shell.cpp \
	Engine.cpp \
	OuterInterpreter.cpp \
	ForthInner.cpp \
	InputStack.cpp \
	InputStream.cpp \
	BlockInputStream.cpp \
	BufferInputStream.cpp \
	ConsoleInputStream.cpp \
	ExpressionInputStream.cpp \
	FileInputStream.cpp \
	Vocabulary.cpp \
	BuiltinClasses.cpp \
	Extension.cpp \
	ShowContext.cpp \
	StructVocabulary.cpp \
	StructCodeGenerator.cpp \
	ForthOps.cpp \
	Server.cpp \
	Client.cpp \
	OpcodeCompiler.cpp \
	BlockFileManager.cpp \
	ParseInfo.cpp \
	Pipe.cpp \
	Forgettable.cpp \
	Thread.cpp \
	ObjectReader.cpp \
	MemoryManager.cpp \
	kbhit.cpp \
	OArray.cpp \
	ODeque.cpp \
	OList.cpp \
	OMap.cpp \
	ONumber.cpp \
	OSocket.cpp \
	OStream.cpp \
	OString.cpp \
	OSystem.cpp \
	ClassVocabulary.cpp \
	LocalVocabulary.cpp \
	DLLVocabulary.cpp \
	NativeType.cpp \
	TypesManager.cpp \
	NumberParser.cpp \
	UsingVocabulary.cpp \
	VocabularyStack.cpp \
	ControlStack.cpp
	
	
	
ASMOBJS := $(OBJ)/InnerInterpARM32.o

OBJS := $(patsubst %.cpp,$(OBJ)/%.o,$(SRCS)) $(ASMOBJS)
# XDEFS is used to pass in defines from the make command line
LIBS := -lm -lstdc++ -ldl -lpthread -no-pie

$(OBJ)/InnerInterpARM32.o: $(LIBSRC)/InnerInterpARM32.S
	$(CC) $(CFLAGS) -o $(OBJ)/InnerInterpARM32.o -DATOMIC_REFCOUNTS -DASM_INNER_INTERPRETER -c $(LIBSRC)/InnerInterpARM32.S

$(OBJ)/libforth.a:	$(OBJS)
	mkdir -p $(OBJ)
	ar rcs $(OBJ)/libforth.a $(OBJS)
	
	
$(OBJ)/%.o: $(LIBSRC)/%.cpp
	$(CC) $(LIBSRC)/$*.cpp $(CPPFLAGS) -o $(OBJ)/$*.o
    
$(OBJ)/ForthMain.o:	$(MAINSRC)/ForthMain.cpp
	mkdir -p $(OBJ)
	$(CC) $(MAINSRC)/ForthMain.cpp $(CPPFLAGS) -o $(OBJ)/ForthMain.o

forth:	$(OBJ)/ForthMain.o $(OBJ)/libforth.a
	mkdir -p $(OBJ)
	$(CC) $(OBJ)/ForthMain.o $(DEFINES) -o forth -I $(LIBSRC) $(LD_LIBRARIES) -no-pie
	cp forth ../Sandbox

logger: $(MAINSRC)/ForthLogger.cpp
	$(CC) $(MAINSRC)/ForthLogger.cpp -o logger

clean:
	-rm $(OBJ)/*.o
	-rm $(OBJ)/libforth.a
	-rm ./forth
    
all:	forth

