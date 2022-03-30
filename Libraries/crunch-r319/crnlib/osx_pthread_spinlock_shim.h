#pragma once

#include <libkern/OSAtomic.h>

typedef OSSpinLock pthread_spinlock_t;

static inline int pthread_spin_init(pthread_spinlock_t *lock, int pshared) {
	OSSpinLockUnlock(lock);
	return 0;
}

static inline int pthread_spin_destroy(pthread_spinlock_t *lock) {
	return 0;
}

static inline int pthread_spin_lock(pthread_spinlock_t *lock) {
	OSSpinLockLock(lock);
	return 0;
}

static inline int pthread_spin_trylock(pthread_spinlock_t *lock) {
	if (OSSpinLockTry(lock)) {
		return 0;
	}
	return EBUSY;
}

static inline int pthread_spin_unlock(pthread_spinlock_t *lock) {
	OSSpinLockUnlock(lock);
	return 0;
}