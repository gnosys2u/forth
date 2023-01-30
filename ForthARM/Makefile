#####################################################################################

# you may need to install readline with this command:
# sudo apt-get install libreadline6-dev libreadline6

#.PHONY:	all clean depends

MAINSRC = .
MAINOBJ = $(MAINSRC)/obj
LIBSRC = ../ForthLib
LIBOBJ = $(LIBSRC)/obj

CC = g++
ASM = as
CDEBUG = -g


DEFINES = -DRASPI -DLINUX -DASM_INNER_INTERPRETER -DINCLUDE_TRACE -DATOMIC_REFCOUNTS -DSTORAGE_LONGS=4000000
CFLAGS = -c $(CDEBUG) $(DEFINES) $(WARNING_FLAGS) -save-temps=obj -std=c++17 -marm

#WARNING_FLAGS = -Wall -Wno-format -Wno-reorder -Wno-unused-variable -Wsign-compare -Wunused-but-set-variable
#WARNING_FLAGS = -Wno-format -Wno-reorder -Wno-unused-variable
WARNING_FLAGS = -Wall -Wno-format -Wno-reorder -Wno-unused-variable -Wno-sign-compare -Wno-conversion-null -Wno-pointer-arith

CPPFLAGS = $(CFLAGS) -std=c++11 -I ../ForthLib
#CPPFLAGS = $(CFLAGS) -I ForthLib



#DEFINES = -DLINUX -DASM_INNER_INTERPRETER -DASM_UNDERSCORE_PREFIX -DFORTH64 -DINCLUDE_TRACE
#DEFINES = -DLINUX -DNOASM_INNER_INTERPRETER -DASM_UNDERSCORE_PREFIX -DFORTH64 -DINCLUDE_TRACE

ASMFLAGS = --defsym ASM_INNER_INTERPRETER=1

LDFLAGS = $(CDEBUG)
LD_LIBRARIES = -L ../ForthLib -lforth -lm -lc -lstdc++ -ldl -lpthread -lreadline

SRCS = \
	ForthShell.cpp \
	ForthEngine.cpp \
	OuterInterpreter.cpp \
	ForthInner.cpp \
	ForthInput.cpp \
	ForthVocabulary.cpp \
	ForthBuiltinClasses.cpp \
	ForthExtension.cpp \
	ForthShowContext.cpp \
	ForthStructs.cpp \
	ForthStructCodeGenerator.cpp \
	ForthOps.cpp \
	ForthServer.cpp \
	ForthClient.cpp \
	ForthOpcodeCompiler.cpp \
	ForthBlockFileManager.cpp \
	ForthParseInfo.cpp \
	ForthPipe.cpp \
	ForthForgettable.cpp \
	ForthThread.cpp \
	ForthObjectReader.cpp \
	ForthMemoryManager.cpp \
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
	DLLVocabulary.cpp \
	LocalVocabulary.cpp \
	NativeType.cpp \
	TypesManager.cpp \
	VocabularyStack.cpp
	
	
	
#ASMSRCS = InnerInterpAmd64.asm \
#	AsmCore64.asm
#ASMSRCS = InnerInterpARM.S
#ASMOBJS := $(patsubst %.S,$(LIBOBJ)/%.o,$(ASMSRCS))
ASMOBJS := $(LIBOBJ)/InnerInterpARM.o

OBJS := $(patsubst %.cpp,$(LIBOBJ)/%.o,$(SRCS)) $(ASMOBJS)
DEPDIR = dep
DEPS := $(patsubst %.cpp,$(DEPDIR)/%.d,$(SRCS))
# XDEFS is used to pass in defines from the make command line
LIBS := -lm -lstdc++ -ldl -lpthread -no-pie

$(LIBOBJ)/InnerInterpARM.o: $(MAINSRC)/InnerInterpARM.S
	$(CC) $(CFLAGS) -o $(LIBOBJ)/InnerInterpARM.o -DATOMIC_REFCOUNTS -DASM_INNER_INTERPRETER -c $(MAINSRC)/InnerInterpARM.S

$(LIBSRC)/libforth.a:	$(OBJS)
	ar rcs $(LIBSRC)/libforth.a $(OBJS)
	
	
-include $(DEPS)

stuff:
	echo "DEPS=" $(DEPS)
	echo "OBJS=" $(OBJS)
	echo "SRCS=" $(SRCS)

#spoo: ForthOps.cpp
#	echo "at " $@ "   less" $< "   star" $* "   pat" $(patsubst %.cpp,%.d,$@)
	
$(DEPDIR)/%.d: %.cpp
	$(CC) -M $(CPPFLAGS) -o $(DEPDIR)/$*.d $(LIBSRC)/$*.cpp
	
#%.d: ../%.cpp
#	$(CC) -M $(CPPFLAGS) -o $*.d $<
	
#	echo "DEP at=" $@ "   less=" $< "   star=" $*

$(LIBOBJ)/%.o: $(LIBSRC)/%.cpp
	$(CC) $(CFLAGS) -o $(LIBOBJ)/$*.o $(LIBSRC)/$*.cpp
    
#$(LIBOBJ)/%.o: $(LIBSRC)/%.asm
#	$(ASM) $(ASMFLAGS) -o $(LIBOBJ)/$*.o $(LIBSRC)/$*.asm

$(MAINOBJ)/ForthMainARM.o:	$(MAINSRC)/ForthMainARM.cpp
	echo "* " $*
	echo "< " $<
	echo "@ " $@
	$(CC) $(CPPFLAGS) -o $(MAINOBJ)/ForthMainARM.o $(MAINSRC)/ForthMainARM.cpp

forth:	$(MAINOBJ)/ForthMainARM.o $(LIBSRC)/libforth.a
	$(CC) $(MAINOBJ)/ForthMainARM.o $(DEFINES) -o forth -I ../ForthLib $(LD_LIBRARIES) -no-pie
	cp forth ../Sandbox

logger: $(MAINSRC)/ForthLoggerARM.cpp
	$(CC) $(MAINSRC)/ForthLoggerARM.cpp -o logger

clean:
	-rm $(MAINOBJ)/*.o
	-rm $(DEPDIR)/*.d
	-rm $(LIBOBJ)/*.o
	-rm $(LIBSRC)/libforth.a
	-rm ./forth
    
#all:	forth $(LIBSRC)/libforth.a
all:	forth

