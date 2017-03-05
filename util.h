#ifndef UTIL_H
#define UTIL_H

void perror_exit(const char *msg);

#if __WORDSIZE == 32
#define UINT64_FMT "llu"
#else
#define UINT64_FMT "lu"
#endif

#endif
