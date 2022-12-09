
autoforget randoms

// park-miller random number generator

: randoms ;

// some other linear congruential random generators
//   which use form randomSeed = ((a * randomSeed) + c) % m
//Borosh-Niederreiter: a = 1812433253, m = 2^32
//Fishman18: a = 62089911, m = 2^31 - 1
//Fishman20: a = 48271, m = 2^31 - 1
//L'Ecuyer: a = 40692, m = 2^31 - 249
//Waterman: a = 1566083941, m = 2^32. 

// these generator ops take a seed on TOS, leave TOS: newSeed generatedRandom

// randu sucks, but is famous
: _randu 65539 * dup ;

// randVax is okay
: _randVax 69069 * 1+ dup ;

// randInmos is probably nearly as bad as randu
: _randInmos 1664525 * dup ;

// this was in a description of how rand in C runtime lib works
: _randCLib  1103515245 * 12345 + dup 16 rshift 32767 and swap ;

: _randParkMiller
  // seed 16807 * MAXINT mod     (doesn't work because of integer overflows turning negative)
  127773 /mod
  swap 16807 * swap 2836 * -
  dup 0<= if
    MAXINT +
  endif
  dup
;

// park-miller is default generator
' _randParkMiller -> op _randomGeneratorOp

#if FORTH64
time ms@ xor -> int _randomSeed
#else
time or ms@ xor -> int _randomSeed
#endif


// _randomSeed 16807 * MAXINT mod
// doesn't work because of integer overflows turning negative

: random
  _randomSeed _randomGeneratorOp -> _randomSeed
;
  
: setRandomSeed
  // randomSeed must be between 1 and 2147483646 (MAXINT - 1)
  // this is tailored for park-miller, should be okay for other linear-congruential gens
  MAXINT and
  // seed is now between 0 and 2147483647
  dup 0= if
    drop 3802896
  else
    dup MAXINT = if
      3802896 -
    endif
  endif
  -> _randomSeed
;

: getRandomSeed
  _randomSeed
;
  
class: RandomIntGenerator
  int seed
  op generatorOp
  
  m: setSeed
    -> seed
  ;m
  
  m: randomize
#if FORTH64
    time
#else
    time or
#endif
    setSeed(ms@ xor)
  ;m
  
  m: init
    randomize
    lit _randParkMiller -> generatorOp
  ;m
  
  m: getSeed
    seed
  ;m
  
  m: setGeneratorOp
    -> generatorOp
  ;m
  
  m: generate
    if(fetch generatorOp 0=)
      // first time only
      init
    endif
    
    generatorOp(seed) -> seed
  ;m
  
  m: shuffle
    -> Array a
    a.count -> cell ii
  
    begin
      mod(generate ii) -> cell jj
      1 ->- ii
      //"swap " %s ii %d " with " %s jj %d %nl
      a.swap(ii jj)
    until(ii 1 =)
    oclear a
  ;m
  
;class


class: XorWowIntRandom extends RandomIntGenerator
  // by Marsaglia
  uint seedB  uint seedC  uint seedD
  uint counter
  
  m: setSeed
    dup -> seed
    dup 0x12345678 xor -> seedB
    dup dup * -> seedC
    42 * 17 / -> seedD
    0 -> counter
  ;m
  
  m: init
    randomize
  ;m
  
  m: generate
    if(fetch generatorOp 0=)
      // first time only
      init
    endif
    
    generatorOp(seed) -> seed
  ;m
  : _randXorWow
    seedD -> uint temp
    
  // seed 16807 * MAXINT mod     (doesn't work because of integer overflows turning negative)
    127773 /mod
    swap 16807 * swap 2836 * -
  dup 0<= if
    MAXINT +
  endif
  dup
;

  
;class

loaddone


struct xorwow_state {
  uint32_t a, b, c, d;
  uint32_t counter;
};

/* The state array must be initialized to not be all zero in the first four words */
uint32_t xorwow(struct xorwow_state *state)
{
	/* Algorithm "xorwow" from p. 5 of Marsaglia, "Xorshift RNGs" */
	uint32_t t = state->d;

	uint32_t const s = state->a;
	state->d = state->c;
	state->c = state->b;
	state->b = s;

	t ^= t >> 2;
	t ^= t << 1;
	t ^= s ^ (s << 4);
	state->a = t;

	state->counter += 362437;
	return t + state->counter;
}

class: MarsagliaRandomIntGenerator extends RandomIntGenerator

  m: init
    0x7A3B5CFE -> seed
  ;m
;class

long
MarsagliaRandomGenerator::NextLong(void)
{
    mZ = 36969 * (mZ & 65535) + (mZ >> 16);
    mW = 18000 * (mW & 65535) + (mW >> 16);
    return (mZ << 16) + mW;
}


void
MarsagliaRandomGenerator::SetSeed(long seedVal)
{
    mW = seedVal;
    mZ = mW * 2654435769;
}



https://en.wikipedia.org/wiki/Xorshift

/* The state word must be initialized to non-zero */
uint32_t xorshift32(struct xorshift32_state *state)
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return state->a = x;
}

struct xorshift64_state {
  uint64_t a;
};

uint64_t xorshift64(struct xorshift64_state *state)
{
	uint64_t x = state->a;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return state->a = x;
}

// https://en.wikipedia.org/wiki/Xorshift#xoshiro_and_xoroshiro

struct xorshift128p_state {
  uint64_t a, b;
};

/* The state must be seeded so that it is not all zero */
uint64_t xorshift128p(struct xorshift128p_state *state)
{
	uint64_t t = state->a;
	uint64_t const s = state->b;
	state->a = s;
	t ^= t << 23;		// a
	t ^= t >> 17;		// b
	t ^= s ^ (s >> 26);	// c
	state->b = t;
	return t + s;
}

loaddone

// the park & miller article specifies the 10000th value must be 1043618065
: test
  1 setRandomSeed
  9999 0 do
    random drop
  loop
  "Next value should be 1043618065: " %s
  random %d %nl
;

loaddone

: randomRaw
  seed 1103515245 *
  12345 +
  0x7fffffff and
  dup -> seed
;

: random
  randomRaw 8 rshift
  swap mod
;

loaddone

// http://c-faq.com/lib/rand.html portable parks-miller
#define a 16807
#define m 2147483647
#define q (m / a)
#define r (m % a)
// q is 127773
// r is 2836
static long int seed = 1;

long int PMrand()
{
    // this hoohah does ((a*seed) mod m) without overflowing in the multiply
	long int hi = seed / q;
	long int lo = seed % q;
	long int test = a * lo - r * hi;
	if(test > 0)
		seed = test;
	else	seed = test + m;
	return seed;
}

To alter it to return floating-point numbers in the range (0, 1) (as in the Park and Miller paper), change the declaration to
	double PMrand()
and the last line to
	return (double)seed / m;
For slightly better statistical properties, Park and Miller now recommend using a = 48271.
