autoforget PIZZASHOP
: PIZZASHOP ;

10 -> int pizzasToDeliverEach
8 -> int pizzasAlreadyMade
10 -> int makingTime
20 -> int deliveryTime
5 -> int defaultNumDrivers
AsyncLock asyncDeliveredCountLock
int pizzasDelivered

class: AsyncPizzaDriver
  Thread deliverThread
  AsyncSemaphore pizzaReadyToDeliver
  int id
  int pizzasLeftToDeliver
  
  m: delete
    oclear deliverThread
    oclear pizzaReadyToDeliver
  ;m

  m: run
    begin
    while(pizzasLeftToDeliver 0>)
      d[ p[ "driver " %s id %d " waiting for a pizza to deliver\n" %s ]p ]d
      pizzaReadyToDeliver.wait
      d[ p[ "driver " %s id %d " got pizza, delivering to customer\n" %s ]p ]d
      ms(deliveryTime)
      d[ p[ "driver " %s id %d " returning to pizza shop\n" %s ]p ]d
      asyncDeliveredCountLock.grab
      pizzasDelivered   1 ->+ pizzasDelivered
      asyncDeliveredCountLock.ungrab
      1 ->- pizzasLeftToDeliver
      p[
        "driver " %s id %d " delivered pizza #" %s pizzasDelivered %d
        if(pizzasLeftToDeliver)
          ", he has " %s pizzasLeftToDeliver %d " to go\n" %s
        else
          ", he is done for this shift.\n" %s
        endif
      ]p
    repeat
    p[ "driver " %s id %d " has gone home.\n" %s ]p
    thisThread.exit
  ;m

  m: init
    -> pizzasLeftToDeliver
    -> id
    -> pizzaReadyToDeliver
    -> deliverThread
    id -> deliverThread.id
  ;m
;class

: asyncDeliveryLoop
  -> AsyncPizzaDriver del
  del.run
  oclear del
;
  

: asyncDeliverPizzas
  -> int pizzasMade
  -> int pizzasPerDriver
  -> int numDrivers
  AsyncPizzaDriver izzy  \ call me Ishmael, izzy for short.
  numDrivers pizzasPerDriver * pizzasMade - -> int pizzasToMake
  system.createAsyncLock -> printLock
  system.createAsyncLock -> asyncDeliveredCountLock
  0 -> pizzasDelivered
  ms@ -> int workStart
  
  p[ "open for asynchronous business!\n" %s ]p
  
  mko Array of AsyncPizzaDriver drivers
  
  AsyncSemaphore pizzaReadyToDeliver
  system.createAsyncSemaphore -> pizzaReadyToDeliver
  pizzaReadyToDeliver.init(pizzasMade)

  do(numDrivers 0)
    new AsyncPizzaDriver -> izzy
    i -> izzy.id
    drivers.push(izzy)
    izzy.init(system.createThread(['] asyncDeliveryLoop 1000 1000) pizzaReadyToDeliver i pizzasToDeliverEach)
    izzy.deliverThread.startWithArgs(r[ izzy ]r) drop
    oclear izzy
  loop

  begin
  while(pizzasToMake 0>)
    d[ p[ "pizza in the oven!\n" %s ]p ]d
    ms(makingTime)
    d[ p[ "made another pizza!\n" %s ]p ]d
    pizzaReadyToDeliver.post
    1 ->- pizzasToMake

    case(pizzasToMake)
      of(1)
        d[ p[ "I need to make one more pizza!\n" %s ]p ]d
      endof
      
      of(0)
        d[ p[ "I am done making pizzas!\n" %s ]p ]d
      endof
      
      d[ p[ "I need to make " %s pizzasToMake %d " more pizzas.\n" %s ]p ]d
    endcase
  repeat
  
  do(numDrivers 0)
    drivers.get(i) -> izzy
    izzy.deliverThread.join
    oclear izzy
  loop
  
  p[ ms@ workStart - %d " milliseconds elapsed\n\n\n" %s ]p
  oclear drivers
  oclear pizzaReadyToDeliver
;

: goAsync
  asyncDeliverPizzas(defaultNumDrivers pizzasToDeliverEach pizzasAlreadyMade)
;

Lock deliveredCountLock

class: PizzaDriver
  Fiber deliverFiber
  Semaphore pizzaReadyToDeliver
  int id
  int pizzasLeftToDeliver
  
  m: delete
    oclear deliverFiber
    oclear pizzaReadyToDeliver
  ;m

  m: run
    begin
    while(pizzasLeftToDeliver 0>)
      d[ p[ "driver " %s id %d " waiting for a pizza to deliver\n" %s ]p ]d
      pizzaReadyToDeliver.wait
      d[ p[ "driver " %s id %d " got pizza, delivering to customer\n" %s ]p ]d
      thisFiber.sleep(deliveryTime)
      d[ p[ "driver " %s id %d " returning to pizza shop\n" %s ]p ]d
      deliveredCountLock.grab
      pizzasDelivered   1 ->+ pizzasDelivered
      deliveredCountLock.ungrab
      1 ->- pizzasLeftToDeliver
      p[
        "driver " %s id %d " delivered pizza #" %s pizzasDelivered %d
        if(pizzasLeftToDeliver)
          ", he has " %s pizzasLeftToDeliver %d " to go\n" %s
        else
          ", he is done for this shift.\n" %s
        endif
      ]p
    repeat
    p[ "driver " %s id %d " has gone home.\n" %s ]p
    thisFiber.exit
  ;m

  m: init
    -> pizzasLeftToDeliver
    -> id
    -> pizzaReadyToDeliver
    -> deliverFiber
    id -> deliverFiber.id
  ;m
;class

: deliveryLoop
  -> PizzaDriver del
  del.run
  oclear del
;
  

: deliverPizzas
  -> int pizzasMade
  -> int pizzasPerDriver
  -> int numDrivers
  PizzaDriver izzy  \ call me Ishmael, izzy for short.
  numDrivers pizzasPerDriver * pizzasMade - -> int pizzasToMake
  system.createAsyncLock -> printLock
  system.createAsyncLock -> deliveredCountLock
  0 -> pizzasDelivered
  ms@ -> int workStart
  
  p[ "open for business!\n" %s ]p
  
  mko Array of PizzaDriver drivers
  
  Semaphore pizzaReadyToDeliver
  new Semaphore -> pizzaReadyToDeliver
  pizzaReadyToDeliver.init(pizzasMade)

  do(numDrivers 0)
    new PizzaDriver -> izzy
    i -> izzy.id
    drivers.push(izzy)
    izzy.init(thisThread.createFiber(['] deliveryLoop 1000 1000) pizzaReadyToDeliver i pizzasToDeliverEach)
    izzy.deliverFiber.startWithArgs(r[ izzy ]r) drop
    oclear izzy
  loop

  begin
  while(pizzasToMake 0>)
    d[ p[ "pizza in the oven!\n" %s ]p ]d
    thisFiber.sleep(makingTime)
    d[ p[ "made another pizza!\n" %s ]p ]d
    pizzaReadyToDeliver.post
    1 ->- pizzasToMake

    if(pizzasToMake)
    endif
    
    case(pizzasToMake)
      of(1)
        d[ p[ "I need to make one more pizza!\n" %s ]p ]d
      endof
      
      of(0)
        d[ p[ "I am done making pizzas!\n" %s ]p ]d
      endof
      
      d[ p[ "I need to make " %s pizzasToMake %d " more pizzas.\n" %s ]p ]d
    endcase
  repeat
  ds
  do(numDrivers 0)
    drivers.get(i) -> izzy
    izzy.deliverFiber.join
    oclear izzy
  loop
  ds
  p[ ms@ workStart - %d " milliseconds elapsed\n\n\n" %s ]p
  oclear drivers
  oclear pizzaReadyToDeliver
;

: go
  deliverPizzas(defaultNumDrivers pizzasToDeliverEach pizzasAlreadyMade)
;

loaddone

