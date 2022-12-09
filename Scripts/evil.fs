autoforget EVIL

: EVIL ;

class: evil
	// implements the 'evil' language - see http://web.archive.org/web/20070103000858/www1.pacific.edu/~twrensch/evil/index.html

	ByteArray wheel
	ByteArray source
	ByteArray pental
	int wheelPos
	int wheelSize
	int sourcePos
	int sourceSize
	int pentalPos
	int pentalSize
	int areg
	int standardMark

	m: reset
		0 -> areg
		0 -> wheelPos
		new ByteArray -> wheel
		wheel.resize(100)
		1 -> wheelSize
		new ByteArray -> pental
		pental.resize(pentalSize)
		0 -> sourcePos
		0 -> wheelPos
		0 -> pentalPos
	;m


	m: setSource
		source.setFromString
		source.count -> sourceSize
	;m
	
	// TOS points to c-style string
	m: init
		new ByteArray -> source
		
		5 -> pentalSize
		reset
	;m
	
	m: jump
		-> int forward

		if( standardMark )
			`m`
		else
			`j`
		endif
		-> byte mark
		
        if( forward )
			begin
            while( sourcePos sourceSize <   source.get(sourcePos) mark <>   and )
                1 ->+ sourcePos
			repeat
        else
			begin
            while( sourcePos  0>=   source.get(sourcePos) mark <>   and )
                1 ->- sourcePos
			repeat
        endif
	;m
    
	m: wopen
        int ix
		if( wheelSize wheel.count >= )
			wheel -> ByteArray temp
			new ByteArray -> wheel
			wheel.resize(temp.count 100+)
			do(wheelSize 0)
				wheel.set(temp.get(i) i)
			loop
		endif
		wheelSize -> ix
		begin
		while(ix wheelPos >)
			wheel.set(wheel.get(i 1-) i)
			1 ->- ix
		repeat
		wheel.set(0 wheelPos)
	;m
    
    
    create weaveMap  4 c, 1 c, 16 c, 2 c, 64 c, 8 c, 128 c, 32 c,

	m: weave
		-> byte x
		
		1 -> int mask
		0 -> int answer
		
		do(8 0)
			if( x mask and )
				or( answer   weaveMap i + c@ ) -> answer
			endif
			mask 2* -> mask
		loop
		
		answer
	;m
	
	m: swap
		wheel -> ByteArray temp
		source -> wheel
		temp -> source
		wheelPos -> int t
		sourcePos -> wheelPos
		t -> sourcePos
		wheelSize -> t
		sourceSize -> wheelSize
		t -> sourceSize
		
		oclear temp
	;m
	
	m: read
		fgetc(stdin)
	;m
	
	m: write
		%c
	;m

	m: dCommand
	;m

	m: doOne
		-> byte cmd
		
		`m` -> byte mark
		`j` -> byte altmark
		byte b

		case( cmd )
			`a` of	1 ->+ areg							endof

			`b` of	jump(false)							endof

			`c` of	wopen								endof

			`d` of
				wheelPos -> int ix
				begin
				while(ix wheelSize 1- <)
					wheel.set(wheel.get(ix 1+) ix)
				repeat
				1 ->- wheelSize
				if( wheelSize 0<= )
					1 -> wheelSize
					0 -> wheelPos
					wheel.set(0 wheelPos)
				endif
			endof
			
			`e` of	weave(areg) -> areg					endof

			`f` of	jump(true)							endof

			`g` of	pental.get(pentalPos) -> areg		endof

			`h` of
				pentalPos 1+ pentalSize mod -> pentalPos
			endof
			
			`i` of
				wheelPos 1+ wheelSize mod -> wheelPos
			endof
			
			`k` of	pental.set(areg pentalPos)			endof

			`l` of
				wheel.get(wheelPos) -> b
				wheel.set(areg wheelPos)
				b -> areg
			endof
			
			`n` of
                1 ->- pentalPos
                if( pentalPos 0< )   pentalSize 1- -> pentalPos   endif
			endof

			`o` of
                1 ->- wheelPos
                if( wheelPos 0< )   wheelSize 1- -> wheelPos   endif
			endof

			`p` of	wheel.get(wheelPos) -> areg			endof

			`q` of	swap								endof

			`r` of	read -> areg						endof

			`s` of	if(areg 0=)  1 ->+ sourcePos endif	endof

			`t` of	if(areg 0<>) 1 ->+ sourcePos endif	endof

			`u` of	1 ->- areg							endof

			`v` of
                pental.get(pentalPos) -> b
                pental.set(areg pentalPos)
                b -> areg
			endof

			`w` of	write(areg)							endof

			`x` of	not(standardMark) -> standardMark	endof

			`y` of	wheel.set(areg wheelPos)			endof

			`z` of	0 -> areg							endof

		endcase
		
		1 ->+ sourcePos
	;m
    
	m: execute
		reset
		begin
		while( sourcePos 0>=  sourcePos sourceSize <  and )
			doOne( source.get(sourcePos) )
		repeat
	;m
	
;class

	
mko String evilProgram

// this is "Hello World!" spelled out character by character
r[
"zaeeeaeeew"
"zaeeaeeaeaw"
"zaeaeeaeeaew"
"zaeaeeaeeaew"
"zuueeueew"
"zaeeeeew"
"zuueueueeeew"
"zuueeueew"
"zaeeaeeaeaeew"
"zaeaeeaeeaew"
"zaeeaeeaew"
"zaeeeeeaw"
"zaeeeeeaeawuuuw"
]r 
evilProgram.load
mko evil vm
vm.init
vm.setSource( evilProgram.get )
vm.execute

loaddone



/* This is a first-pass (call it version 0.2) of an interpreter
 * for the evil programing language. More specifically this is
 * the base language, without the standard library.
 *
 * There should be detailed descriptions of the evil programming
 * language elsewhere. Here, I will just say that it will take
 * any text file as a valid program. All lowercase letters are
 * evil language commands. Uppercase letters are standard library
 * calls (not implemented in this version). Note that there is no
 * such thing as a syntax error in evil.
 *
 * Usage:
 *     java evil mySourceFile.evl
 *
 * ------------------------
 * version: 0.21
 * release date: 10/31/2002
 * Author: Tom Wrensch
 * ------------------------
 *
 * Release History:
 *   0.20 10/25/2002 TeW  First public version released.
 *   0.21 10/31/2002 TeW  bug fix release
 */
import java.io.IOException;
import java.io.FileInputStream;

public class evil implements Runnable {

    byte[] wheel, source, pental;
    int wheelPos, sourcePos, pentalPos;
    int wheelSize, sourceSize, pentalSize;
	byte A;
    boolean standardMark = true;
    
    static public void main(String[] args) {
        StringBuffer buf = new StringBuffer();
        if (args.length < 1) return;
        try {
            FileInputStream fin =
                new FileInputStream(args[0]);
            int b;
            do {
                b = fin.read();
                if (b > 0) 
                    buf.append((char)b);
            } while (b > 0);
        } catch (IOException e) {
        }
        buf.append("");
        evil e = new evil(buf.toString());
        e.execute();
    }
    
    public evil(String pgm) {
        pentalSize = 5;
        reset();
        source = pgm.getBytes();
        sourceSize = source.length;
    }
    
    public void execute() {
        reset();
        while (sourcePos >= 0 && sourcePos < sourceSize)
            doOne(source[sourcePos]);
    }
    
        
    public void run() {
        execute();
    }
    
    public byte[] getWheel() {
        if (wheel.length == wheelSize)
            return wheel;
        byte[] temp = wheel;
        wheel = new byte[wheelSize];
        for (int i=0; i<wheelSize; i++)
            wheel[i] = temp[i];
        return temp;
    }
    
    public byte[] getPental() {
        return pental;
    }
    
    public byte getA() {
        return A;
    }
    
    
    void reset() {
        A = 0;
        wheelPos = 0;
        wheel = new byte[100];
        wheelSize = 1;
        pental = new byte[pentalSize];
        sourcePos = 0;
        wheelPos = 0;
        pentalPos = 0;
    }
    
    
    void doOne(byte cmd) {
        byte mark = (byte) 'm';
        byte altmark = (byte) 'j';
        byte b;
        
        switch ((char) cmd) {
            
            case 'a':
                A++;
                break;
            
            case 'b': 
                jump(false);
                break;
            
            case 'c':
                wopen();
                break;
            
            case 'd':
                for (int i=wheelPos;i<wheelSize-1;i++)
                    wheel[i]=wheel[i+1];
                wheelSize--;
                if (wheelSize <= 0) {
                    wheelSize = 1;
                    wheelPos = 0;
                    wheel[wheelPos] = 0;
                }
                break;
                
            case 'e':
                A = weave(A);
                break;
            
            case 'f':
                jump(true);
                break;
            
            case 'g':
                A = pental[pentalPos];
                break;
            
            case 'h':
                pentalPos = (pentalPos + 1) % pentalSize;
                break;
            
            case 'i':
                wheelPos = (wheelPos + 1) % wheelSize;
                break;
            
            case 'j':
                break;
            
            case 'k':
                pental[pentalPos] = A;
                break;
            
            case 'l':
                b = wheel[wheelPos];
                wheel[wheelPos] = A;
                A = b;
                break;
            
            case 'm':
                break;
            
            case 'n':
                pentalPos = (pentalPos - 1);
                if (pentalPos<0) pentalPos = pentalSize-1;
                break;
            
            case 'o':
                wheelPos = (wheelPos - 1) ;
                if (wheelPos < 0) wheelPos = wheelSize - 1;
                break;
            
            case 'p':
                A = wheel[wheelPos];
                break;
            
            case 'q':
                swap();
                break;
            
            case 'r':
                A = read();
                break;
            
            case 's':
                if (A == 0) sourcePos++;
                break;
            
            case 't':
                if (A != 0) sourcePos++;
                break;
                
            case 'u':
                A--;
                break;
            
            case 'v':
                b=pental[pentalPos];
                pental[pentalPos] = A;
                A = b;
                break;
            
            case 'w':
                write(A);
                break;
            
            case 'x':
                standardMark = !standardMark;
                break;
            
            case 'y':
                wheel[wheelPos] = A;
                break;
            
            case 'z':
                A = 0;
        }
        sourcePos++;
    }
    
    void jump(boolean forward) {
        byte mark = (byte) (standardMark?'m':'j');
        if (forward) {
            while (sourcePos < sourceSize
                    && source[sourcePos] != mark)
                sourcePos++;
        }
        else {
            while (sourcePos >= 0
                    && source[sourcePos] != mark)
                sourcePos--;
        }
    }
    
    void wopen() {
        if (wheelSize >= wheel.length) {
            byte[] temp = wheel;
            wheel = new byte[temp.length + 100];
            for (int i=0; i<wheelSize; i++)
                wheel[i] = temp[i];
        }
        for (int i=wheelSize; i>wheelPos; i--)
            wheel[i] = wheel[i-1];
        wheel[wheelPos] = 0;
    }
    
    
    static int[] weaveMap = {4, 1, 16, 2, 64, 8, 128, 32};

    byte weave(byte x) {
        int mask = 1;
        int answer = 0;
        for (int i=0; i<8; i++) {
            if ((x&mask)!=0)
                answer = answer|(byte)weaveMap[i];
            mask = mask<<1;
        }
        return (byte)answer;
    }
    
    void swap() {
        byte[] temp = wheel;
        wheel = source;
        source = temp;
        int t = wheelPos;
        wheelPos = sourcePos;
        sourcePos = t;
        t = wheelSize;
        wheelSize = sourceSize;
        sourceSize = t;
    }
    
    byte read() {
        byte answer = 0;
        try {
            answer = (byte) System.in.read();
        } 
        catch (IOException e) {
        }
        return answer;
    }
    
    void write(byte b) {
        System.out.print((char)b);
    }
}