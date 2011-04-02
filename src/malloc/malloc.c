#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>
#include "libc.h"
#include "atomic.h"
#include "pthread_impl.h"

uintptr_t __brk(uintptr_t);
void *__mmap(void *, size_t, int, int, int, off_t);
int __munmap(void *, size_t);
void *__mremap(void *, size_t, size_t, int, ...);
int __madvise(void *, size_t, int);

struct chunk {
	size_t data[1];
	struct chunk *next;
	struct chunk *prev;
};

struct bin {
	int lock[2];
	struct chunk *head;
	struct chunk *tail;
};

static struct {
	uintptr_t brk;
	size_t *heap;
	uint64_t binmap;
	struct bin bins[64];
	int brk_lock[2];
	int free_lock[2];
} mal;


#define SIZE_ALIGN (4*sizeof(size_t))
#define SIZE_MASK (-SIZE_ALIGN)
#define OVERHEAD (2*sizeof(size_t))
#define MMAP_THRESHOLD (0x1c00*SIZE_ALIGN)
#define DONTCARE 16
#define RECLAIM 163840

#define CHUNK_SIZE(c) ((c)->data[0] & SIZE_MASK)
#define CHUNK_PSIZE(c) ((c)->data[-1] & SIZE_MASK)
#define PREV_CHUNK(c) ((struct chunk *)((char *)(c) - CHUNK_PSIZE(c)))
#define NEXT_CHUNK(c) ((struct chunk *)((char *)(c) + CHUNK_SIZE(c)))
#define MEM_TO_CHUNK(p) (struct chunk *)((size_t *)p - 1)
#define CHUNK_TO_MEM(c) (void *)((c)->data+1)
#define BIN_TO_CHUNK(i) (MEM_TO_CHUNK(&mal.bins[i].head))

#define C_INUSE  ((size_t)1)
#define C_FLAGS  ((size_t)3)
#define C_SIZE   SIZE_MASK

#define IS_MMAPPED(c) !((c)->data[0] & (C_INUSE))


/* Synchronization tools */

static void lock(volatile int *lk)
{
	if (!libc.threads_minus_1) return;
	while(a_swap(lk, 1)) __wait(lk, lk+1, 1, 1);
}

static void unlock(volatile int *lk)
{
	if (!libc.threads_minus_1) return;
	a_store(lk, 0);
	if (lk[1]) __wake(lk, 1, 1);
}

static void lock_bin(int i)
{
	if (libc.threads_minus_1)
		lock(mal.bins[i].lock);
	if (!mal.bins[i].head)
		mal.bins[i].head = mal.bins[i].tail = BIN_TO_CHUNK(i);
}

static void unlock_bin(int i)
{
	if (!libc.threads_minus_1) return;
	unlock(mal.bins[i].lock);
}

static int first_set(uint64_t x)
{
#if 1
	return a_ctz_64(x);
#else
	static const char debruijn64[64] = {
		0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
		62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
		63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
	};
	static const char debruijn32[32] = {
		0, 1, 23, 2, 29, 24, 19, 3, 30, 27, 25, 11, 20, 8, 4, 13,
		31, 22, 28, 18, 26, 10, 7, 12, 21, 17, 9, 6, 16, 5, 15, 14
	};
	if (sizeof(long) < 8) {
		uint32_t y = x;
		if (!y) {
			y = x>>32;
			return 32 + debruijn32[(y&-y)*0x076be629 >> 27];
		}
		return debruijn32[(y&-y)*0x076be629 >> 27];
	}
	return debruijn64[(x&-x)*0x022fdd63cc95386dull >> 58];
#endif
}

static int bin_index(size_t x)
{
	x = x / SIZE_ALIGN - 1;
	if (x <= 32) return x;
	if (x > 0x1c00) return 63;
	return ((union { float v; uint32_t r; }){ x }.r>>21) - 496;
}

static int bin_index_up(size_t x)
{
	x = x / SIZE_ALIGN - 1;
	if (x <= 32) return x;
	return ((union { float v; uint32_t r; }){ x }.r+0x1fffff>>21) - 496;
}

#if 0
void __dump_heap(int x)
{
	struct chunk *c;
	int i;
	for (c = (void *)mal.heap; CHUNK_SIZE(c); c = NEXT_CHUNK(c))
		fprintf(stderr, "base %p size %zu (%d) flags %d/%d\n",
			c, CHUNK_SIZE(c), bin_index(CHUNK_SIZE(c)),
			c->data[0] & 15,
			NEXT_CHUNK(c)->data[-1] & 15);
	for (i=0; i<64; i++) {
		if (mal.bins[i].head != BIN_TO_CHUNK(i) && mal.bins[i].head) {
			fprintf(stderr, "bin %d: %p\n", i, mal.bins[i].head);
			if (!(mal.binmap & 1ULL<<i))
				fprintf(stderr, "missing from binmap!\n");
		} else if (mal.binmap & 1ULL<<i)
			fprintf(stderr, "binmap wrongly contains %d!\n", i);
	}
}
#endif

static struct chunk *expand_heap(size_t n)
{
	struct chunk *w;
	uintptr_t new;

	lock(mal.brk_lock);

	if (n > SIZE_MAX - mal.brk - 2*PAGE_SIZE) goto fail;
	new = mal.brk + n + SIZE_ALIGN + PAGE_SIZE - 1 & -PAGE_SIZE;
	n = new - mal.brk;

	if (__brk(new) != new) goto fail;

	w = MEM_TO_CHUNK(new);
	w->data[-1] = n | C_INUSE;
	w->data[0] = 0 | C_INUSE;

	w = MEM_TO_CHUNK(mal.brk);
	w->data[0] = n | C_INUSE;
	mal.brk = new;
	
	unlock(mal.brk_lock);

	return w;
fail:
	unlock(mal.brk_lock);
	return 0;
}

static int init_malloc(size_t n)
{
	static int init, waiters;
	int state;
	struct chunk *c;

	if (init == 2) return 0;

	while ((state=a_swap(&init, 1)) == 1)
		__wait(&init, &waiters, 1, 1);
	if (state) {
		a_store(&init, 2);
		return 0;
	}

	mal.brk = __brk(0) + 2*SIZE_ALIGN-1 & -SIZE_ALIGN;

	c = expand_heap(n);

	if (!c) {
		a_store(&init, 0);
		if (waiters) __wake(&init, 1, 1);
		return -1;
	}

	mal.heap = (void *)c;
	c->data[-1] = 0 | C_INUSE;
	free(CHUNK_TO_MEM(c));

	a_store(&init, 2);
	if (waiters) __wake(&init, -1, 1);
	return 1;
}

static int adjust_size(size_t *n)
{
	/* Result of pointer difference must fit in ptrdiff_t. */
	if (*n-1 > PTRDIFF_MAX - SIZE_ALIGN - PAGE_SIZE) {
		if (*n) {
			errno = ENOMEM;
			return -1;
		} else {
			*n = SIZE_ALIGN;
			return 0;
		}
	}
	*n = (*n + OVERHEAD + SIZE_ALIGN - 1) & SIZE_MASK;
	return 0;
}

static void unbin(struct chunk *c, int i)
{
	if (c->prev == c->next)
		a_and_64(&mal.binmap, ~(1ULL<<i));
	c->prev->next = c->next;
	c->next->prev = c->prev;
	c->data[0] |= C_INUSE;
	NEXT_CHUNK(c)->data[-1] |= C_INUSE;
}

static int alloc_fwd(struct chunk *c)
{
	int i;
	size_t k;
	while (!((k=c->data[0]) & C_INUSE)) {
		i = bin_index(k);
		lock_bin(i);
		if (c->data[0] == k) {
			unbin(c, i);
			unlock_bin(i);
			return 1;
		}
		unlock_bin(i);
	}
	return 0;
}

static int alloc_rev(struct chunk *c)
{
	int i;
	size_t k;
	while (!((k=c->data[-1]) & C_INUSE)) {
		i = bin_index(k);
		lock_bin(i);
		if (c->data[-1] == k) {
			unbin(PREV_CHUNK(c), i);
			unlock_bin(i);
			return 1;
		}
		unlock_bin(i);
	}
	return 0;
}


/* pretrim - trims a chunk _prior_ to removing it from its bin.
 * Must be called with i as the ideal bin for size n, j the bin
 * for the _free_ chunk self, and bin j locked. */
static int pretrim(struct chunk *self, size_t n, int i, int j)
{
	size_t n1;
	struct chunk *next, *split;

	/* We cannot pretrim if it would require re-binning. */
	if (j < 40) return 0;
	if (j < i+3) {
		if (j != 63) return 0;
		n1 = CHUNK_SIZE(self);
		if (n1-n <= MMAP_THRESHOLD) return 0;
	} else {
		n1 = CHUNK_SIZE(self);
	}
	if (bin_index(n1-n) != j) return 0;

	next = NEXT_CHUNK(self);
	split = (void *)((char *)self + n);

	split->prev = self->prev;
	split->next = self->next;
	split->prev->next = split;
	split->next->prev = split;
	split->data[-1] = n | C_INUSE;
	split->data[0] = n1-n;
	next->data[-1] = n1-n;
	self->data[0] = n | C_INUSE;
	return 1;
}

static void trim(struct chunk *self, size_t n)
{
	size_t n1 = CHUNK_SIZE(self);
	struct chunk *next, *split;

	if (n >= n1 - DONTCARE) return;

	next = NEXT_CHUNK(self);
	split = (void *)((char *)self + n);

	split->data[-1] = n | C_INUSE;
	split->data[0] = n1-n | C_INUSE;
	next->data[-1] = n1-n | C_INUSE;
	self->data[0] = n | C_INUSE;

	free(CHUNK_TO_MEM(split));
}

void *malloc(size_t n)
{
	struct chunk *c;
	int i, j;

	if (adjust_size(&n) < 0) return 0;

	if (n > MMAP_THRESHOLD) {
		size_t len = n + PAGE_SIZE - 1 & -PAGE_SIZE;
		char *base = __mmap(0, len, PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (base == (void *)-1) return 0;
		c = (void *)(base + SIZE_ALIGN - sizeof(size_t));
		c->data[0] = len - (SIZE_ALIGN - sizeof(size_t));
		c->data[-1] = SIZE_ALIGN - sizeof(size_t);
		return CHUNK_TO_MEM(c);
	}

	i = bin_index_up(n);
	for (;;) {
		uint64_t mask = mal.binmap & -(1ULL<<i);
		if (!mask) {
			if (init_malloc(n) > 0) continue;
			c = expand_heap(n);
			if (!c) return 0;
			if (alloc_rev(c)) {
				struct chunk *x = c;
				c = PREV_CHUNK(c);
				NEXT_CHUNK(x)->data[-1] = c->data[0] =
					x->data[0] + CHUNK_SIZE(c);
			}
			break;
		}
		j = first_set(mask);
		lock_bin(j);
		c = mal.bins[j].head;
		if (c != BIN_TO_CHUNK(j) && j == bin_index(c->data[0])) {
			if (!pretrim(c, n, i, j)) unbin(c, j);
			unlock_bin(j);
			break;
		}
		unlock_bin(j);
	}

	/* Now patch up in case we over-allocated */
	trim(c, n);

	return CHUNK_TO_MEM(c);
}

void *realloc(void *p, size_t n)
{
	struct chunk *self, *next;
	size_t n0, n1;
	void *new;

	if (!p) return malloc(n);

	if (adjust_size(&n) < 0) return 0;

	self = MEM_TO_CHUNK(p);
	n1 = n0 = CHUNK_SIZE(self);

	if (IS_MMAPPED(self)) {
		size_t extra = self->data[-1];
		char *base = (char *)self - extra;
		size_t oldlen = n0 + extra;
		size_t newlen = n + extra;
		/* Crash on realloc of freed chunk */
		if ((uintptr_t)base < mal.brk) *(char *)0=0;
		if (newlen < PAGE_SIZE && (new = malloc(n))) {
			memcpy(new, p, n-OVERHEAD);
			free(p);
			return new;
		}
		newlen = (newlen + PAGE_SIZE-1) & -PAGE_SIZE;
		if (oldlen == newlen) return p;
		base = __mremap(base, oldlen, newlen, MREMAP_MAYMOVE);
		if (base == (void *)-1)
			return newlen < oldlen ? p : 0;
		self = (void *)(base + extra);
		self->data[0] = newlen - extra;
		return CHUNK_TO_MEM(self);
	}

	next = NEXT_CHUNK(self);

	/* Merge adjacent chunks if we need more space. This is not
	 * a waste of time even if we fail to get enough space, because our
	 * subsequent call to free would otherwise have to do the merge. */
	if (n > n1 && alloc_fwd(next)) {
		n1 += CHUNK_SIZE(next);
		next = NEXT_CHUNK(next);
	}
	/* FIXME: find what's wrong here and reenable it..? */
	if (0 && n > n1 && alloc_rev(self)) {
		self = PREV_CHUNK(self);
		n1 += CHUNK_SIZE(self);
	}
	self->data[0] = n1 | C_INUSE;
	next->data[-1] = n1 | C_INUSE;

	/* If we got enough space, split off the excess and return */
	if (n <= n1) {
		//memmove(CHUNK_TO_MEM(self), p, n0-OVERHEAD);
		trim(self, n);
		return CHUNK_TO_MEM(self);
	}

	/* As a last resort, allocate a new chunk and copy to it. */
	new = malloc(n-OVERHEAD);
	if (!new) return 0;
	memcpy(new, p, n0-OVERHEAD);
	free(CHUNK_TO_MEM(self));
	return new;
}

void free(void *p)
{
	struct chunk *self = MEM_TO_CHUNK(p);
	struct chunk *next;
	size_t final_size, new_size, size;
	int reclaim=0;
	int i;

	if (!p) return;

	if (IS_MMAPPED(self)) {
		size_t extra = self->data[-1];
		char *base = (char *)self - extra;
		size_t len = CHUNK_SIZE(self) + extra;
		/* Crash on double free */
		if ((uintptr_t)base < mal.brk) *(char *)0=0;
		__munmap(base, len);
		return;
	}

	final_size = new_size = CHUNK_SIZE(self);
	next = NEXT_CHUNK(self);

	for (;;) {
		/* Replace middle of large chunks with fresh zero pages */
		if (reclaim && (self->data[-1] & next->data[0] & C_INUSE)) {
			uintptr_t a = (uintptr_t)self + SIZE_ALIGN+PAGE_SIZE-1 & -PAGE_SIZE;
			uintptr_t b = (uintptr_t)next - SIZE_ALIGN & -PAGE_SIZE;
#if 1
			__madvise((void *)a, b-a, MADV_DONTNEED);
#else
			__mmap((void *)a, b-a, PROT_READ|PROT_WRITE,
				MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
#endif
		}

		if (self->data[-1] & next->data[0] & C_INUSE) {
			self->data[0] = final_size | C_INUSE;
			next->data[-1] = final_size | C_INUSE;
			i = bin_index(final_size);
			lock_bin(i);
			lock(mal.free_lock);
			if (self->data[-1] & next->data[0] & C_INUSE)
				break;
			unlock(mal.free_lock);
			unlock_bin(i);
		}

		if (alloc_rev(self)) {
			self = PREV_CHUNK(self);
			size = CHUNK_SIZE(self);
			final_size += size;
			if (new_size+size > RECLAIM && (new_size+size^size) > size)
				reclaim = 1;
		}

		if (alloc_fwd(next)) {
			size = CHUNK_SIZE(next);
			final_size += size;
			if (new_size+size > RECLAIM && (new_size+size^size) > size)
				reclaim = 1;
			next = NEXT_CHUNK(next);
		}
	}

	self->data[0] = final_size;
	next->data[-1] = final_size;
	unlock(mal.free_lock);

	self->next = BIN_TO_CHUNK(i);
	self->prev = mal.bins[i].tail;
	self->next->prev = self;
	self->prev->next = self;

	if (!(mal.binmap & 1ULL<<i))
		a_or_64(&mal.binmap, 1ULL<<i);

	unlock_bin(i);
}
