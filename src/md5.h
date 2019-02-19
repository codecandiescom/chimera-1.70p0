#ifndef MD5_H
#define MD5_H

typedef unsigned int uint32;

struct MD5Context
{
  uint32 buf[4];
  uint32 bits[2];
  unsigned char in[64];
};

void MD5Init _ArgProto((struct MD5Context *context));
void MD5Update _ArgProto((struct MD5Context *context, unsigned char *buf, unsigned len));
void MD5Final _ArgProto((unsigned char digest[16], struct MD5Context *context));
void MD5Transform _ArgProto((uint32 buf[4], uint32 in[16]));

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#endif /* !MD5_H */
