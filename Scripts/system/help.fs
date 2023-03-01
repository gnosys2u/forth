\ this file is loaded the first time "help" is executed - if this file hasn't been loaded, help just leaves the address of the
\ desired symbol on TOS and loads this file - the last line in this file should be a $help op which will display the desired
\ help after these definitions have been loaded

"Loading help definitions\n" %s
\ help definitions for builtin ops
addHelp abort			abort		terminate execution with fatal error
addHelp drop			VAL drop	drop top element of param stack
addHelp _doDoes			INTERNAL	compiled at start of "does" section of words created by a builds...does word
addHelp lit							pushes longword which is compiled immediately after it
addHelp flit						pushes float which is compiled immediately after it
addHelp dlit						pushes double which is compiled immediately after it
addHelp _doVariable		INTERNAL	compiled at start of words defined by "variable"
addHelp _doConstant		INTERNAL	compiled at start of words defined by "constant"
addHelp _doDConstant	INTERNAL	compiled at start of words defined by "dconstant"
addHelp _endBuilds		INTERNAL	compiled at end of "builds" section
addHelp done			INTERNAL	makes inner interpreter return - used by outer interpreter
addHelp _doByte			INTERNAL	compiled at start of byte global vars
addHelp _doShort		INTERNAL	compiled at start of short global vars
addHelp _doInt			INTERNAL	compiled at start of int global vars
addHelp _doFloat		INTERNAL	compiled at start of float global vars
addHelp _doDouble		INTERNAL	compiled at start of double global vars
addHelp _doString		INTERNAL	compiled at start of string global vars
addHelp _doOp			INTERNAL	compiled at start of opcode global vars
addHelp _doLong			INTERNAL	compiled at start of long global vars
addHelp _doObject		INTERNAL	compiled at start of object global vars
addHelp _doUByte		INTERNAL	compiled at start of unsigned byte global vars
addHelp _doUShort		INTERNAL	compiled at start of unsigned short global vars
addHelp _exit			INTERNAL	compiled at end of user definitions which have no local vars
addHelp _exitL			INTERNAL	compiled at end of user definitions which have local vars
addHelp _exitM			INTERNAL	compiled at end of method definitions which have no local vars
addHelp _exitML			INTERNAL	compiled at end of method definitions which have local vars
addHelp _doVocab		INTERNAL	compiled at start of vocabularies
addHelp _doByteArray    INTERNAL	compiled at start of byte global arrays
addHelp _doShortArray	INTERNAL	compiled at start of short global arrays
addHelp _doIntArray		INTERNAL	compiled at start of int global arrays
addHelp _doFloatArray	INTERNAL	compiled at start of float global arrays
addHelp _doDoubleArray	INTERNAL	compiled at start of double global arrays
addHelp _doStringArray	INTERNAL	compiled at start of string global arrays
addHelp _doOpArray		INTERNAL	compiled at start of opcode global arrays
addHelp _doLongArray	INTERNAL	compiled at start of 64-bit global arrays
addHelp _doObjectArray	INTERNAL	compiled at start of object global arrays
addHelp _doUByteArray	INTERNAL	compiled at start of opcode global arrays
addHelp _doUShortArray	INTERNAL	compiled at start of opcode global arrays
addHelp initString		INTERNAL	compiled when a local string variable is declared
addHelp initStringArray	INTERNAL	compiled when a local string array is declared
addHelp +				A B ... (A+B)		add top two items
addHelp @				PTR ... A	fetches longword from address PTR
addHelp badOp			INTERNAL	set bad opcode error
addHelp _doStruct		INTERNAL	compiled at start of opcode global arrays
addHelp _doStructArray	INTERNAL	compiled at start of global structure array instances, next word is padded element length
addHelp _doStructType	INTERNAL	compiled at the start of each user-defined structure defining word 
addHelp _doClassType	INTERNAL	compiled at the start of each user-defined class defining word 
addHelp _doEnum			INTERNAL	compiled at start of enum defining word, acts just like 'int'
addHelp _do				INTERNAL	compiled at start of do loop
addHelp _loop			INTERNAL	compiled at end of do loop
addHelp _+loop			INTERNAL	compiled at end of do +loop
addHelp _doNew			INTERNAL	compiled by 'new', immediately following word is class vocab ptr, executes class 'new' operator
addHelp 2@				PTR ... DA	fetch double from address PTR
addHelp _allocObject	INTERNAL	default class 'new' operator, mallocs spaces for a new object instance and pushes new object
addHelp vocabToClass	INTERNAL	varop setter, putting this before a vocab op makes it return vocabs class object
addHelp ref				ref VAR ... PTR	return address of VAR
addHelp ->				-> VAR		set next variable action to "store"
addHelp ->+				N ->+ VAR ...	add N to VAR, append for strings
addHelp ->-				N ->- VAR ...	subtract N from VAR
addHelp this			... OBJECT
addHelp thisData		... OBJECT_DATA
addHelp thisMethods		... OBJECT_METHODS
addHelp execute			OP ...		execute op which is on TOS
addHelp call			IP ...			rpushes current IP and sets IP to that on TOS
addHelp goto			IP ...			sets IP to that on TOS
addHelp i				... LOOPINDEX_I		pushes innermost doloop index
addHelp j				... LOOPINDEX_J		pushes next innermost doloop index
addHelp unloop			remove do-loop info from rstack, use this before an 'exit' inside a do-loop
addHelp leave			exit a do-loop continuing just after the 'loop'
addHelp here	... DP	returns DP

addHelp getNewest	getNewest VOCAB ... PTR_TO_SYMBOL	get a pointer to newest symbol in VOCAB
addHelp findEntry	SYMNAME findEntry VOCAB ...	PTR_TO_SYMBOL	search VOCAB for SYMNAME, return pointer to symbol entry or NULL if not found
addHelp findEntryValue	SYMVALUE findEntryValue VOCAB ... PTR_TO_SYMBOL		search VOCAB for word whose value is SYMVALUE, return pointer to symbol entry or NULL if not found
addHelp addEntry	NAME TYPE VAL addEntry VOCAB ...	add new symbol to VOCAB - VAL holds low 24 bits, TYPE is high 8 bits of opcode
addHelp removeEntry	NAME removeEntry VOCAB ...		remove named symbol from VOCAB
addHelp entryLength	entryLength VOCAB ... NLONGS	length of symbol value field in longwords
addHelp numEntries	numEntries VOCAB ... NSYMBOLS	return number of symbols in VOCAB

addHelp -		A B ... (A-B)		subtract top two items
addHelp *		A B ... (A*B) 	mutliply top two items
addHelp 2*		A  ... (A*2)		multiply top item by 2
addHelp 4*		A  ... (A*4)		multiply top item by 4
addHelp 8*		A  ... (A*8)		multiply top item by 8
addHelp /		A B ... (A/B)		divide top two items
addHelp 2/		A ... (A/2)		divide top item by 2
addHelp 4/		A ... (A/4)		divide top item by 4
addHelp 8/		A ... (A/8)		divide top item by 4
addHelp /mod	A B ... (A/B) (A mod B)	divide top two items, return quotient & remainder
addHelp mod		A B ... (A mod B)			take modulus of top two items
addHelp negate	A ... (-A)	negate top item

addHelp f+	FA FB ... (FA+FB)	add top two floating point items
addHelp f-	FA FB ... (FA-FB)	subtract top two floating point items
addHelp f*	FA FB ... (FA*FB)	multiply top two floating point items
addHelp f/	FA FB ... (FA/FB)	divide top two floating point items

addHelp f=	FA FB ... FA=FB
addHelp f<>	FA FB ... FA<>FB
addHelp f>	FA FB ... FA>FB
addHelp f>=	FA FB ... FA>=FB
addHelp f<	FA FB ... FA<FB
addHelp f<=	FA FB ... FA<=FB
addHelp f0=	FA ... FA=0
addHelp f0<>	FA ... FA<>0
addHelp f0>	FA ... FA>0
addHelp f0>=	FA ... FA>=0
addHelp f0<	FA ... FA<0
addHelp f0<=	FA ... FA<=0
addHelp fwithin	FVAL FLO FHI ... (FLO<=FVAL<FHI)
addHelp fmin	FA FB ... min(FA,FB)
addHelp fmax	FA FB ... max(FA,FB)

addHelp fAddBlock    SRCA SRCB DST NUM ...   add blocks of NUM floats at SRCA and SRCB and store results in DST
addHelp fSubBlock    SRCA SRCB DST NUM ...   subtract blocks of NUM floats at SRCA and SRCB and store results in DST
addHelp fMulBlock    SRCA SRCB DST NUM ...   multiply blocks of NUM floats at SRCA and SRCB and store results in DST
addHelp fDivBlock    SRCA SRCB DST NUM ...   divide blocks of NUM floats at SRCA and SRCB and store results in DST
addHelp fScaleBlock  SRC DST SCALE NUM ...   multiply block of NUM floats at SRC by SCALE and store results in DST
addHelp fOffsetBlock  SRC DST OFFSET NUM ... add OFFSET to block of NUM floats at SRC and store results in DST
addHelp fMixBlock    SRC DST SCALE NUM ...   multiply block of NUM floats at SRC by SCALE and add results into DST

addHelp d+	DA DB ... (DA+DB)	add top two double floating point items
addHelp d-	DA DB ... (DA-DB)	subtract top two double floating point items
addHelp d*	DA DB ... (DA*DB)	multiply top two double floating point items
addHelp d/	DA DB ... (DA/DB)	divide top two double floating point items
addHelp d=	DA DB ... DA=DB
addHelp d<>	DA DB ... DA<>DB
addHelp d>	DA DB ... DA>DB
addHelp d>=	DA DB ... DA>=DB
addHelp d<	DA DB ... DA<DB
addHelp d<=	DA DB ... DA<=DB
addHelp d0=	DA ... DA=0
addHelp d0<>	DA ... DA<>0
addHelp d0>	DA ... DA>0
addHelp d0>=	DA ... DA>=0
addHelp d0<	DA ... DA<0
addHelp d0<=	DA ... DA<=0
addHelp dwithin	DVAL DLO DHI ... (DLO<=DVAL<DHI)
addHelp dmin	DA DB ... min(DA,DB)
addHelp dmax	DA DB ... max(DA,DB)

addHelp dAddBlock    SRCA SRCB DST NUM ...   add blocks of NUM doubles at SRCA and SRCB and store results in DST
addHelp dSubBlock    SRCA SRCB DST NUM ...   subtract blocks of NUM doubles at SRCA and SRCB and store results in DST
addHelp dMulBlock    SRCA SRCB DST NUM ...   multiply blocks of NUM doubles at SRCA and SRCB and store results in DST
addHelp dDivBlock    SRCA SRCB DST NUM ...   divide blocks of NUM doubles at SRCA and SRCB and store results in DST
addHelp dScaleBlock  SRC DST SCALE NUM ...   multiply block of NUM doubles at SRC by SCALE and store results in DST
addHelp dOffsetBlock  SRC DST OFFSET NUM ...  add OFFSET to block of NUM doubles at SRC and store results in DST
addHelp dMixBlock    SRC DST SCALE NUM ...   multiply block of NUM doubles at SRC by SCALE and add results into DST

addHelp dsin		DA    ... sin(DA)
addHelp darcsin		DA    ... arcsin(DA)
addHelp dcos		DA    ... cos(DA)
addHelp darccos		DA    ... arccos(DA)
addHelp dtan		DA    ... tan(DA)
addHelp darctan		DA    ... arctan(DA)
addHelp darctan2	DA DB ... arctan(DA/DB)
addHelp dexp		DA    ... exp(DA)
addHelp dln			DA    ... ln(DA)
addHelp dlog10		DA    ... log10(DA)
addHelp dpow		DA DB ... DA**DB
addHelp dsqrt		DA    ... sqrt(DA)
addHelp dceil		DA    ... ceil(DA)
addHelp dfloor		DA    ... floor(DA)
addHelp dabs		DA    ... abs(DA)
addHelp dldexp		DA B  ... ldexp(DA,B)
addHelp dfrexp		DA    ... frac(DA) exponent(DA)
addHelp dmodf		DA    ... frac(DA) whole(DA)
addHelp dfmod		DA DB ... fmod(DA,DB)

addHelp i2f			A ... float(A)
addHelp i2d			A ... double(A)
addHelp f2i			A ... int(A)
addHelp f2d			A ... double(A)
addHelp d2i			A ... int(A)
addHelp d2f			A ... float(A)

addHelp or			A B ... or(A,B)
addHelp and			A B ... and(A,B)
addHelp xor			A B ... xor(A,B)
addHelp invert		A   ... ~A
addHelp lshift		A B ... A<<B
addHelp rshift		A B ... A>>B
addHelp urshift		A B ... A>>B	unsigned right shift

addHelp not		A   ... not(A)   true iff A is 0, else false
addHelp true	    ... -1
addHelp false	    ... 0
addHelp null	    ... 0

addHelp =		A B ... A=B
addHelp <>		A B ... A<>B
addHelp >		A B ... A>B
addHelp >=		A B ... A>=B
addHelp <		A B ... A<B
addHelp <=		A B ... A<=B
addHelp 0=		A   ... A=0
addHelp 0<>		A   ... A<>0
addHelp 0>		A   ... A>0
addHelp 0>=		A   ... A>=0
addHelp 0<		A   ... A<0
addHelp 0<=		A   ... A<=0
addHelp u>		A B ... A>B		unsigned comparison
addHelp u<		A B ... A<B		unsigned comparison
addHelp within	VAL LO HI ... (LO<=VAL<HI)
addHelp min	A B ... min(A,B)
addHelp max	A B ... max(A,B)

addHelp >r		A ...		pushes top of param stack on top of return stack
addHelp r>		... A		pops top of return stack to top of param stack
addHelp rdrop	...		drops top of return stack
addHelp rp		... RETURN_STACK_PTR
addHelp r0		... EMPTY_RETURN_STACK_PTR

addHelp dup		A ... A A
addHelp ?dup
addHelp swap	A B ... B A
addHelp over	A B ...	A B A
addHelp rot		A B C ... B C A
addHelp -rot	A B C ... C A B
addHelp nip		A B C ... A C
addHelp tuck	A B C ... A C B C
addHelp pick	A ... PS(A)
addHelp roll	N ...		rolls Nth item to top of params stack
addHelp sp		... PARAM_STACK_PTR
addHelp s0		... EMPTY_PARAM_STACK_PTR
addHelp fp		... LOCAL_VAR_FRAME_PTR
addHelp 2dup	DA ...	DA DA
addHelp 2swap	DA DB ... DB DA
addHelp 2drop	DA ...
addHelp 2over	DA DB ... DA DB DA
addHelp 2rot	DA DB DC ... DB DC DA
addHelp r[		push SP on rstack, used with ]r to count a variable number of arguments
addHelp ]r		remove old SP from rstack, push count of elements since r[ on TOS

addHelp !		A PTR ...	stores longword A at address PTR
addHelp c!		A PTR ...	stores byte A at address PTR
addHelp c@		PTR ... A	fetches unsigned byte from address PTR
addHelp sc@		PTR ... A	fetches signed byte from address PTR
addHelp c2i		A ... LA	sign extends byte to long
addHelp w!		A PTR ...	stores word A at address PTR
addHelp w@		PTR ... A	fetches unsigned word from address PTR
addHelp sw@		PTR ... A	fetches signed word from address PTR
addHelp w2i		WA ... LA	sign extends word to long
addHelp 2!		DA PTR ...	store double at address PTR
addHelp move	SRC DST N ...	copy N bytes from SRC to DST
addHelp fill	DST N A ...		fill N bytes at DST with byte value A
addHelp varAction!	A varAction! ...	set varAction to A (use not recommended)
addHelp varAction@	varAction@ ... A	fetch varAction (use not recommended)

addHelp strcpy	STRA STRB ...	copies string from STRB to STRA
addHelp strncpy	STRA STRB N ...	copies up to N chars from STRB to STRA
addHelp strlen	STR ... LEN		returns length of string at STR
addHelp strcat	STRA STRB ...	appends STRB to string at STRA
addHelp strncat	STRA STRB N ...	appends up to N chars from STRB to STRA
addHelp strchr	STR CHAR ... PTR	returns ptr to first occurence of CHAR in string STR
addHelp strrchr	STR CHAR ... PTR	`returns ptr to last occurence of CHAR is string STR
addHelp strcmp	STRA STRB ... V		returns 0 iff STRA = STRB, else result of last char comparison
addHelp stricmp	STRA STRB ... V		returns 0 iff STRA = STRB ignoring case, else result of last char comparison
addHelp strncmp	STRA STRB N ... V		returns 0 iff STRA = STRB ignoring case, else result of last char comparison, for first N chars
addHelp strstr	STRA STRB ... PTR	returns ptr to first occurence of STRB in string STRA
addHelp strtok	STRA STRB ... PTR	returns ptr to next token in STRA, delimited by a char in STRB, modifies STRA, pass 0 for STRA after first call

addHelp fopen				PATH_STR ATTRIB_STR ... FILE		open file
addHelp fclose				FILE ... RESULT						close file
addHelp fseek				FILE OFFSET CTRL ... RESULT			seek in file, CTRL: 0 from start, 1 from current, 2 from end 
addHelp fread				NITEMS ITEMSIZE FILE ... RESULT		read items from file
addHelp fwrite				NITEMS ITEMSIZE FILE ... RESULT		write items to file
addHelp fgetc				FILE ... CHARVAL					read a character from file
addHelp fputc				CHARVAL FILE ... RESULT				write a character to file
addHelp feof				FILE ... RESULT						check if file is at end-of-file
addHelp fexists				PATH_STR ... RESULT					check if file exists
addHelp ftell				FILE ... OFFSET						return current read/write position in file
addHelp flen				FILE ... FILE_LENGTH				return length of file
addHelp fgets				BUFFER MAXCHARS FILE ... NUMCHARS	read a line of up to MAXCHARS from FILE into BUFFER
addHelp fputs				BUFFER FILE ...						write null-terminated string from BUFFER to FILE

addHelp stdin				... FILE							get standard in file
addHelp stdout				... FILE							get standard out file
addHelp stderr				... FILE							get standard error file

addHelp l+		LA LB ... (LA+LB)
addHelp l-		LA LB ... (LA-LB)
addHelp l*		LA LB ... (LA*LB)
addHelp l/		LA LB ... (LA/LB)
addHelp lmod	LA LB ... (LA mod LB
addHelp l/mod	A B ... (LA/LB) (LA mod LB)	divide top two items, return quotient & remainder
addHelp lnegate	A ... (-LA)	negate top item
addHelp i2l		INTA ... LONGA		convert signed 32-bit int to signed 64-bit int
addHelp l2f		LONGA ... FLOATA	convert signed 64-bit int to 32-bit float
addHelp l2d		LONGA ... DOUBLEA	convert signed 64-bit int to 64-bit float
addHelp f2l		FLOATA ... LONGA	convert 32-bit float to signed 64-bit int
addHelp d2l		DOUBLEA ... LONGA	convert 64-bit float to signed 64-bit int

addHelp l=		LA LB ... LA=LB
addHelp l<>		LA LB ... LA<>LB
addHelp l>		LA LB ... LA>LB
addHelp l>=		LA LB ... LA>=LB
addHelp l<		LA LB ... LA<LB
addHelp l<=		LA LB ... LA<=LB
addHelp l0=		LA ... LA=0
addHelp l0<>	LA ... LA<>0
addHelp l0>		LA ... LA>0
addHelp l0>=	LA ... LA>=0
addHelp l0<		LA ... LA<0
addHelp l0<=	LA ... LA<=0
addHelp lwithin	LVAL LLO LHI ... (LLO<=LVAL<LHI)
addHelp lmin	LA LB ... min(LA,LB)
addHelp lmax	LA LB ... max(LA,LB)

addHelp do				ENDVAL STARTVAL ...			start do loop, ends at ENDVAL-1
addHelp loop			...							end do loop
addHelp +loop			INCREMENT ...				end do loop, add INCREMENT to index each time
addHelp if				BOOL ...					start if statement
addHelp else										start else branch of if statement
addHelp endif										end if statement
addHelp begin										begin loop statement
addHelp until			BOOL ...					loop back to "begin" if BOOL is false
addHelp while			BOOL ...					exit loop after "repeat" if BOOL is false
addHelp repeat										end loop statement
addHelp again										loop back to "begin"
addHelp case			TESTVAL ...					begin case statement
addHelp of				CASEVAL ...					begin one case branch
addHelp endof										end case branch
addHelp endcase			TESTVAL ...					end all cases - drops TESTVAL

addHelp try                                         start of code section protected by exception handler
addHelp except                                      start of exception handler code
addHelp finally                                     defines code run after exception handler
addHelp endtry                                      end of exception handler
addHelp raise           EXCEPTION_NUM ...           raise an exception

addHelp builds										starts a builds...does definition
addHelp does										starts the runtime part of a builds...does definition
addHelp exit										exit current hi-level op
addHelp ;											ends a colon definition
addHelp :											starts a colon definition
addHelp f:                                          starts a function definition
addHelp ;f                                          ends a function definition, leaves function opcode on stack
addHelp code										starts an assembler definition
addHelp create										starts a 
addHelp variable	variable NAME		creates a variable op which pushes its address when executed
addHelp constant	A constant NAME		creates a constant op which pushes A when executed
addHelp 2constant	DA 2constant NAME	creates a double constant op which pushes DA when executed
addHelp byte		byte VAR			declare a 8-bit integer variable or field, may be preceeded with initializer "VAL ->"
addHelp ubyte		ubyte VAR			declare a unsigned 8-bit integer variable or field, may be preceeded with initializer "VAL ->"
addHelp short		short VAR			declare a 16-bit integer variable or field, may be preceeded with initializer "VAL ->"
addHelp ushort		ushort VAR			declare a unsigned 16-bit integer variable or field, may be preceeded with initializer "VAL ->"
addHelp int			int VAR				declare a 32-bit integer variable or field, may be preceeded with initializer "VAL ->"
addHelp uint		uint VAR			declare a unsigned 32-bit integer variable or field, may be preceeded with initializer "VAL ->"
addHelp long		long VAR			declare a 64-bit int variable or field, may be preceeded with initializer "VAL ->"
addHelp ulong		ulong VAR			declare a 64-bit unsigned int variable or field, may be preceeded with initializer "VAL ->"
addHelp float		float VAR			declare a 32-bit floating point variable or field, may be preceeded with initializer "VAL ->"
addHelp double		double VAR			declare a 64-bit floating point variable or field, may be preceeded with initializer "VAL ->"
addHelp string		MAXLEN string NAME	declare a string variable or field with a specified maximum length
addHelp op			op VAR				declare a forthop variable or field
addHelp void		returns void		declare that a method returns nothing ?does this work?
addHelp arrayOf		NUM arrayOf TYPE	declare an array variable or field with NUM elements of specified TYPE
addHelp ptrTo		ptrTo TYPE			declare a pointer variable or field of specified TYPE
addHelp struct:		struct: NAME		start a structure type definition
addHelp ;struct							end a structure type definition
addHelp class:		class: NAME			start a class type definition
addHelp ;class							end a class definition
addHelp m:		    m: NAME		        start a class method definition
addHelp ;m   							end a class method definition
addHelp returns		returns TYPE		specify the return type of a method, can only occur in a class method definition
addHelp doMethod	OBJECT METHOD# doMethod		execute speficied method on OBJECT			
addHelp implements:	implements: NAME	start an interface definition within a class definition
addHelp ;implements						ends an interface definition
addHelp union							within a structure definition, resets the field offset to 0
addHelp extends		extends NAME		within a class/struct definition, declares the parent class/struct
addHelp sizeOf				sizeOf STRUCT_NAME	...	STRUCT_SIZE_IN_BYTES
addHelp offsetOf			offsetOf STRUCT_NAME FIELD_NAME ... OFFSET_OF_FIELD_IN_BYTES
addHelp new			new TYPE ... OBJECT		creates an object of specified type
addHelp initMemberString	initMemberString NAME		used inside a method, sets the current and max len fields of named member string
addHelp enum:		enum: NAME			starts an enumerated type definition
addHelp ;enum							ends an enumerated type definition
addHelp recursize						used inside a colon definition, allows it to invoke itself recursively
addHelp precedence						used inside a colon definition, makes it have precedence (execute in compile mode)
addHelp load		load PATH			start loading a forth source file whose name immediately follows "load"
addHelp $load		PATH $load			start loading a forth source file whose name is on TOS
addHelp loaddone						terminate loading a forth source file before the end of file
addHelp requires	requires NAME		if forthop NAME exists do nothing, if not load NAME.txt
addHelp $evaluate	STR $evaluate		interpret string on TOS
addHelp ]					sets state to compile
addHelp [					sets state to interpret
addHelp state				leaves address of "state" var on TOS
addHelp '		' NAME		push opcode of NAME on TOS (does not have precedence)
addHelp postpone		postpone NAME	compile opcode of NAME, including ops which have precedence
addHelp compile			compile			compile immediately following opcode (opcode must not have precedence)
addHelp [']		['] NAME				push opcode of NAME (has precedence)

addHelp forth		...							overwrites the top of the vocabulary stack with forth vocabulary
addHelp definitions	...							makes the top of the vocabulary stack be the destination of newly defined words
addHelp vocabulary	vocabulary VOCAB			create a new vocabulary
addHelp also		...							duplicates top of vocabulary stack, use "also vocab1" to add vocab1 to the stack above current vocab
addHelp previous	...							drops top of vocabulary stack
addHelp only		...							sets the vocabulary stack to just one element, forth, use "only vocab1" to make vocab1 the only vocab in stack
addHelp forget		forget WORDNAME				forget named word and all newer definitions
addHelp autoforget	autoforget WORDNAME			forget named word and all newer definitions, don't report error if WORDNAME is not defined yet
addHelp vlist				display current search vocabulary
addHelp find

addHelp align	...		aligns DP to a lonword boundary
addHelp allot	A ...	adds A to DP, allocating A bytes
addHelp ,		A ...	compiles longword A
addHelp c,		A ...	compiles byte A
addHelp malloc	A ... PTR	allocates a block of memory with A bytes
addHelp realloc	IPTR A ... OPTR		resizes block at IPTR to be A bytes long, leaves new address OPTR
addHelp free	PTR ...		frees a block of memory

addHelp .					NUM ...				prints number in current base followed by space
addHelp .2					LNUM ...			prints 64-bit number in current base followed by space
addHelp %d					NUM ...				prints number in decimal
addHelp %x					NUM ...				prints number in hex
addHelp %2d					LNUM ...			prints 64-bit number in decimal
addHelp %2x					LNUM ...			prints 64-bit number in hex
addHelp %s					STRING ...			prints string
addHelp %c					CHARVAL ...			prints character
addHelp %nc					NUM CHARVAL ...			prints character NUM times
addHelp spaces              NUM ...             prints NUM spaces
addHelp type				STRING NCHARS ...	print a block of NCHARS characters of text
addHelp %4c                 NUM ...             prints 32-bit int as 4 characters
addHelp %8c                 LNUM ...            prints 64-bit int as 8 characters
addHelp %bl										prints a space
addHelp %nl										prints a newline
addHelp %f					FPNUM ...			prints a single-precision floating point number with %f format
addHelp %2f					DOUBLE_FPNUM ...	prints a double-precision floating point number with %f format
addHelp %g					FPNUM ...			prints a single-precision floating point number with %g format
addHelp %2g					DOUBLE_FPNUM ...	prints a double-precision floating point number with %g format
addHelp format				ARG FORMAT_STRING ... STRING_ADDR		format a 32-bit numeric value
addHelp 2format				ARG64 FORMAT_STRING ... STRING_ADDR		format a 64-bit numeric value
addHelp addTempString       STRING NUM ... TSTRING      allocate a temporary string - STRING may be null or NUM may be 0
addHelp atoi                STRING ... NUM      convert string to integer
addHelp atof                STRING ... DOUBLE   convert string to double-precision floating point
addHelp fprintf				FILE FORMAT_STRING (ARGS) NUM_ARGS ... RESULT		print formatted string to file
addHelp snprintf			BUFFER BUFFER_SIZE FORMAT_STRING (ARGS) NUM_ARGS ... RESULT		print formatted string to BUFFER
addHelp fscanf				FILE FORMAT_STRING (ARGS) NUM_ARGS ... RESULT		read formatted string from file
addHelp sscanf				BUFFER FORMAT_STRING (ARGS) NUM_ARGS ... RESULT		read formatted string from BUFFER
addHelp base				leaves address of "base" var on TOS
addHelp octal				sets current base to 8
addHelp decimal				sets current base to 10
addHelp hex					sets current base to 16
addHelp printDecimalSigned	makes all decimal printing signed, printing in any other base is unsigned	
addHelp printAllSigned		makes printing in any base signed
addHelp printAllUnsigned	makes printing in any base unsigned

addHelp getConsoleOut       ... OBJECT          get console output stream
addHelp getDefaultConsoleOut    ... OBJECT      get default console output stream
addHelp setConsoleOut       OBJECT ...          set console output stream
addHelp resetConsoleOut     ...                 set console output stream to default

addHelp toupper             CHAR ... UCHAR      change character to upper case
addHelp isupper             CHAR ... RESULT     test if character is upper case
addHelp isspace             CHAR ... RESULT     test if character is whitespace
addHelp isalpha             CHAR ... RESULT     test if character is alphabetic
addHelp isgraph             CHAR ... RESULT     test if character is graphic
addHelp tolower             CHAR ... LCHAR      change character to lower case
addHelp islower             CHAR ... RESULT     test if character is lower case

addHelp outToFile			FILE ...	redirect output to file
addHelp outToScreen			...			set output to screen (standard out)
addHelp outToString			STRING ...	redirect output to string
addHelp	outToOp				OP ...		redirect output to forth op (op takes a single string argument, returns nothing)
addHelp getConOutFile		... FILE	returns redirected output file

addHelp blword					... STRING_ADDR						fetch next whitespace-delimited token from input stream, return its address
addHelp $word					CHARVAL ... STRING_ADDR				fetch next token delimited by CHARVAL from input stream, return its address
addHelp (						(  COMMENT TEXT )					inline comment, ends at ')' or end of line
addHelp features			                                        variable that allows you to enable and disable language features
addHelp .features                                                   displays what features are currently enabled
addHelp source					... INPUT_BUFFER_ADDR LENGHT		return address of base of input buffer and its length
addHelp >in						... INPUT_OFFSET_ADDR				return pointer to input buffer offset variable
addHelp fillInputBuffer			PROMPT_STRING ... INPUT_BUFFER_ADDR		display prompt, fill input buffer & return input buffer address

addHelp vocNewestEntry	pVocab ... pNewestEntry						get ptr to newest entry in vocabulary
addHelp vocNextEntry	pEntry pVocab ... pNextEntry				get ptr to next entry in vocabulary (can go past end of vocab)
addHelp vocNumEntries	pVocab ... numEntries						get number of entries in vocabulary
addHelp vocName			pVocab ...	pName							get name of vocabulary
addHelp vocChainHead	... pVocab									get head of chain of all vocabularies
addHelp vocChainNext	pVocab ... pVocab							get next vocabulary in chain
addHelp vocFindEntry	pName pVocab ... pEntry						find first entry in vocabulary by name
addHelp vocFindNextEntry	pEntry pName pVocab ... pEntry			find next entry in vocabulary by name
addHelp vocFindEntryByValue		value pVocab ... pEntry				find first entry in vocabulary by value
addHelp vocFindNextEntryByValue		pEntry value pVocab ... pEntry	find next entry in vocabulary by value
addHelp vocAddEntry		pName opType opValue pVocab ...				add an entry to a vocabulary
addHelp vocRemoveEntry		pEntry pVocab ...						remove an entry from a vocabulary
addHelp vocValueLength		pVocab ... valueLengthInLongs			get length of vocabulary value field in longs

addHelp DLLVocabulary			DLLVocabulary VOCAB_NAME DLL_FILEPATH			creates a new DLL vocabulary, also loads the dll
addHelp addDLLEntry				NUM_ARGS "ENTRY_NAME" addDLLEntry	adds a new entry point to current definitions DLL vocabulary
addHelp addDLLEntryEx				NUM_ARGS "OP_NAME" "ENTRY_NAME" addDLLEntryEx	adds a new entry point to current definitions DLL vocabulary
addHelp DLLVoid     when used prior to dll_0...dll_15, newly defined word will return nothing on TOS
addHelp time		... TIME_AS_INT64
addHelp strftime 	BUFFADDR BUFFLEN FORMAT_STRING TIME_AS_INT64 ...	puts formatted string in buffer
addHelp ms@	... TIME_AS_INT32			returns milliseconds since forth started running
addHelp ms	MILLISECONDS ...			sleep for specified number of milliseconds

addHelp rand		... VAL			returns pseudo-random number
addHelp srand		VAL ...			sets pseudo-random number seed
addHelp hash		PTR LEN HASHIN ... HASHOUT		generate a 32-bit hashcode for LEN bytes at PTR starting with initial hashcode HASHIN
addHelp qsort		ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET ...		for strings, set COMPARE_TYPE to kBTString + (256 * maxLen)
addHelp bsearch		KEY_ADDR ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET ... INDEX (-1 if not found)

addHelp dstack				display parameter stack
addHelp drstack				display return stack
addHelp system				STRING ... RESULT				pass DOS command line to system, returns exit code (0 means success)
addHelp chdir				STRING ... RESULT				change current working directory, returns exit code (0 means success)
addHelp remove				STRING ... RESULT				remove named file, returns exit code (0 means success)
addHelp bye					exit forth
addHelp argv				INDEX ... STRING_ADDR			return arguments from command line that started forth
addHelp argc				... NUM_ARGUMENTS				return number of arguments from command line that started forth (not counting "forth" itself)
addHelp turbo			...		switches between slow and fast mode
addHelp stats			...		displays forth engine statistics
addHelp describe		describe OPNAME		displays info on op, disassembles userops
addHelp error			ERRORCODE ...		set the error code
addHelp addErrorText	TEXT ...	add error text
addHelp setTrace		BOOLVAL ...	turn tracing output on/off

addHelp #if				BOOLVAL #if ...			beginning of conditional compilation section
addHelp #ifdef			#ifdef SYMBOL			beginning of conditional compilation section
addHelp #ifndef			#ifndef SYMBOL			beginning of conditional compilation section
addHelp #else			#else					begin else part of conditional compilation section
addHelp #endif			#endif					end of conditional compilation sections

addHelp CreateEvent			MANUALRESET INITIALSTATE NAMESTR ... HANDLE
addHelp CloseHandle			HANDLE ... RESULT
addHelp SetEvent			HANDLE ... RESULT
addHelp ResetEvent			HANDLE ... RESULT
addHelp PulseEvent			HANDLE ... RESULT
addHelp GetLastError		... LASTERROR
addHelp WaitForSingleObject	HANDLE TIMEOUT ... RESULT	
addHelp WaitForMultipleObject	NUMHANDLES HANDLESPTR WAITALL TIMEOUT ... RESULT
addHelp InitializeCriticalSection	CRITSECTIONPTR ...
addHelp DeleteCriticalSection	CRITSECTIONPTR ...
addHelp EnterCriticalSection	CRITSECTIONPTR ...
addHelp LeaveCriticalSection	CRITSECTIONPTR ...
addHelp FindFirstFile	PATHSTR FINDDATA_PTR ... SEARCH_HANDLE
addHelp FindNextFile	SEARCH_HANDLE FINDDATA_PTR ... RESULT
addHelp FindClose		SEARCH_HANDLE ... RESULT
addHelp windowsConstants	... PTR_TO_CONSTANTS
addHelp dumpProfile         ...         dump opcode execution counts.  start profiling with setTrace(1024)
addHelp resetProfile        ...         reset opcode execution counts.


\ THIS NEXT LINE MUST BE THE LAST IN THIS FILE!

$help
