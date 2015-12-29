#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#define htons(s)	(((s) >> 8) | (((s) & 0xff) << 8))
#define htonl(l)	( (((l) & 0xff000000) >> 24) \
                	| (((l) & 0x00ff0000) >> 8) \
                	| (((l) & 0x0000ff00) << 8) \
                	| (((l) & 0x000000ff) << 24))
#define ntohs(s)	htons(s)
#define ntohl(l)	htonl(l)

#endif
