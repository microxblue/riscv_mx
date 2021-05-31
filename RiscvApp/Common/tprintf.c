#include "firmware.h"

#define out(c)  	*bf++ = c

static
void putc (
  char   *ptr,
  char    ch
)
{
	if (ptr) {
		*ptr = ch;
	} else {
		putchar(ch);
	}
}

char haschar (void)
{
	if ((*(volatile uint32_t *)UART_LSR) & LSR_RXDA) {
		return 1;
	} else {
		return 0;
	}
}

char getchar (void)
{
  uint8_t src;

  do {
    src = haschar();
  } while (!src);

	return (char)(*(volatile uint32_t *)UART_DAT);
}

void putchar_uart (char ch)
{
    while ((*((volatile uint32_t*)UART_LSR) & LSR_TXRDY) == 0);
    *((volatile uint32_t*)UART_DAT) = ch;
}

void putchar (char ch)
{
	  putchar_uart (ch);
}

int snprintf ( char *s, size_t n, const char * fmt, ... )
{
	va_list      va;
	char         ch;
	unsigned int mask;
  unsigned int num;
  char         buf[12];
  char*        bf;
  char*        p;
  char*        sh;
  char*        st;
  char         uc;
  char         zs;

	va_start(va, fmt);

	sh = s;
	st = s + n;
	if (sh == 0)  st = 0;

	while ((ch=*(fmt++))) {
    if (ch!='%') {
			putc((sh < st) ? sh++ : 0, ch);
		}
		else {
			char lz=0;
			char w =1;
      char hx=0;
      unsigned char dgt;
			ch=*(fmt++);
			if (ch=='0') {
				ch=*(fmt++);
				lz=1;
			}
			if (ch>='0' && ch<='9') {
				w=0;
				while (ch>='0' && ch<='9') {
					w=(((w<<2)+w)<<1)+ch-'0';
					ch=*fmt++;
				}
			}
			bf=buf;
			p=bf;
			zs=0;
			switch (ch) {
				case 0:
					goto abort;

        case 'x':
				case 'X' :
          hx = 1;
				case 'u':
				case 'd' :
					num = va_arg(va, unsigned int);
	        uc  = (ch=='X') ? 'A' : 'a';
					if (ch=='d' && (int)num<0) {
						num = -(int)num;
						out('-');
					}
          if (hx)
            mask = 0x10000000;
          else
            mask = 1000000000;
          while (mask) {
            dgt = 0;
            while (num >= mask) {
		          num -= mask;
		          dgt++;
            }
            if (zs || dgt>0) {
              out(dgt+(dgt<10 ? '0' : uc-10));
	            zs=1;
	          }
            mask = hx ? mask >> 4 : mask / 10;
          }
          if (zs == 0) out('0');
					break;
				case 'c' :
					out((char)(va_arg(va, int)));
					break;
				case 's' :
					p=va_arg(va, char*);
					break;
				case '%' :
					out('%');
				default:
					break;
			}
			*bf=0;
			bf=p;
			while (*bf++ && w > 0)
				w--;
			while (w-- > 0)
        putc((sh < st) ? sh++ : 0, lz ? '0' : ' ');
			while ((ch= *p++))
				putc((sh < st) ? sh++ : 0, ch);
		}
	}
	if (sh < st) {
		*sh = 0;
	}

abort:;
	va_end(va);

	return sh - s;
}

