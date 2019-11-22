/*
 * util/base64.h
 */
#ifndef BASE64_H_
#define BASE64_H_

char * base64_decode(const void * data, const size_t len);
char * base64_encode(const unsigned char * src, const size_t len, size_t * n);

#endif  /* BASE64_H_ */
