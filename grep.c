#include "grep.h"
int vflag= 1, oflag, listf, listn, col, names[26],anymarks;
int tfile = -1, tline, iblock = -1, oblock = -1, ichanged, nleft;
char WRERR[] = "WRITE ERROR", *braslist[NBRA], *braelist[NBRA], obuff[BLKSIZE];
int nbra, subnewa, subolda,  fchange, wrapp, bpagesize = 20, peekc, lastc, iter = 1, ninbuf, fc, io, pflag, given;
char savedfile[FNSIZE], regx[ESIZE+4], *filelist[FNSIZE], genbuf[LBSIZE];
char linebuf[LBSIZE], expbuf[ESIZE+4], *nextip, *linebp, Q[] = "", T[] = "TMP", *globp, *tfname, *loc1, *loc2, ibuff[BLKSIZE];
unsigned int *addr1, *addr2, *dot, *dol, *zero;
long count;
unsigned nlall = 128;

int main(int argc, char *argv[]) {
        char *p1, *p2;
	argv++, fc = argc-2;
	for(int i = 0; i < fc; ++i){filelist[i] = argv[i+1];}
	if (argc>2) {
	  p1 = argv[0], p2 = regx;
	  while ((*p2++ = *p1++)){if (p2 >= &regx[sizeof(regx)]){p2--;}}
	  p1 = argv[1], p2 = savedfile;
	  while ((*p2++ = *p1++)){if (p2 >= &savedfile[sizeof(savedfile)]){p2--;}}
	}
	else{
	  printf("Usage: <string> <files>\n");
	  quit(0);
	  }
	zero = (unsigned *)malloc(nlall*sizeof(unsigned)); init();
	for(;;){
	  io = open(savedfile, 0);
	  setwide();
	  squeeze(0);
	  ninbuf = 0;
	  append(getfile, addr2);
	  exfile();
	  global(1);
	  --fc;
	  memset(linebuf, 0, sizeof(linebuf));
	  memset(ibuff, 0, sizeof(ibuff));
	  memset(obuff, 0, sizeof(obuff));
	  if(fc < 1){exit(0);}
	  else{
	    p1 = filelist[iter], p2 = savedfile;
	    while ((*p2++ = *p1++)){if (p2 >= &savedfile[sizeof(savedfile)]){p2--;}}
	    if(fc > 1){++iter;}
	    globp = 0;
	  }
	}
	quit(0);
	return 0;
}
void print(void) {
	unsigned int *a1 = addr1;
	do {
		if (listn) {count = a1-zero; putchr('\t');}
		puts(m_getline(*a1++));
	} while (a1 <= addr2);
	dot = addr2, listf = 0, listn = 0, pflag = 0;
}
int getnum(void) {
	int r = 0, c;
	while ((c=getchr())>='0' && c<='9'){r = r*10 + c - '0';}
	peekc = c;
	return (r);
}
void setwide(void) {
	if (!given) {addr1 = zero + (dol>zero); addr2 = dol;}
}
void setnoaddr(void) {
        if (given){error(Q);}
}
void squeeze(int i) {
  if (addr1<zero+i || addr2>dol || addr1>addr2){error(Q);}
}
void exfile(void) {
	close(io); io = -1;
}
void onintr(int n) {
	putchr('\n');
	lastc = '\n';
	error(Q);
}
void onhup(int n) {
	if (dol > zero) {
		addr1 = zero+1; addr2 = dol; io = creat("ed.hup", 0600);
		if (io > 0){putfile();}
	}
	fchange = 0;
	quit(0);
}
void error(char *s) {
	int c;
	wrapp = 0, listf = 0, listn = 0;
	putchr('?');
	puts(s);
	count = 0;
	lseek(0, (long)0, 2);
	pflag = 0;
	if (globp){lastc = '\n';}
	globp = 0, peekc = lastc;
	if(lastc){while ((c = getchr()) != '\n' && c != EOF);}
	if (io > 0){close(io); io = -1;}
	quit(1);
}
int getchr(void) {
	char c;
	if ((lastc=peekc)) {
		peekc = 0;
		return(lastc);
	}
	if (globp) {
	  if ((lastc = *globp++) != 0){return(lastc);}
	  globp = 0;
	  return(EOF);
	}
	if (read(0, &c, 1) <= 0){return(lastc = EOF);}
	lastc = c&0177;
	return(lastc);
}
int getfile(void) {
	int c;
	char *lp, *fp;
	lp = linebuf, fp = nextip;
	do {
	  if (--ninbuf < 0) {
	    if ((ninbuf = read(io, genbuf, LBSIZE)-1) < 0){
	      if (lp>linebuf) {puts("'\\n' appended"); *genbuf = '\n';}
	      else{return(EOF);}
	    }
	    fp = genbuf;
	    while(fp < &genbuf[ninbuf]) {if (*fp++ & 0200){break;}}
	    fp = genbuf;
		  
	  }
	  c = *fp++;
	  if (c=='\0'){continue;}
	  if (c&0200 || lp >= &linebuf[LBSIZE]) {lastc = '\n';
	    error(Q);}
	  *lp++ = c, count++;
	} while (c != '\n');
	*--lp = 0, nextip = fp;
	return(0);
}
void putfile(void) {
	unsigned int *a1;
	int n, nib;
	char *fp, *lp;
	nib = BLKSIZE, fp = genbuf, a1 = addr1;
	do {
		lp = m_getline(*a1++);
		for (;;) {
			if (--nib < 0) {
				n = fp-genbuf;
				if(write(io, genbuf, n) != n) {puts(WRERR); error(Q);}
				nib = BLKSIZE-1, fp = genbuf;
			}
			count++;
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);
	n = fp-genbuf;
	if(write(io, genbuf, n) != n) {puts(WRERR);
	  error(Q);}
}
int append(int (*f)(void), unsigned int *a) {
	unsigned int *a1, *a2, *rdot;
	int nline, tl;
	nline = 0, dot = a;
	while ((*f)() == 0) {
		if ((dol-zero)+1 >= nlall) {
			unsigned *ozero = zero;
			nlall += 1024;
			if ((zero = (unsigned *)realloc((char *)zero, nlall*sizeof(unsigned)))==NULL) {error("MEM?");
			  onhup(0);}
			dot += zero - ozero, dol += zero - ozero;
		}
		tl = putline();
		nline++;
		a1 = ++dol, a2 = a1+1, rdot = ++dot;
		while (a1 > rdot){*--a2 = *--a1;}
		*rdot = tl;
	}
	return(nline);
}
void quit(int n) {
	if (vflag && fchange && dol!=zero) {fchange = 0;
	  error(Q);}
	unlink(tfname);
	exit(0);
}
void gdelete(void) {
  unsigned int *a1, *a2, *a3 = dol;
	for (a1=zero; (*a1&01)==0; a1++){if (a1>=a3){return;}}
	for (a2=a1+1;
	     a2<=a3;) {
		if (*a2&01) {a2++;
		  dot = a1;}
		else{*a1++ = *a2++;}
	}
	dol = a1-1;
	if (dot>dol){dot = dol;}
	fchange = 1;
}
char *m_getline(unsigned int tl) {
	char *bp, *lp;
	int nl;
	lp = linebuf, bp = getblock(tl, READ), nl = nleft, tl &= ~((BLKSIZE/2)-1);
	while ((*lp++ = *bp++)){if (--nl == 0) {bp = getblock(tl+=(BLKSIZE/2), READ);
	    nl = nleft;}}
	return(linebuf);
}
int putline(void) {
	char *bp, *lp;
	int nl;
	unsigned int tl;
	fchange = 1, lp = linebuf, tl = tline, bp = getblock(tl, WRITE), nl = nleft, tl &= ~((BLKSIZE/2)-1);
	while ((*bp = *lp++)) {
		if (*bp++ == '\n') {
		  *--bp = 0, linebp = lp;
			break;
		}
		if (--nl == 0) {bp = getblock(tl+=(BLKSIZE/2), WRITE); nl = nleft;}
	}
	nl = tline;
	tline += (((lp-linebuf)+03)>>1)&077776;
	return(nl);
}
char *getblock(unsigned int atl, int iof) {
	int bno, off;
	bno = (atl/(BLKSIZE/2)), off = (atl<<1) & (BLKSIZE-1) & ~03;
	if (bno >= NBLK) {lastc = '\n'; error(T);}
	nleft = BLKSIZE - off;
	if (bno==iblock) {
		ichanged |= iof;
		return(ibuff+off);
	}
	if (bno==oblock){return(obuff+off);}
	if (iof==READ) {
	  if (ichanged){blkio(iblock, ibuff, write);}
	  ichanged = 0, iblock = bno;
	  blkio(bno, ibuff, read);
	  return(ibuff+off);
	}
	if (oblock>=0){blkio(oblock, obuff, write);}
	oblock = bno;
	return(obuff+off);
}
void blkio(int b, char *buf, int (*iofcn)(int, char*, int)) {
	lseek(tfile, (long)b*BLKSIZE, 0);
	if ((*iofcn)(tfile, buf, BLKSIZE) != BLKSIZE) {error(T);}
}
void init(void) {
	int *markp;
	close(tfile); tline = 2;
	for (markp = names; markp < &names[26]; ){*markp++ = 0;}
	subnewa = 0, anymarks = 0, iblock = -1, oblock = -1; ichanged = 0;
	close(creat(tfname, 0600)); tfile = open(tfname, 2); dot = dol = zero;
}
void global(int k) {
        char *gp;
	int c;
	unsigned int *a1;
	char globuf[GBSIZE]; char tmp[GBSIZE] = "/";
	if (globp){error(Q);}
	setwide(), squeeze(dol>zero);
	globp = strcat(regx, "\n"), globp = strcat(tmp, regx);
	if ((c=getchr())=='\n'){error(Q);}
	compile(c), gp = globuf;
	while ((c = getchr()) != '\n') {
	  if (c==EOF){error(Q);}
	  if (c=='\\') {
	    c = getchr();
	    if (c!='\n'){*gp++ = '\\';}
	  }
	  *gp++ = c;
	  if (gp >= &globuf[GBSIZE-2]){error(Q);}
	}
	if (gp == globuf){*gp++ = 'p';}
	*gp++ = '\n'; *gp++ = 0;
	for (a1=zero; a1<=dol; a1++) {
		*a1 &= ~01;
		if (a1>=addr1 && a1<=addr2 && execute(a1)==k){*a1 |= 01;}
	}
	if (globuf[0]=='d' && globuf[1]=='\n' && globuf[2]=='\0') {
		gdelete();
		return;
	}
	for (a1=zero; a1<=dol; a1++) {
		if (*a1 & 01) {
		  *a1 &= ~01; dot = a1; globp = globuf, addr1 = 0; a1 = 0; addr2 = a1, given = 0; addr2 = dot; addr1 = addr2;
		  printf("%s: ", savedfile);
		  print(); a1 = zero;
		}
	}
}
void compile(int eof) {
        int c, cclcnt;
	char *ep; char *lastep;
	char bracket[NBRA], *bracketp;
	ep = expbuf, bracketp = bracket;
	if ((c = getchr()) == '\n') {peekc = c; c = eof;}
	if (c == eof) {quit(1);}
	nbra = 0;
	if (c=='^') {c = getchr(), *ep++ = CCIRC;}
	peekc = c, lastep = 0;
	for (;;) {
		c = getchr();
		if (c == '\n') {peekc = c, c = eof;}
		if (c==eof) {
			*ep++ = CEOF;
			return;
		}
		if (c!='*'){lastep = ep;}
		switch (c) {
		case '\\':
			if ((c = getchr())=='(') {
			  *bracketp++ = nbra, *ep++ = CBRA, *ep++ = nbra++;
				continue;
			}
			if (c == ')') {
			  *ep++ = CKET, *ep++ = *--bracketp;
				continue;
			}
			if (c>='1' && c<'1'+NBRA) {
			  *ep++ = CBACK, *ep++ = c-'1';
				continue;
			}
			*ep++ = CCHR, *ep++ = c;
			continue;
		case '.':
			*ep++ = CDOT;
			continue;
		case '*':
			if (lastep==0 || *lastep==CBRA || *lastep==CKET)
			*lastep |= STAR;
			continue;
		case '$':
			if ((peekc=getchr()) != eof && peekc!='\n')
			*ep++ = CDOL;
			continue;
		case '[':
		  *ep++ = CCL, *ep++ = 0, cclcnt = 1;
			if ((c=getchr()) == '^') {c = getchr(); ep[-2] = NCCL;}
			do {
				if (c=='-' && ep[-1]!=0) {
					if ((c=getchr())==']') {
						*ep++ = '-'; cclcnt++;
						break;
					}
					while (ep[-1]<c) {*ep = ep[-1]+1; ep++; cclcnt++;}
				}
				*ep++ = c; cclcnt++;
			} while ((c = getchr()) != ']');
			lastep[1] = cclcnt;
			continue;

		default:
		  *ep++ = CCHR, *ep++ = c;
		}
	}
}
int execute(unsigned int *addr) {
	char *p1, *p2;
	int c;
	for (c=0; c<NBRA; c++) {braslist[c] = 0; braelist[c] = 0;}
	p2 = expbuf;
	if (addr == (unsigned *)0) {
	  if (*p2==CCIRC){return(0);}
	  p1 = loc2;
	} else if (addr==zero){return(0);}
	else{p1 = m_getline(*addr);}
	if (*p2==CCIRC) {
		loc1 = p1;
		return(advance(p1, p2+1));
	}
	if (*p2==CCHR) {
		c = p2[1];
		do {
		  if (*p1!=c){continue;}
			if (advance(p1, p2)) {
			  loc1 = p1;
			  return(1);
			}
		} while (*p1++);
		return(0);
	}
	do {
		if (advance(p1, p2)) {
			loc1 = p1;
			return(1);
		}
	} while (*p1++);
	return(0);
}
int advance(char *lp, char *ep) {
	char *curlp;
	int i;
	for (;;) switch (*ep++) {
	case CCHR:
	  if (*ep++ == *lp++){continue;}
	  return(0);
	case CDOT:
	  if (*lp++){continue;}
	  return(0);
	case CDOL:
	  if (*lp==0){continue;}
	  return(0);
	case CEOF:
	  loc2 = lp;
	  return(1);
	case CCL:
	  if (cclass(ep, *lp++, 1)) {
	    ep += *ep;
	    continue;
	  }
	  return(0);
	case NCCL:
	  if (cclass(ep, *lp++, 0)) {
	    ep += *ep;
	    continue;
	  }
	  return(0);
	case CBRA:
	  braslist[*ep++] = lp;
	  continue;
	case CKET:
	  braelist[*ep++] = lp;
	  continue;
	case CBACK:
	  if (braelist[i = *ep++]==0){error(Q);}
	  if (backref(i, lp)) {
	    lp += braelist[i] - braslist[i];
	    continue;
	  }
	  return(0);
	case CBACK|STAR:
	  if (braelist[i = *ep++] == 0){error(Q);}
		curlp = lp;
		while (backref(i, lp)){lp += braelist[i] - braslist[i];}
		while (lp >= curlp) {if (advance(lp, ep)){return(1);}lp -= braelist[i] - braslist[i];}
		continue;
	case CDOT|STAR:
		curlp = lp;
		while (*lp++);
	case CCHR|STAR:
		curlp = lp;
		while (*lp++ == *ep);
		ep++;
	case CCL|STAR:
	case NCCL|STAR:
		curlp = lp;
		while (cclass(ep, *lp++, ep[-1]==(CCL|STAR)));
		ep += *ep;
	default:
	  error(Q);
	}
}
int backref(int i, char *lp) {
	char *bp;
	bp = braslist[i];
	while (*bp++ == *lp++){if (bp >= braelist[i]){return(1);}}
	return(0);
}
int cclass(char *set, int c, int af) {
	int n;
	if (c==0){return(0);}
	n = *set++;
	while (--n){if (*set++ == c){return(af);}}
	return(!af);
}
char	line[70];
char	*linp	= line;
void putchr(int ac) {
	char *lp;
	int c;
	lp = linp, c = ac;
	if (listf) {
		if (c=='\n') {if (linp!=line && linp[-1]==' ') {*lp++ = '\\'; *lp++ = 'n';}}
		else {
			if (col > (72-4-2)) {col = 8; *lp++ = '\\'; *lp++ = '\n';*lp++ = '\t';}
			col++;
			if (c=='\b' || c=='\t' || c=='\\') {
				*lp++ = '\\';
				if (c=='\b'){c = 'b';}
				else if (c=='\t'){c = 't';}
				col++;
			} else if (c<' ' || c=='\177') {
				*lp++ = '\\'; *lp++ =  (c>>6)    +'0';
				*lp++ = ((c>>3)&07)+'0'; c     = ( c    &07)+'0';col += 3;
			}
		}
	}
	*lp++ = c;
	if(c == '\n' || lp >= &line[64]) {
		linp = line; write(oflag?2:1, line, lp-line);
		return;
	}
	linp = lp;
}
