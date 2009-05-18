/*
 * Bob Jenkins' hash function, original code at:
 * http://burtleburtle.net/bob/c/lookup2.c
 *
 * Maintainance modifications for jenkins_hash-test.c:
 *
 * - SELF_TEST portions have been moved into a separate file,
 *   jenkins_hash-test.c, which can be used in TESTS of Makefile.am
 * - made test program return 0 on success, 1 on failure as required
 */

/*
--------------------------------------------------------------------
lookup2.c, by Bob Jenkins, December 1996, Public Domain.
hash(), hash2(), hash3, and mix() are externally useful functions.
Routines to test the hash are included if SELF_TEST is defined.
You can use this free for any purpose.  It has no warranty.
--------------------------------------------------------------------
*/
#define SELF_TEST

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "jhash.h"

#ifdef SELF_TEST

typedef ub4 (*hash_func) (ub1 * k, ub4 length, ub4 initval);

/* used for timings */
void driver1(hash_func hash)
{
  ub4 buf[256];
  ub4 i;
  ub4 h=0;

  for (i=0; i<256; ++i) 
  {
    h = hash((ub1 *)buf,i,h);
  }
}

/* check that every input bit changes every output bit half the time */
#define HASHSTATE 1
#define HASHLEN   1
#define MAXPAIR 80
#define MAXLEN 70
void driver2(hash_func hash)
{
  ub1 qa[MAXLEN+1], qb[MAXLEN+2], *a = &qa[0], *b = &qb[1];
  ub4 c[HASHSTATE], d[HASHSTATE], i, j=0, k, l, m=1, z;
  ub4 e[HASHSTATE],f[HASHSTATE],g[HASHSTATE],h[HASHSTATE];
  ub4 x[HASHSTATE],y[HASHSTATE];
  ub4 hlen;

  printf("No more than %d trials should ever be needed \n",MAXPAIR/2);
  for (hlen=0; hlen < MAXLEN; ++hlen)
  {
    z=0;
    for (i=0; i<hlen; ++i)  /*----------------------- for each input byte, */
    {
      for (j=0; j<8; ++j)   /*------------------------ for each input bit, */
      {
	for (m=1; m<8; ++m) /*------------ for serveral possible initvals, */
	{
	  for (l=0; l<HASHSTATE; ++l) e[l]=f[l]=g[l]=h[l]=x[l]=y[l]=~((ub4)0);

      	  /*---- check that every output bit is affected by that input bit */
	  for (k=0; k<MAXPAIR; k+=2)
	  { 
	    ub4 finished=1;
	    /* keys have one bit different */
	    for (l=0; l<hlen+1; ++l) {a[l] = b[l] = (ub1)0;}
	    /* have a and b be two keys differing in only one bit */
	    a[i] ^= (k<<j);
	    a[i] ^= (k>>(8-j));
	     c[0] = hash(a, hlen, m);
	    b[i] ^= ((k+1)<<j);
	    b[i] ^= ((k+1)>>(8-j));
	     d[0] = hash(b, hlen, m);
	    /* check every bit is 1, 0, set, and not set at least once */
	    for (l=0; l<HASHSTATE; ++l)
	    {
	      e[l] &= (c[l]^d[l]);
	      f[l] &= ~(c[l]^d[l]);
	      g[l] &= c[l];
	      h[l] &= ~c[l];
	      x[l] &= d[l];
	      y[l] &= ~d[l];
	      if (e[l]|f[l]|g[l]|h[l]|x[l]|y[l]) finished=0;
	    }
	    if (finished) break;
	  }
	  if (k>z) z=k;
	  if (k==MAXPAIR) 
	  {
	     printf("Some bit didn't change: ");
	     printf("%.8lx %.8lx %.8lx %.8lx %.8lx %.8lx  ",
	            e[0],f[0],g[0],h[0],x[0],y[0]);
	     printf("i %ld j %ld m %ld len %ld\n",i,j,m,hlen);
	     exit (1);
	  }
	  if (z==MAXPAIR) goto done;
	}
      }
    }
   done:
    if (z < MAXPAIR)
    {
      printf("Mix success  %2ld bytes  %2ld initvals  ",i,m);
      printf("required  %ld  trials\n",z/2);
    }
  }
  printf("\n");
}

/* Check for reading beyond the end of the buffer and alignment problems */
void driver3(hash_func hash)
{
  ub1 buf[MAXLEN+20], *b;
  ub4 len;
  ub1 q[] = "This is the time for all good men to come to the aid of their country";
  ub1 qq[] = "xThis is the time for all good men to come to the aid of their country";
  ub1 qqq[] = "xxThis is the time for all good men to come to the aid of their country";
  ub1 qqqq[] = "xxxThis is the time for all good men to come to the aid of their country";
  ub4 h,i,j,ref,x,y;

  printf("Endianness.  These should all be the same:\n");
  printf("%.8lx\n", hash(q, sizeof(q)-1, (ub4)0));
  printf("%.8lx\n", hash(qq+1, sizeof(q)-1, (ub4)0));
  printf("%.8lx\n", hash(qqq+2, sizeof(q)-1, (ub4)0));
  printf("%.8lx\n", hash(qqqq+3, sizeof(q)-1, (ub4)0));
  printf("\n");
  for (h=0, b=buf+1; h<8; ++h, ++b)
  {
    for (i=0; i<MAXLEN; ++i)
    {
      len = i;
      for (j=0; j<i; ++j) *(b+j)=0;

      /* these should all be equal */
      ref = hash(b, len, (ub4)1);
      *(b+i)=(ub1)~0;
      *(b-1)=(ub1)~0;
      x = hash(b, len, (ub4)1);
      y = hash(b, len, (ub4)1);
      if ((ref != x) || (ref != y)) 
      {
	printf("alignment error: %.8lx %.8lx %.8lx %ld %ld\n",ref,x,y,h,i);
      }
    }
  }
}

/* check for problems with nulls */
 void driver4(hash_func hash)
{
  ub1 buf[1];
  ub4 h,i,state[HASHSTATE];


  buf[0] = ~0;
  for (i=0; i<HASHSTATE; ++i) state[i] = 1;
  printf("These should all be different\n");
  for (i=0, h=0; i<8; ++i)
  {
    h = hash(buf, (ub4)0, h);
    printf("%2ld  0-byte strings, hash is  %.8lx\n", i, h);
  }
}

int main (int argc, char * argv[])
{
  driver1(jenkins_hash);   /* test that the key is hashed: used for timings */
  driver2(jenkins_hash);   /* test that whole key is hashed thoroughly */
  driver3(jenkins_hash);   /* test that nothing but the key is hashed */
  driver4(jenkins_hash);   /* test hashing multiple buffers (all buffers are null) */

  driver1((hash_func)jenkins_hash2);   /* test that the key is hashed: used for timings */
  driver2((hash_func)jenkins_hash2);   /* test that whole key is hashed thoroughly */
  driver4((hash_func)jenkins_hash2);   /* test hashing multiple buffers (all buffers are null) */

  exit (0);
}

#endif  /* SELF_TEST */
