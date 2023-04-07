
class: BrainfuckInterpreter
  ptrTo byte mpInstruction
  ptrTo byte mpData
  ByteArray mData
  InStream mInStream
  OutStream mOutStream
  CellArray mLoopStack

  m: init
    new ByteArray mData!
    mData.resize(60000)
    mData.base mpData!
    new ConsoleInStream mInStream!
    new ConsoleOutStream mOutStream!
    new CellArray mLoopStack!
  ;m
  
  m: delete
    mData~
    mLoopStack~
  ;m
  
  m: interpret
    byte instruction
    false int done!
    mpInstruction!
    begin
    while(mpInstruction b@ dup -> instruction)
      mpInstruction++
      case(instruction)
        '>' of mpData++ endof
        '<' of mpData-- endof
        '+' of mpData b@ 1+ mpData b! endof
        '-' of mpData b@ 1- mpData b! endof
        '.' of mpData b@ %c endof
        ',' of mInStream.getChar mpData b! endof

        '[' of
          if(mpData b@)
            mLoopStack.push(mpInstruction)
          else
            \  skip to next ]
            done~
            begin
              mpInstruction b@ dup
              if
                mpInstruction++
                if(']' =)
                  true done!
                endif
              else
                \  ran off end of program without finding ']'
                drop
                true done!
              endif
            until(done)
          endif
        endof

        ']' of
          if(mpData b@)
            mLoopStack.pop dup mpInstruction!
            mLoopStack.push
          else
            mLoopStack.pop drop
          endif
        endof

      endcase
    repeat
  ;m
  
;class

." Type 'hello' to run a HelloWorld program in brainfuck"
: hello
  mko BrainfuckInterpreter bfi
  bfi.init
  bfi.interpret("++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.")
  bfi~
;

loaddone

>   next            dp++
<   prev            dp--
+   incVal          *dp += 1
-   decVal          *dp -= 1
.   outVal          output *dp as byte
,   accept one byte of input, store it at dp
[
        if *dp==0
            set ip to command after next ] command
]
        if *dp != 0
            set ip to command after previous [ command

