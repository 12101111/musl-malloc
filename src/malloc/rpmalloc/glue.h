#include "rpmalloc.h"
#if (defined(__GNUC__) && __GNUC__ >= 9)
  #pragma GCC diagnostic ignored "-Wattributes"  // or we get warnings that nodiscard is ignored on a forward
  #define hidden_alias(fun)   __attribute__((alias(#fun), used, visibility("hidden"), copy(fun)));
  #define public_alias(fun)   __attribute__((alias(#fun), used, visibility("default"), copy(fun)));
#else
  #define hidden_alias(fun)   __attribute__((alias(#fun), used, visibility("hidden")));
  #define public_alias(fun)   __attribute__((alias(#fun), used, visibility("default"), copy(fun)));
#endif

void* __libc_malloc_impl(size_t)      hidden_alias(rpmalloc)
void* __libc_realloc(void*, size_t)   hidden_alias(rprealloc)
void  __libc_free(void*)              hidden_alias(rpfree)
void *aligned_alloc(size_t align, size_t size)  public_alias(rpaligned_alloc)
size_t malloc_usable_size(void * p)             public_alias(rpmalloc_usable_size)

hidden void __malloc_atfork(int who) {}
hidden void __malloc_donate(char *start, char *end) {}
hidden int __malloc_process_init() {
	rpmalloc_set_main_thread();
	return rpmalloc_initialize();
}
hidden void __malloc_thread_init() {
  rpmalloc_thread_initialize();
}
hidden void __malloc_thread_finalize() {
  rpmalloc_thread_finalize(1);
}

#include "pthread_impl.h"

static inline heap_t* get_thread_heap_raw(void) {
  pthread_t self = __pthread_self();
  return (heap_t *) self->heap;
}

static inline uintptr_t get_thread_id(void) {
  pthread_t self = __pthread_self();
  return (uintptr_t) self;
}

static void set_thread_heap(heap_t* heap) {
  pthread_t self = __pthread_self();
  self->heap = (void *)heap;
  if (heap) heap->owner_thread = get_thread_id();
}
