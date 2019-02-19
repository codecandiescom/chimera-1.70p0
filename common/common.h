/*
 * common.h
 */

/*
 * ANSI-C stuff.  Snarfed from xloadimage.
 */
#ifdef __STDC__

#if !defined(_ArgProto)
#define _ArgProto(ARGS) ARGS
#endif

#else /* !__STDC__ */

#if !defined(const) /* "const" is an ANSI thing */
#define const
#endif
#if !defined(_ArgProto)
#define _ArgProto(ARGS) ()
#endif

#endif /* __STDC__ */

void free_mem _ArgProto((char *));
char *alloc_mem _ArgProto((int));
char *calloc_mem _ArgProto((int, int));
char *realloc_mem _ArgProto((char *, int));
char *alloc_string _ArgProto((char *));
