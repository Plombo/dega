
#ifndef MD5_H
#define MD5_H 1
typedef enum { bool_false = 0, bool_true = 1 } Bool_t;
//#define FALSE bool_false
//#define TRUE bool_true
#define TorF(x) ((x) ? TRUE : FALSE)
#define MD5_DIGEST_LENGTH 16

#include <string.h>
#include <memory.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t buf[4];
	uint32_t bytes[2];
	uint32_t in[16];
} Md5_t;

void
Md5Initialize(Md5_t *);

void
Md5Invalidate(Md5_t *);

void
Md5Update(Md5_t *, const unsigned char *buf, unsigned int len);

void
Md5Final(Md5_t *, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif /* MD5_H */
