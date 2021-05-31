#ifndef _COMMON_H_
#define _COMMON_H_

#define  abs(N)   ((N<0)?(-N):(N))

#define  BIT0     0x00000001
#define  BIT1     0x00000002
#define  BIT2     0x00000004
#define  BIT3     0x00000008
#define  BIT4     0x00000010
#define  BIT5     0x00000020
#define  BIT6     0x00000040
#define  BIT7     0x00000080
#define  BIT8     0x00000100
#define  BIT9     0x00000200
#define  BIT10    0x00000400
#define  BIT11    0x00000800
#define  BIT12    0x00001000
#define  BIT13    0x00002000
#define  BIT14    0x00004000
#define  BIT15    0x00008000
#define  BIT16    0x00010000
#define  BIT17    0x00020000
#define  BIT18    0x00040000
#define  BIT19    0x00080000
#define  BIT20    0x00100000
#define  BIT21    0x00200000
#define  BIT22    0x00400000
#define  BIT23    0x00800000
#define  BIT24    0x01000000
#define  BIT25    0x02000000
#define  BIT26    0x04000000
#define  BIT27    0x08000000
#define  BIT28    0x10000000
#define  BIT29    0x20000000
#define  BIT30    0x40000000
#define  BIT31    0x80000000

#define  ALIGN_UP(address, align)     (((address) + ((align) - 1)) & ~((align)-1))
#define  ALIGN_DOWN(address, align)   ((address) & ~((align)-1))

#define  SIZE_1KB    0x00000400
#define  SIZE_2KB    0x00000800
#define  SIZE_4KB    0x00001000
#define  SIZE_8KB    0x00002000
#define  SIZE_16KB   0x00004000
#define  SIZE_32KB   0x00008000
#define  SIZE_64KB   0x00010000
#define  SIZE_128KB  0x00020000
#define  SIZE_256KB  0x00040000
#define  SIZE_512KB  0x00080000
#define  SIZE_1MB    0x00100000
#define  SIZE_2MB    0x00200000
#define  SIZE_4MB    0x00400000
#define  SIZE_8MB    0x00800000
#define  SIZE_16MB   0x01000000
#define  SIZE_32MB   0x02000000
#define  SIZE_64MB   0x04000000
#define  SIZE_128MB  0x08000000
#define  SIZE_256MB  0x10000000
#define  SIZE_512MB  0x20000000


typedef  unsigned char     BYTE;
typedef  unsigned short    WORD;
typedef  unsigned int     DWORD;

typedef  unsigned char    UINT8;
typedef  unsigned short  UINT16;
typedef  unsigned int    UINT32;
typedef  long long       UINT64;

typedef  char              CHAR;
typedef  char              INT8;
typedef  short            INT16;
typedef  int              INT32;
typedef  unsigned int    size_t;
typedef  void              VOID;

#define  MMIO(x)  (*((volatile unsigned int *)(x)))
#define  MMIO_S(x)  (*((volatile unsigned short *)(x)))
#define  MMIO_B(x)  (*((volatile unsigned char *)(x)))

#define  MMIO_AND_OR(x, a, b)  MMIO(x) = (MMIO(x) & (a)) | (b)

typedef union {
  UINT64  Value;
  struct {
    UINT32  Lo;
    UINT32  Hi;
  } Part;
} UINT64_HL;

void *memcpy (void *d, void *s, size_t n);
void *memset (void *s, int c, size_t n);
int   memcmp (void *s1, void *s2, size_t n);
int   strncmpi (void *s1, void *s2, size_t n);
char *strcpy (char *dest,const char *src);
char *strcat (char *dest, const char *src);
int   strcmp (const char * cs,const char * ct);
int   strncmp (const char * cs,const char * ct, int count);
int   strlen (const char * s);

char  toupper (char ch);
char  tolower (char ch);
char  getchar (void);
void  putchar (char c);
char  haschar (void);

char *skipchar (char *str, char ch);
char *findchar (char *str, char ch);
char *findbyte (char *str, char ch);

unsigned long xtoi (char *str);

int snprintf ( char *s, size_t n, const char * format, ... );
#define sprintf(b, ...)  snprintf (b, 1024, ##__VA_ARGS__)
#define printf(...)      snprintf (0,    0, ##__VA_ARGS__)

void  puts (const char *s);

void  delay_ms (unsigned int ms);

unsigned int *irq(unsigned int *regs, unsigned int irqs);

int   test (void);

#endif
