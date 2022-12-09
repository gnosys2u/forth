requires asm_pentium
requires sdl

autoforget sdl_audio
: sdl_audio ;

also sdl

subroutine _sdlAudioCallback
	// void (SDLCALL *callback)(void *userdata, Uint8 *stream, int len);
	eax push,
	ebx push,
	ecx push,
	// 0x0c ret 0x10=userdata 0x14=stream 0x18=len
	0x14 esp d] eax mov,	// eax = stream
	0x10 esp d] ebx mov,	// ebx = userdata
	0x18 esp d] ecx mov,	// ecx = len

	here 
	  bl eax ] mov,
	  2 # ebx add,
	  1 # eax add,
	  1 # ecx sub,
	jnz,

	ecx pop,
	ebx pop,
	eax pop,
  ret,
endcode

SDL_AudioSpec desiredSpec
SDL_AudioSpec actualSpec

: atestStart

  44100 -> desiredSpec.freq
  AUDIO_U8 -> desiredSpec.format
  1 -> desiredSpec.channels
  0 -> desiredSpec.silence // not needed?
  _sdlAudioCallback -> desiredSpec.callback
  null -> desiredSpec.userdata
  
  SDL_OpenAudio( desiredSpec null )
  0<> if
    "SDL_OpenAudio failed!\n" %s %nl
    exit
  endif
  SDL_PauseAudio( 0 )
;

: atestEnd
  SDL_PauseAudio( 1 )
  SDL_CloseAudio 
;

loaddone
	0x14 [esp] eax mov,	// eax = stream
	0x10 [esp] ebx mov,	// ebx = userdata
	0x18 [esp] ecx mov,	// ecx = len


	mov	eax, [edx]	; pop file pointer
	push	eax
	mov	eax, [edx+4]	; pop numItems
	push	eax
	mov	eax, [edx+8]	; pop item size
	push	eax
	mov	eax, [edx+12]	; pop dest pointer
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileWrite
	call	eax
	add		sp, 16

	int			freq				// DSP frequency -- samples per second
	short			format			// Audio data format
	byte			channels 		// Number of channels: 1 mono, 2 stereo
	byte			silence			// Audio buffer silence value (calculated)
	short			samples			// Audio buffer size in samples (power of 2)
	short			padding			// Necessary for some compile environments
	int			size				// Audio buffer size in bytes (calculated)
	// This function is called when the audio device needs more data.
	//   'stream' is a pointer to the audio data buffer
	//   'len' is the length of that buffer in bytes.
	//   Once the callback returns, the buffer will no longer be valid.
	//   Stereo samples are stored in a LRLRLR ordering.
	// void (SDLCALL *callback)(void *userdata, Uint8 *stream, int len);
	ptrTo int	callback
	ptrTo int	userdata
