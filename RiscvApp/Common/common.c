#include "firmware.h"

char  *skipchar (char *str, char ch)
{
  while (*str && (*str == ch))  str++;
  return str;
}

char  *findchar (char *str, char ch)
{
  while (*str && (*str != ch))  str++;
  return str;
}

char  *findbyte (char *str, char ch)
{
  while (*str != ch)  str++;
  return str;
}

void *memset(void *s, int c, size_t n)
{
  register unsigned char *dst = (unsigned char *)s;
  while (n--)  *dst++ = (unsigned char)c;
  return s;
}

void *memcpy(void *d, void *s, size_t n)
{
  register unsigned char *dst = (unsigned char *)d;
  register unsigned char *src = (unsigned char *)s;
  while (n--)  *dst++ = *src++;
  return d;
}


char * strcpy(char * dest,const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}

char * strcat(char * dest, const char * src)
{
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;

	return tmp;
}

int strcmp(const char * cs,const char * ct)
{
	register signed char __res;

	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
	}

	return __res;
}

int strncmp(const char * cs,const char * ct, int count)
{
	register signed char __res = 0;

	while (count) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
		count--;
	}

	return __res;
}

int strlen(const char * s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}


char tolower(char ch)
{
  if (ch >= 'A' && ch <= 'Z')
    ch = ch - 'A' + 'a';
  return ch;
}

char toupper(char ch)
{
  if (ch >= 'a' && ch <= 'z')
    ch = ch - 'a' + 'A';
  return ch;
}

int strncmpi(void *s1, void *s2, size_t n)
{
  register unsigned char *str1 = (unsigned char *)s1;
  register unsigned char *str2 = (unsigned char *)s2;
  char  i, j;
  while (n--)  {
    i = *str1++;
    j = *str2++;
    if ( ( i >= 'a' ) && ( i <= 'z' ) ) {
      i = i - 'a' + 'A';
    }
    if ( ( j >= 'a' ) && ( j <= 'z' ) ) {
      j = j - 'a' + 'A';
    }
    if (i != j)
      return 1;
  }
  return 0;
}


int memcmp(void *s1, void *s2, size_t n)
{
  register unsigned char *str1 = (unsigned char *)s1;
  register unsigned char *str2 = (unsigned char *)s2;
  while (n--)  {
    if (*str1++ != *str2++)
      return 1;
  }
  return 0;
}

unsigned long xtoi (char *str)
{
  unsigned long   u;
  char            c;
  char            v;

  // skip white space
  str = skipchar (str, ' ');

  // skip preceeding zeros
  str = skipchar (str, '0');

  // skip preceeding "0x"
  if (*str && (*str == 'x' || *str == 'X')) {
    str ++;
  }

  // convert hex digits
  u = 0;
  do {
    c = *(str++);
    if (c >= 'a'  &&  c <= 'f') {
      c -= 'a' - 'A';
    }
    if (c >= '0'  &&  c <= '9') {
      v = c - '0';
    } else if (c >= 'A'  &&  c <= 'F') {
      v = c - 'A' + 10;
    } else {
      break;
    }
    u = (u << 4)  |  v;
  } while (c);

  return u;
}

void delay_ms (unsigned int ms)
{
	volatile uint32_t i;
	volatile uint32_t j;

	for (i = 0; i < ms; i++) {
		for (j = 0; j < 2000; j++);
	}
}

