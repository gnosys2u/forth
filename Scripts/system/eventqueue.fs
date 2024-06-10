
autoforget eventqueue
: eventqueue ;

class: Event

  Object obj
  int id
  
  m: delete
    obj~
  ;m
  
;class


class: AsyncEventQueue

  List of Event events
  AsyncLock queueLock
  AsyncSemaphore queueSemaphore
  
  m: init
    new List events!
    system.createAsyncLock queueLock!
    system.createAsyncSemaphore queueSemaphore!
    queueSemaphore.init(0)
  ;m
  
  m: delete
    events~
    queueLock~
    queueSemaphore~
  ;m
  
  m: waitForEvent
    queueSemaphore.wait
    queueLock.grab
    events.unrefHead
    queueLock.ungrab
  ;m
  
  m: queueEvent
    queueLock.grab
    events.addTail
    queueLock.ungrab
    queueSemaphore.post
  ;m
  
;class


class: EventQueue

  List of Event events
  Lock queueLock
  Fiber waitingConsumer
  
  m: init
    new List events!
    new Lock queueLock!
  ;m
  
  m: delete
    events~
    queueLock~
    if(objNotNull(waitingConsumer))
      waitingConsumer.wake
      waitingConsumer~
    endif
  ;m
  
  m: waitForEvent
    false bool gotEvent!
    begin
      queueLock.grab
      if(events.isEmpty)
        if(objIsNull(waitingConsumer))
          thisFiber waitingConsumer!
          queueLock.ungrab
          waitingConsumer.sleep(MAXINT)
        else
          queueLock.ungrab
          error("EventQueue:waitForEvent - another thread is already waiting on queue")
        endif
      else
        true gotEvent!
        events.unrefTail
        queueLock.ungrab
      endif
    until(gotEvent)
  ;m
  
  m: queueEvent
    queueLock.grab
    events.addTail
    if(objNotNull(waitingConsumer))
      waitingConsumer.wake
      waitingConsumer~
    endif
    queueLock.ungrab
  ;m
  
;class


0 int __eventId!
system.createAsyncLock AsyncLock __eventCreateLock!

: createEvent
  mko Event event
  __eventCreateLock.grab
  __eventId event.id!
  __eventId++
  __eventCreateLock.ungrab
  event@~
;
  
  

loaddone

