autoforget PHILOSOPHERS
: PHILOSOPHERS ;

25 -> int helpingsToEat
1 -> int eatingTime
1 -> int digestionTime
5 -> int defaultNumPhilosophers
true -> bool useForks

class: Philosopher
  Fiber eatingFiber
  Lock firstFork
  Lock secondFork
  int id
  int timesFed
  int timesHungry
  
  m: delete
    oclear eatingFiber
    oclear firstFork
    oclear secondFork
  ;m

  m: isHungry
    \ timesFed helpingsToEat <
    eatingFiber.getRunState kFTRSExited <>
  ;m
  
  m: run
    \ "philosopher " %s id %d " starting\n" %s
    begin
    while(timesFed helpingsToEat <)
      d[ "philosopher " %s id %d " grabbing first fork(" %s firstFork.id %d ")\n" %s ]d
      firstFork.grab
      d[ "philosopher " %s id %d " grabbing second fork(" %s secondFork.id %d ")\n" %s ]d
      secondFork.grab
      1 ->+ timesFed
      d[ "philosopher " %s id %d " ate " %s timesFed %d " time, going to sleep\n" %s ]d

      thisFiber.sleep(eatingTime)
      
      d[ "philosopher " %s id %d " ungrabbing first fork(" %s firstFork.id %d ")\n" %s ]d
      firstFork.ungrab
      d[ "philosopher " %s id %d " ungrabbing second fork(" %s secondFork.id %d ")\n" %s ]d
      secondFork.ungrab

      thisFiber.sleep(digestionTime)
      
    repeat
    "philosopher " %s id %d " done\n" %s
    thisFiber.exit
  ;m

  m: init
    -> secondFork
    -> firstFork
    -> id
    -> eatingFiber
  ;m
  
;class

: philosopherLoop
  -> Philosopher phil
  phil.run
  oclear phil
;

: diningPhilosophers
  -> int numPhilosophers
  Philosopher phil
  mko Array of Philosopher philosophers
  mko Array of Lock forks

  
  do(numPhilosophers 0)
    new Philosopher -> phil
    i -> phil.id
    philosophers.push(phil)
    oclear phil
    
    new Lock -> Lock fork
    i -> fork.id
    forks.push(fork)
    oclear fork
  loop

  "let the feast begin!\n" %s
  ms@ -> int feastStart

  int firstFork
  int secondFork
  do(numPhilosophers 0)
    i -> int firstFork
    i 1+ -> int secondFork
    if(i numPhilosophers 1- =)
      \ switch grab order for last philosopher to prevent deadlock
      firstFork -> secondFork
      0 -> firstFork
    endif
    
    philosophers.get(i) ->o phil
    phil.init(thisThread.createFiber(['] philosopherLoop 1000 1000) i forks.get(firstFork) forks.get(secondFork))
    \ "philosopher " %s i %d %bl phil.eatingFiber.__fiber %x %nl
    phil.eatingFiber.startWithArgs(r[ phil ]r) drop
    
  loop
  
  do(numPhilosophers 0)
    philosophers.get(i).eatingFiber.join
  loop
  
  "all philosophers stuffed!\n" %s
  ms@ feastStart - %d " milliseconds elapsed\n\n\n" %s

  oclear philosophers
  oclear forks
;

: go
  defaultNumPhilosophers diningPhilosophers
;
  

class: AsyncPhilosopher
  Thread eatingThread
  AsyncLock firstFork
  AsyncLock secondFork
  int id
  int timesFed
  int timesHungry
  
  m: delete
    oclear eatingThread
    oclear firstFork
    oclear secondFork
  ;m

  m: isHungry
    eatingThread.getRunState kFTRSExited <>
  ;m
  
  m: run
    begin
    while(timesFed helpingsToEat <)
      if(useForks)
        d[ p[ "philosopher " %s id %d " grabbing first fork(" %s firstFork.id %d ")\n" %s ]p ]d
        firstFork.grab
        d[ p[ "philosopher " %s id %d " grabbing second fork(" %s secondFork.id %d ")\n" %s ]p ]d
        secondFork.grab
      endif
      1 ->+ timesFed
      d[ p[ "philosopher " %s id %d " ate " %s timesFed %d " time, going to sleep\n" %s ]p ]d

      ms(eatingTime)
      
      if(useForks)
        d[ p[ "philosopher " %s id %d " ungrabbing first fork(" %s firstFork.id %d ")\n" %s ]p ]d
        firstFork.ungrab
        d[ p[ "philosopher " %s id %d " ungrabbing second fork(" %s secondFork.id %d ")\n" %s ]p ]d
        secondFork.ungrab
      endif
      
      ms(digestionTime)
      
    repeat
    p[ "philosopher " %s id %d " done\n" %s ]p
    thisThread.exit
  ;m

  m: init
    -> secondFork
    -> firstFork
    -> id
    -> eatingThread
    id -> eatingThread.id
  ;m
  
;class



: asyncPhilosopherLoop
  -> AsyncPhilosopher phil
  phil.run
  oclear phil
;
  
: asyncDiningPhilosophers
  -> int numPhilosophers
  mko Array of AsyncPhilosopher asyncPhilosophers
  mko Array of AsyncLock asyncForks
  system.createAsyncLock -> printLock
  AsyncPhilosopher phil
  
  do(numPhilosophers 0)
    new AsyncPhilosopher -> phil
    i -> phil.id
    asyncPhilosophers.push(phil)
    oclear phil
    
    system.createAsyncLock -> AsyncLock fork
    i -> fork.id
    asyncForks.push(fork)
    oclear fork
  loop
  
  p[ "let the asynchronous feast begin!\n" %s ]p
  ms@ -> int feastStart
  
  int firstFork
  int secondFork
  do(numPhilosophers 0)
    i -> int firstFork
    i 1+ -> int secondFork
    if(i numPhilosophers 1- =)
      \ switch grab order for last philosopher to prevent deadlock
      firstFork -> secondFork
      0 -> firstFork
    endif
    asyncPhilosophers.get(i) ->o phil
    phil.init(system.createThread(['] asyncPhilosopherLoop 1000 1000) i asyncForks.get(firstFork) asyncForks.get(secondFork))
    \ p[ "philosopher " %s i %d %bl phil.eatingThread.__thread %x %nl ]p
    phil.eatingThread.startWithArgs(r[ phil ]r) drop
  loop
  
  do(numPhilosophers 0)
    asyncPhilosophers.get(i).eatingThread.join
  loop
  p[ "all philosophers stuffed!\n" %s ]p
  p[ ms@ feastStart - %d " milliseconds elapsed\n\n\n" %s ]p
  
  oclear printLock
  oclear asyncPhilosophers
  oclear asyncForks
;

: goAsync
  defaultNumPhilosophers asyncDiningPhilosophers
;


loaddone

