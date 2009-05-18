/** \file
 * Bob Jenkins' hash function, original code at:
 * http://burtleburtle.net/bob/c/lookup2.c
 */

/*
--------------------------------------------------------------------
lookup2.c, by Bob Jenkins, December 1996, Public Domain.
hash(), hash2(), hash3, and mix() are externally useful functions.
Routines to test the hash are included if SELF_TEST is defined.
You can use this free for any purpose.  It has no warranty.
--------------------------------------------------------------------
*/
#ifndef __JHASH_H__
#define __JHASH_H__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

/**
 Hash a variable-length key into a 32-bit value

 \param k the key (the unaligned variable-length array of bytes)
 \param len the length of the key, counting by bytes
 \param initval the previous hash, or any 4-byte arbitrary value

 \returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Every 1-bit and 2-bit delta achieves avalanche.
About 36+6len instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
<pre>
  h = (h & hashmask(10));
</pre>
In which case, the hash table should have \a hashsize(10) elements.

If you are hashing n strings (ub1 **)k, do it like this:
<pre>
  for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);
</pre>

By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

See http://burlteburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^32 is
acceptable.  Do NOT use for cryptographic purposes.
 */
ub4 jenkins_hash(ub1 * k, ub4 length, ub4 initval);

/**
 An optimised hash function for keys of length 4n bytes.
 * \param k the key
 * \param length the length of the key, in ub4s
 * \param initval the previous hash, or an arbitrary value

 This works on all machines.  jenkins_hash2() is identical to jenkins_hash()
 on little-endian machines, except that the length has to be measured
 in ub4s instead of bytes.  It is much faster than jenkins_hash().  It 
 requires:
 - that the key be an array of ub4's, and
 - that all your machines have the same endianness, and
 - that the length be the number of ub4's in the key
*/
ub4 jenkins_hash2(ub4 * k, ub4 length, ub4 initval);

/**
 Jenkins little-endian ONLY hash function (NOT PORTABLE)

 \param k the key
 \param length the length of the key
 \param initval the previous hash, or an arbitrary value

 This is identical to jenkins_hash() on little-endian machines (like Intel 
 x86s or VAXen).  It gives nondeterministic results on big-endian
 machines.  It is faster than jenkins_hash(), but a little slower than 
 jenkins_hash2(), and it requires:
 - that all your machines be little-endian
 */
ub4 jenkins_hash3(ub1 * k, ub4 length, ub4 initval);

#endif /* __JHASH_H__ */
