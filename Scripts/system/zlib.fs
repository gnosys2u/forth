autoforget zlib

DLLVocabulary zlib zlib.dll

also zlib definitions



enum: eZlibFlushValues
  Z_NO_FLUSH
  Z_PARTIAL_FLUSH
  Z_SYNC_FLUSH
  Z_FULL_FLUSH
  Z_FINISH
  Z_BLOCK
  Z_TREES
;enum

enum: eZlibConstants
  // Return codes for the compression/decompression functions. Negative values
  //  are errors, positive values are used for special but normal events.

  0   Z_OK
  1   Z_STREAM_END
  2   Z_NEED_DICT
  -1  Z_ERRNO
  -2  Z_STREAM_ERROR
  -3  Z_DATA_ERROR
  -4  Z_MEM_ERROR
  -5  Z_BUF_ERROR
  -6  Z_VERSION_ERROR

  // compression levels
  0   Z_NO_COMPRESSION
  1   Z_BEST_SPEED
  9   Z_BEST_COMPRESSION
  -1  Z_DEFAULT_COMPRESSION
  
  // compression strategy
  1   Z_FILTERED
  2   Z_HUFFMAN_ONLY
  3   Z_RLE
  4   Z_FIXED
  0   Z_DEFAULT_STRATEGY

  // Possible values of the data_type field (though see inflate())
  0   Z_BINARY
  1   Z_TEXT
  1   Z_ASCII  // for compatibility with 1.2.2 and earlier
  2   Z_UNKNOWN

  // The deflate compression method (the only one supported in this version)
  8   Z_DEFLATED

  0   Z_NULL

;enum

dll_2   gzopen        // gzFile gzopen OF((const char *, const char *));
dll_2   gzdopen       // gzFile gzdopen OF((int fd, const char *mode));
// Set the internal buffer size used by this library's functions.  The default buffer size is 8192 bytes.  64K or more is recommended.
dll_2   gzbuffer      // int gzbuffer OF((gzFile file, unsigned size));
// Dynamically update the compression level or strategy.
dll_3   gzsetparams   // int gzsetparams OF((gzFile file, int level, int strategy));

dll_3   gzread        // int gzread OF((gzFile file, voidp buf, unsigned len));
dll_3   gzwrite       // int gzwrite OF((gzFile file, voidpc buf, unsigned len));
//int gzprintf Z_ARG((gzFile file, const char *format, ...)); - unsupported
dll_2   gzputs        // int gzputs OF((gzFile file, const char *s));
dll_3   gzgets        // char * gzgets OF((gzFile file, char *buf, int len));
dll_2   gzputc        // int gzputc OF((gzFile file, int c));
dll_1   gzgetc        // int gzgetc OF((gzFile file));
dll_2   gzungetc      // int gzungetc OF((int c, gzFile file));
dll_2   gzflush       // int gzflush OF((gzFile file, int flush));
dll_1   gzrewind      // int gzrewind OF((gzFile file));
dll_1   gzeof         // int gzeof OF((gzFile file));
dll_1   gzdirect      // int gzdirect OF((gzFile file));
dll_1   gzclose       // int    gzclose OF((gzFile file));
dll_2   gzerror       // const char * gzerror OF((gzFile file, int *errnum));
dll_1   gzclearerr    // void gzclearerr OF((gzFile file));
dll_3   gzseek        // z_off_t gzseek OF((gzFile, z_off_t, int));
dll_1   gztell        // z_off_t gztell OF((gzFile));
dll_1   gzoffset      // z_off_t gzoffset OF((gzFile));




previous definitions
