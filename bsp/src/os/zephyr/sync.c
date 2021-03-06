/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ZEPHYR OS abstraction / synchronization services
 * (semaphores, mutexes, critical sections...).
 */

#include <zephyr.h>
#include "infra/panic.h"

#include "common.h"

#ifdef   CONFIG_MICROKERNEL
#include <mutex.h>
#include <microkernel/command_packet.h>
#endif

/*-------- Settings for the SYNC. OBJECTS */
#define SEMAPHORE_POOL_SIZE          32 /** Total number of semaphores */
#define MUTEX_POOL_SIZE              32 /** Total number of mutexes */


/** Macros for reading, setting and clearing semaphore and mutexes usage trackers */
#define IS_OBJ_RESERVED(pool, idx)         (((pool) & (1 << (idx))) != 0)
#define RESERVE_OBJ(pool, idx)             (pool) |= (1 << (idx))
#define UNRESERVE_OBJ(pool, idx)           (pool) &= ~(1 << (idx))

#define IS_SEMAPHORE_RESERVED(idx)         IS_OBJ_RESERVED(g_SemaphoresInUse, \
							   (idx))
#define RESERVE_SEMAPHORE(idx)             RESERVE_OBJ(g_SemaphoresInUse, (idx))
#define UNRESERVE_SEMAPHORE(idx)           UNRESERVE_OBJ(g_SemaphoresInUse, \
							 (idx))

#define IS_MUTEX_RESERVED(idx)             IS_OBJ_RESERVED(g_MutexesInUse, (idx))
#define RESERVE_MUTEX(idx)                 RESERVE_OBJ(g_MutexesInUse, (idx))
#define UNRESERVE_MUTEX(idx)               UNRESERVE_OBJ(g_MutexesInUse, (idx))


#ifdef CONFIG_NANOKERNEL
#define GET_SEMAPHORE_INDEX(sem)     ((((uint32_t)(sem)) - \
				       ((uint32_t)&(g_SemaphorePool[0]))) / \
				      sizeof(struct nano_sem))
#define GET_MUTEX_INDEX(sem)         ((((uint32_t)(sem)) - \
				       ((uint32_t)&(g_MutexPool[0]))) /	\
				      sizeof(struct nano_sem))
#else
#define GET_SEMAPHORE_INDEX(sem)     ((((uint32_t)(sem)) - \
				       ((uint32_t)&(g_SemaphorePool[0]))) / \
				      sizeof(struct _k_sem_struct))
#define GET_MUTEX_INDEX(sem)         ((((uint32_t)(sem)) - \
				       ((uint32_t)&(g_MutexPool[0]))) /	\
				      sizeof(struct _k_mutex_struct))
#endif


#ifdef CONFIG_NANOKERNEL
static struct nano_sem _PoolLockSem;
#else /* CONFIG_MICROKERNEL */
static ksem_t _PoolLockSem;
#endif

static uint32_t g_SemaphoresInUse;    /* bitmask for tracking which semaphores are being used */
static uint32_t g_MutexesInUse;       /* bitmask for tracking which mutexes are being used */


#ifdef CONFIG_NANOKERNEL
/* allocate the pool of semaphores and mutexes */
static struct nano_sem g_SemaphorePool [SEMAPHORE_POOL_SIZE];
static struct nano_sem g_MutexPool     [MUTEX_POOL_SIZE];
#else
/* CONFIG_MICROKERNEL:
 * no need to allocate the pools of semaphores and mutexes: this is done
 * by variables _k_sem_list and _k_mutex_list from NODE1.c
 */
static struct _k_sem_struct g_SemaphorePool[SEMAPHORE_POOL_SIZE];
static struct _k_mutex_struct g_MutexPool[MUTEX_POOL_SIZE];

#endif

#ifdef CONFIG_MICROKERNEL
/**
 * Converts Zephyr's semaphore API return value to OS_ERR_TYPE error code.
 *
 * @param [in] zephyrErr : error code returned by the microkernel's semaphore functions
 *
 * @return framework standardized error code (::OS_ERR_TYPE)
 */
static OS_ERR_TYPE _ZephyrErrToOsErr(int zephyrErr)
{
	OS_ERR_TYPE osErr = E_OS_ERR_UNKNOWN;

#ifdef CONFIG_NANOKERNEL
	switch (zephyrErr) {
	case 0:
		osErr = E_OS_ERR_BUSY;
		break;

	case 1:
		osErr = E_OS_OK;
		break;

	default:
		break;
	}

#else

	switch (zephyrErr) {
	case RC_OK:
		osErr = E_OS_OK;
		break;

	case RC_FAIL:
		osErr = E_OS_ERR_BUSY;
		break;

	case RC_TIME:
		osErr = E_OS_ERR_TIMEOUT;
		break;

	default:
		break;
	}
#endif
	return osErr;
}
#endif

/**
 * Locks access to a pool of synchronization objects.
 *
 * This function manages the concurrent accesses to the pools
 * of semaphores and mutexes.
 *
 * This function is only used when creating and deleting semaphores or mutexes.
 *
 * Rationale:
 *
 *  This function does not manage concurrent accesses to the pools when running
 *  from an ISR context. ( Creating a semaphore or a mutex in an ISR does not
 *  seem a valid use-case anyway ).
 */
void _PoolLock(void)
{
	T_EXEC_LEVEL execLvl;

	execLvl = _getExecLevel();

	/* use the private semaphore to manage access to the pool of sync objects */
	switch (execLvl) {
	case E_EXEC_LVL_FIBER:
#ifdef CONFIG_NANOKERNEL
		nano_fiber_sem_take(&_PoolLockSem, TICKS_UNLIMITED);
#else
		/* TODO:  where is FIBER_SemaTestW ? */
		(void)task_sem_take(_PoolLockSem, TICKS_UNLIMITED);
#endif
		break;

	case E_EXEC_LVL_TASK:
#ifdef CONFIG_NANOKERNEL
		nano_task_sem_take(&_PoolLockSem, TICKS_UNLIMITED);
#else
		(void)task_sem_take(_PoolLockSem, TICKS_UNLIMITED);
#endif
		break;

	default:
		/* do nothing */
		break;
	}
}

/**
 * Unlocks access to a pool of synchronization objects.
 *
 * This function manages the concurrent accesses to the pools
 * of semaphores and mutexes.
 *
 * This function is only used when creating and deleting semaphores or mutexes.
 *
 * Rationale:
 *
 *  This function does not manage concurrent accesses to the pools when running
 *  from an ISR context. ( Creating a semaphore or a mutex in an ISR does not
 *  seem a valid use-case anyway ).
 *
 */
void _PoolUnlock(void)
{
	T_EXEC_LEVEL execLvl;

	execLvl = _getExecLevel();

	/* free up the private semaphore  */
	switch (execLvl) {
	case E_EXEC_LVL_FIBER:
#ifdef CONFIG_NANOKERNEL
		nano_fiber_sem_give(&_PoolLockSem);
#else
		fiber_sem_give(_PoolLockSem);
#endif
		break;

	case E_EXEC_LVL_TASK:
#ifdef CONFIG_NANOKERNEL
		nano_task_sem_give(&_PoolLockSem);
#else
		task_sem_give(_PoolLockSem);
#endif
		break;

	default:
		/* do nothing */
		break;
	}
}


/**********************************************************
************** Forward declarations **********************/
static bool _IsSemaphoreValid(T_SEMAPHORE sem);
static bool _IsMutexValid(T_MUTEX mut);



/**********************************************************
************** Private functions  ************************
**********************************************************/

/**
 * Checks if a semaphore pointer is valid, and if the semaphore has been created.
 *
 * @param [in] sem  semaphore object
 *
 * @return bool:
 *         - true -> the semaphore is valid
 *         - false -> otherwise
 */
static bool _IsSemaphoreValid(T_SEMAPHORE sem)
{
	bool valid = false;
	uint8_t idx;

	if (sem != NULL) {
		idx = GET_SEMAPHORE_INDEX(sem);
		if (idx < SEMAPHORE_POOL_SIZE) {
			valid = IS_SEMAPHORE_RESERVED(idx);
		}
	}
	return valid;
}


/**
 * Checks if a mutex pointer is valid, and if the mutex has been created.
 *
 * @param [in] mut  mutex object
 *
 * @return bool:
 *         - true -> the semaphore is valid
 *         - false -> otherwise
 */
static bool _IsMutexValid(T_MUTEX mut)
{
	bool valid = false;
	uint8_t idx;

	if (mut != NULL) {
		idx = GET_MUTEX_INDEX(mut);
		if (idx < MUTEX_POOL_SIZE) {
			valid = IS_MUTEX_RESERVED(idx);
		}
	}
	return valid;
}



#ifdef   CONFIG_NANOKERNEL

/**
 * Structure passed as an argument to the timer callback for managing the timeout while waiting for a semaphore.
 */
typedef struct {
	struct nano_sem *semaphore; /* pointer on (valid) NanoK semaphore object */
	bool timedout;     /* flag to indicate if the timed wait expired or not */
}T_SEM_WAIT_DATA;

/**
 * Timer callback to implement a timeout while waiting for a semaphore.
 *
 * This function marks that the timeout has expired, then signals the
 * semaphore.
 *
 * @param cbkData structure that contains the semaphore to signal and
 *                a timeout flag
 */
static void _SemaphoreTimeoutCallback(void *cbkData)
{
	T_SEM_WAIT_DATA *data = (T_SEM_WAIT_DATA *)cbkData;

	data->timedout = true;
	nano_fiber_sem_give(data->semaphore);
	/* rem: for nanoK, timer callbacks are always executed in fiber context */
}


/**
 * Timed wait for a semaphore in NanoK.
 *
 * This function waits for a semaphore to be signaled, and uses a timer
 * to limit the waiting to the specified delay.
 *
 * REM: this STATIC function does not check the validity of its arguments.
 *
 * @param semaphore pointer on a NanoK semaphore object
 * @param timeout maximum delay to wait for the semaphore (in ms), or OS_WAIT_FOREVER
 * @param execLvl current execution level (fiber or task)
 *
 * @return std error type :
 *     - E_OS_OK: semaphore was signaled before timeout expiration
 *     - E_OS_ERR_TIMEOUT: timeout expired before the semaphore was signaled
 *     - E_OS_ERR: no timer was available
 *
 */
static OS_ERR_TYPE _WaitForSemaphore(struct nano_sem *	semaphore,
				     uint32_t		timeout,
				     T_EXEC_LEVEL	execLvl)
{
	OS_ERR_TYPE err;
	T_TIMER tmr = NULL;
	T_SEM_WAIT_DATA semWaitData;

	if (OS_WAIT_FOREVER != timeout) {
		/* init timer callback parameter */
		semWaitData.semaphore = semaphore;
		semWaitData.timedout = false;

		/* arm a timer to signal the semaphore when it expires */
		tmr =
			timer_create(_SemaphoreTimeoutCallback,
				     (void *)&semWaitData,
				     timeout, false, true,
				     &err);
	}


	if ((NULL != tmr) || (OS_WAIT_FOREVER == timeout)) {
		/* wait indefinitely for the semaphore to be signaled */
		if (E_EXEC_LVL_FIBER == execLvl) {
			nano_fiber_sem_take((struct nano_sem *)semaphore,
					    TICKS_UNLIMITED);
		} else { /* execLvl can only be "task" at this point */
			nano_task_sem_take((struct nano_sem *)semaphore,
					   TICKS_UNLIMITED);
		}

		if (NULL != tmr) {
			timer_delete(tmr);
			/* check if the semaphore was signaled by the timer */
			if (true == semWaitData.timedout) {
				err = E_OS_ERR_TIMEOUT;
			} else {
				err = E_OS_OK;
			}
		} else { /* null timer here means that no timeout was specified */
			err = E_OS_OK;
		}
	} else {
		/* timer_set_callback returned NOK because there is no available timer */
		err = E_OS_ERR;
	}

	return err;
}
#endif


/**********************************************************
************** Exported functions ************************
**********************************************************/

/*----- Initialization  */


/**
 * Initializes the resources used by the framework's sync services.
 *
 * @attention  This function must be called during the initialization
 *             of the OS abstraction layer.
 * @attention  This function shall only be called once after reset, otherwise
 *             it may cause the take/lock and give/unlock services to fail
 */
void os_init_sync(void)
{
	g_SemaphoresInUse = 0; /* all semaphores unused */
	g_MutexesInUse = 0; /* all mutexes unused */

#ifdef   CONFIG_NANOKERNEL
	nano_sem_init(&_PoolLockSem);
	nano_task_sem_give(&_PoolLockSem);
#else
	_PoolLockSem = FWK_SYNC_SEM; /* FWK_SYNC_SEM is defined by allnodes.h */
#endif

	_PoolUnlock();
}

/*----- Critical sections  */

static uint32_t g_ItLockKey;  /*<! lock key for disabling/restoring all interrupts */
static int16_t g_disableSchedNestCount = 0;  /*<! Used to count each time the function disable_scheduling is called  */

/**
 * Disables scheduling.
 *
 * Disables scheduling and task preemption.
 *
 * This service has no effect when called from an ISR context.
 *
 * Authorized execution levels:  task, fiber.
 *
 * ZEPHYR SPECIFIC:
 *
 * This service disables all interrupts.
 */
void disable_scheduling(void)
{
	/* Lock scheduling */
	uint32_t key = irq_lock();

	/* Check counter: if 0 store context */
	if (0 == g_disableSchedNestCount)
		g_ItLockKey = key;

	/* increase function counter */
	g_disableSchedNestCount++;
}


/**
 * Enables scheduling.
 *
 * Restores scheduling and task preemption.
 * This service has no effect when called from an ISR context.
 *
 * Authorized execution levels:  task, fiber.
 *
 * ZEPHYR SPECIFIC:
 *
 * This service unmasks all interrupts that were masked by a previous
 * call to disable_scheduling.
 */
void enable_scheduling(void)
{
	/** decrease function counter */
	g_disableSchedNestCount--;
	/** Check counter: if 0 restore context */
	if (0 == g_disableSchedNestCount)
		irq_unlock(g_ItLockKey);
}





/*----- Semaphores  */


/**
 * Creates a semaphore.
 *
 * Creates or reserves a semaphore object. The service may fail if all allocated
 * semaphores are already being used.
 *
 * @warning This service may panic if:
 *          - no semaphore is available, or
 *          - when called from an ISR.
 *
 * Semaphores are not dynamically allocated and destroyed. They are picked from a
 * (limited) static pool that is defined at configuration time. Concurrent
 * accesses to this pool are serialized by a framework-specific semaphore.
 *
 * The total number of semaphores that may be in use (created) at the same time is 32.
 * Use semaphore_delete, then semaphore_create to reset a semaphore (TBC).
 *
 * Authorized execution levels:  task, fiber.
 *
 * @param initialCount: initial count of the semaphore.
 *
 * @return Handler on the created semaphore.
 *     NULL if all allocated semaphores are already being used
 */
T_SEMAPHORE semaphore_create(uint32_t initialCount)
{
	T_SEMAPHORE semaphore = NULL;
	uint32_t idx = 0;
	T_EXEC_LEVEL execLvl;

	/* check execution level */
	execLvl = _getExecLevel();
	if ((E_EXEC_LVL_FIBER == execLvl) || (E_EXEC_LVL_TASK == execLvl)) {
		/* Block concurrent accesses to g_SemaphorePool */
		_PoolLock();
		/* look for an available semaphore in the pool */
		do {
			if (!IS_SEMAPHORE_RESERVED(idx)) {
				semaphore =
					(T_SEMAPHORE)(&(g_SemaphorePool[idx]));
				RESERVE_SEMAPHORE(idx);
			}
			idx++;
		} while ((semaphore == NULL) && (idx < SEMAPHORE_POOL_SIZE));

		if (semaphore != NULL) {
			/* initialize the fields of the semaphore object */
#ifdef   CONFIG_NANOKERNEL
			idx--; /* use idx as the semaphore's index in the pool */
			nano_sem_init(&g_SemaphorePool[idx]);
			((struct nano_sem *)semaphore)->nsig = initialCount;
#else      /*  -> MICRO KERNEL_ABSTRACTION */
			((struct _k_sem_struct *)semaphore)->waiters = NULL;
			((struct _k_sem_struct *)semaphore)->level =
				initialCount;
			((struct _k_sem_struct *)semaphore)->count = 0;
#endif
		} else {
			/* all semaphores from the pool are already being used */
			panic(E_OS_ERR);
		}
		/* release the semaphore pool */
		_PoolUnlock();
	} else { /* service may not be called from ISR context */
		panic(E_OS_ERR_NOT_ALLOWED);
	}
	return semaphore;
}





/**
 * Deletes a semaphore.
 *
 * Disables a semaphore that was reserved by semaphore_create. Deleting a
 * semaphore while a task is waiting (or will wait) for it to be signaled
 * may create a deadlock.
 *
 * @warning This service may panic if:
 *          - semaphore parameter is invalid, or
 *          - when called from an ISR.
 *
 * Authorized execution levels:  task, fiber.
 *
 * See also \ref semaphore_create
 *
 * @param semaphore: handler on the semaphore to delete (returned by semaphore_create).
 *
 */
void semaphore_delete(T_SEMAPHORE semaphore)
{
	uint8_t idx;
	T_EXEC_LEVEL execLvl;

	/* check execution level */
	execLvl = _getExecLevel();
	if ((E_EXEC_LVL_FIBER == execLvl) || (E_EXEC_LVL_TASK == execLvl)) {
		if (_IsSemaphoreValid(semaphore)) {
			/* Block concurrent accesses to g_SemaphorePool */
			_PoolLock();

			idx = GET_SEMAPHORE_INDEX(semaphore);
			if (idx < SEMAPHORE_POOL_SIZE) {
				UNRESERVE_SEMAPHORE(idx);
			}
			/* release the semaphore pool */
			_PoolUnlock();
		} else { /* argument is invalid */
			panic(E_OS_ERR);
		}
	} else { /* service may not be called from ISR context */
		panic(E_OS_ERR_NOT_ALLOWED);
	}
}


/**
 * Gives/signals a semaphore.
 *
 * @warning This service may panic if err parameter is null and:
 *          - semaphore parameter is invalid.
 *
 * Authorized execution levels:  task, fiber, ISR.
 *
 * @param semaphore: handler on the semaphore to delete (returned by semaphore_create).
 *
 * @param err (out): execution status:
 *     - E_OS_OK semaphore was freed/signaled
 *     - E_OS_ERR: semaphore parameter is invalid, or was not created
 */
void semaphore_give(T_SEMAPHORE semaphore, OS_ERR_TYPE *err)
{
	T_EXEC_LEVEL execLvl;

	/* check input parameters */
	if (_IsSemaphoreValid(semaphore)) {
		/* get the current execution level: ISR, Fiber or Task */
		execLvl = _getExecLevel();
#ifdef   CONFIG_NANOKERNEL
		/* call the nanoK service that corresponds to the current execution level */
		switch (execLvl) {
		case E_EXEC_LVL_ISR:
			nano_isr_sem_give((struct nano_sem *)semaphore);
			break;
		case E_EXEC_LVL_FIBER:
			nano_fiber_sem_give((struct nano_sem *)semaphore);
			break;
		case E_EXEC_LVL_TASK:
			nano_task_sem_give((struct nano_sem *)semaphore);
			break;
		default: /* the value returned by _getExecLevel is out of bounds */
			error_management(err, E_OS_ERR_UNKNOWN);
			break;
		}
#else /* -> CONFIG_MICROKERNEL */
		if (E_EXEC_LVL_ISR == execLvl) {
			isr_sem_give((ksem_t)semaphore);
		} else {
			task_sem_give((ksem_t)semaphore);
		}
#endif
		error_management(err, E_OS_OK);
	} else { /*semaphore pointer is invalid */
		error_management(err, E_OS_ERR);
	}
}



/**
 * Takes/blocks a semaphore.
 *
 * - This service may block while waiting on the semaphore,
 * depending on the timeout parameter.
 * - This service shall not be called from an ISR context.
 *
 * Authorized execution levels:  task, fiber.
 *
 * @param semaphore: handler on the semaphore to delete (value returned by semaphore_create).
 *
 * @param timeout: maximum number of milliseconds to wait for the semaphore. Special values
 *             OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 *
 * @return execution status:
 *      - E_OS_OK: semaphore was successfully taken
 *      - E_OS_ERR / INVALID_@param semaphore parameter is invalid (semaphore was deleted or never created)
 *      - E_OS_ERR_TIMEOUT: could not take semaphore before timeout expiration
 *      - E_OS_ERR_BUSY: could not take semaphore (did not wait)
 *      - E_OS_ERR_NOT_ALLOWED: service cannot be executed from ISR context.
 */
OS_ERR_TYPE  semaphore_take(T_SEMAPHORE semaphore, int timeout)
{
	OS_ERR_TYPE err = E_OS_ERR;
	T_EXEC_LEVEL execLvl;
	int zephyrErr;

	/* check input parameters */
	if (_IsSemaphoreValid(semaphore)) {
		/* get the current execution level: ISR, Fiber or Task */
		execLvl = _getExecLevel();
		if (E_EXEC_LVL_ISR != execLvl) {
			/* If the caller does not want to wait for the semaphore, or if the function is called from an ISR */
			if (OS_NO_WAIT == timeout) {
#ifdef   CONFIG_NANOKERNEL
				zephyrErr =
					nano_isr_sem_take(
						(struct nano_sem *)semaphore,
						TICKS_NONE);                               /* a.k.a  nano_fiber_sem_take, nano_task_sem_take, _SemTake */
				if (1 == zephyrErr)
					err = E_OS_OK;
				else
					err = E_OS_ERR_BUSY;
#else /* -> CONFIG_MICROKERNEL */
				zephyrErr = task_sem_take((ksem_t)semaphore,
							  TICKS_NONE);
				err = _ZephyrErrToOsErr(zephyrErr);
#endif
			} else {
#ifdef   CONFIG_NANOKERNEL
				/* Wait for the semaphore */
				err = _WaitForSemaphore(
					(struct nano_sem *)semaphore, timeout,
					execLvl);

#else /* -> CONFIG_MICROKERNEL */
				/* If the caller does not specify a timeout */
				if (OS_WAIT_FOREVER == timeout) {
					zephyrErr = task_sem_take(
						(ksem_t)semaphore,
						TICKS_UNLIMITED);                   /* rem: waits forever */
					err = _ZephyrErrToOsErr(zephyrErr);
				} else {
					zephyrErr = task_sem_take(
						(ksem_t)semaphore,
						CONVERT_MS_TO_TICKS(timeout));
					err = _ZephyrErrToOsErr(zephyrErr);
				}
				/* REM: microkernel API does not provide FIBER_SemaTest[W/WT] functions. The uk only has one fiber which is dedicated to the K_swapper.
				 * User applications/drivers may not create fibers.
				 */
#endif
			} /* end else (OS_NO_WAIT == timeout) */
		} else { /* this service may not be called from an ISR */
			err = E_OS_ERR_NOT_ALLOWED;
		}
	}
	/* else: semaphore pointer is invalid, return generic error code */
	return err;
}






/**
 * Gets the semaphore count.
 *
 * Returns the count of a semaphore.
 *
 * @warning This service may panic if err parameter is null and:
 *          - semaphore parameter is invalid
 *
 * Authorized execution levels:  task, fiber, ISR.
 *
 * @param semaphore: handler on the semaphore to delete (returned by semaphore_create).
 *
 * @param err (out): execution status:
 *         - E_OS_OK : returned count value is correct
 *         - E_OS_ERR: semaphore parameter is invalid, or was not created
 *
 * @return
 *         - negative value: number of clients waiting on the semaphore,
 *         - positive value: number of times the semaphore has been signaled.
 *
 */
int32_t semaphore_get_count(T_SEMAPHORE semaphore, OS_ERR_TYPE *err)
{
	int count = 0;

	if (_IsSemaphoreValid(semaphore)) {
#ifdef   CONFIG_NANOKERNEL
		count = ((struct nano_sem *)semaphore)->nsig;
#else /* -> CONFIG_MICROKERNEL */
		count = ((struct _k_sem_struct *)semaphore)->level; // TODO FC, really, ->level and not ->count ??
#endif
		error_management(err, E_OS_OK);
	} else { /*semaphore pointer is invalid */
		error_management(err, E_OS_ERR);
	}

	return count;
}


/**
 * Creates a mutex.
 *
 * Creates or reserves a mutex object. The service may fail if all allocated
 * mutexes are already being used.
 *
 * @warning This service may panic if:
 *          - no mutex is available, or
 *          - when called from an ISR.
 *
 * Authorized execution levels:  task, fiber.
 *
 * Mutexes are not dynamically allocated and destroyed. They are picked from a
 * (limited) static pool that is defined at configuration time. Concurrent accesses
 * to this pool are serialized by a framework-specific semaphore.
 *
 * The total number of mutexes that may be in use (created) at the same time is 32.
 * Mutexes shall not be recursive: consecutive calls to mutex_lock shall fail.
 * - ZEPHYR  K: mutexes shall be implemented as Zephyr resources.
 * - ZEPHYR nK: mutexes shall be implemented as Zephyr semaphores.
 *
 * @return Handler on the created mutex.
 *     NULL if all allocated mutexes are already being used.
 *
 */
T_MUTEX mutex_create(void)
{
	T_MUTEX mutex = NULL;
	uint32_t idx = 0;
	T_EXEC_LEVEL execLvl;

	/* check execution level */
	execLvl = _getExecLevel();
	if ((E_EXEC_LVL_FIBER == execLvl) || (E_EXEC_LVL_TASK == execLvl)) {
		/* Block concurrent accesses to g_MutexPool */
		_PoolLock();
		/* look for an available mutex in the pool */
		do {
			if (!IS_MUTEX_RESERVED(idx)) {
				mutex = (T_MUTEX)(&(g_MutexPool[idx]));
				RESERVE_MUTEX(idx);
			}
			idx++;
		} while ((mutex == NULL) && (idx < MUTEX_POOL_SIZE));

		if (mutex != NULL) {
			/* initialize the fields of the mutex object */
#ifdef   CONFIG_NANOKERNEL
			idx--; /* use idx as the semaphore's index in the pool */
			nano_sem_init(&g_MutexPool[idx]);
			((struct nano_sem *)mutex)->nsig = 1; /* binary semaphore TODO: check this */

#else  /*  -> MICRO KERNEL_ABSTRACTION */
			((struct _k_mutex_struct *)mutex)->owner = ANYTASK;
			((struct _k_mutex_struct *)mutex)->
			current_owner_priority = 64;
			((struct _k_mutex_struct *)mutex)->
			original_owner_priority = 64;
			((struct _k_mutex_struct *)mutex)->level = 0;
			((struct _k_mutex_struct *)mutex)->waiters = NULL;
			((struct _k_mutex_struct *)mutex)->count = 0;
			((struct _k_mutex_struct *)mutex)->num_conflicts = 0;
#endif
		} else { /* all mutexes from the pool are already being used */
			panic(E_OS_ERR);
		}
		/* release the mutex pool */
		_PoolUnlock();
	} else { /* service may not be called from an ISR */
		panic(E_OS_ERR_NOT_ALLOWED);
	}

	return mutex;
}




/**
 * Deletes a mutex.
 *
 * Disables a mutex that was reserved using mutex_create. Deleting a mutex while
 * a task is waiting (or will wait) for it to be freed may create a deadlock.
 *
 * @warning This service may panic if:
 *          - mutex parameter is invalid, or
 *          - when called from an ISR.
 *
 * Authorized execution levels: task, fiber.
 * See also \ref mutex_create
 *
 * @param mutex: handler on the mutex to delete (value returned by mutex_create).
 *
 */
void mutex_delete(T_MUTEX mutex)
{
	uint8_t idx;
	T_EXEC_LEVEL execLvl;

	/* check execution level */
	execLvl = _getExecLevel();
	if ((E_EXEC_LVL_FIBER == execLvl) || (E_EXEC_LVL_TASK == execLvl)) {
		if (_IsMutexValid(mutex)) {
			/* Block concurrent accesses to g_MutexPool  */
			_PoolLock();

			idx = GET_MUTEX_INDEX(mutex);
			UNRESERVE_MUTEX(idx);

			/* release the mutex pool */
			_PoolUnlock();
		} else { /* argument is invalid */
			panic(E_OS_ERR);
		}
	} else { /* service may not be called from an ISR */
		panic(E_OS_ERR_NOT_ALLOWED);
	}
}




/**
 * Unlocks/gives a mutex.
 *
 * @warning This service may panic if:
 *          - mutex parameter is invalid, or
 *          - when called from an ISR.
 *
 * Authorized execution levels:  task, fiber.
 *
 * ZEPHYR SPECIFIC:
 *
 * Unlocking a free mutex will not panic (because ZEPHYR primitives do not
 *  return an execution status ).
 *
 * @param mutex: handler on the mutex to unlock (value returned by mutex_create),
 *
 */
void mutex_unlock(T_MUTEX mutex)
{
	T_EXEC_LEVEL execLvl;

#ifdef CONFIG_NANOKERNEL
	struct nano_sem *resPtr;
#else
	struct _k_mutex_struct *resPtr;
#endif

	/* check execution level */
	execLvl = _getExecLevel();
	if ((E_EXEC_LVL_FIBER == execLvl) || (E_EXEC_LVL_TASK == execLvl)) {
		/* check input parameters */
		if (_IsMutexValid(mutex)) {
#ifdef CONFIG_NANOKERNEL

			resPtr = (struct nano_sem *)mutex;
			if (resPtr->nsig > 0) {
				/* mutex is already unlocked */
				panic(E_OS_ERR);
			} else {
				/* call the nanoK service that corresponds to the current execution level */
				switch (execLvl) {
				case E_EXEC_LVL_ISR:
					nano_isr_sem_give(resPtr);
					break;
				case E_EXEC_LVL_FIBER:
					nano_fiber_sem_give(resPtr);
					break;
				case E_EXEC_LVL_TASK:
					nano_task_sem_give(resPtr);
					break;
				default: /* execLvl is out of bounds */
					panic(E_OS_ERR_UNKNOWN);
					break;
				}
			}
#else

			/* get a pointer on the Zephyr resource object */
			resPtr = (struct _k_mutex_struct *)mutex;

			if (resPtr->level == 0) { /* mutex is not locked */
				panic(E_OS_ERR);
			} else {
				task_mutex_unlock((kmutex_t)mutex); /* TODO: check if that works from a FIBER context */
			}
#endif
		} else { /* mut is a invalid */
			panic(E_OS_ERR);
		}
	} else { /* service may not be called from an ISR */
		panic(E_OS_ERR_NOT_ALLOWED);
	}
}






/**
 * Locks/takes a mutex.
 *
 * This service may block while waiting on the mutex's
 * availability, depending on the timeout parameter.
 *
 * Authorized execution levels:  task, fiber.
 *
 * @param mutex: handler on the mutex to lock (value returned by mutex_create).
 *
 * @param timeout: maximum number of milliseconds to wait for the mutex. Special
 *            values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 * @return execution status:
 *      - E_OS_OK: mutex was successfully taken
 *      - E_OS_ERR: mut parameter is invalid (mutex was deleted or never created)
 *      - E_OS_ERR_TIMEOUT: could not take semaphore before timeout expiration
 *      - E_OS_ERR_BUSY: could not take semaphore (did not wait)
 *      - E_OS_ERR_NOT_ALLOWED: service cannot be executed from ISR context.
 */
OS_ERR_TYPE mutex_lock(T_MUTEX mutex, int timeout)
{
	OS_ERR_TYPE err = E_OS_ERR;
	int zephyrErr;
	T_EXEC_LEVEL execLvl;

	/* check execution level */
	execLvl = _getExecLevel();
	if (E_EXEC_LVL_ISR != execLvl) {
		/* check input parameters */
		if (_IsMutexValid(mutex)) {
#ifdef CONFIG_NANOKERNEL
			/* If the caller does not want to wait for the semaphore */
			if (OS_NO_WAIT == timeout) {
				zephyrErr =
					nano_isr_sem_take(
						(struct nano_sem *)mutex,
						TICKS_NONE);                           /* a.k.a  nano_fiber_sem_take, nano_task_sem_take, _SemTake */
				if (1 == zephyrErr)
					err = E_OS_OK;
				else
					err = E_OS_ERR_BUSY;
			} else {
				err = _WaitForSemaphore(
					(struct nano_sem *)mutex, timeout,
					execLvl);
			}
#else
			if (OS_NO_WAIT == timeout) {
				timeout = TICKS_NONE;
			}
			if (OS_WAIT_FOREVER == timeout) {
				timeout = TICKS_UNLIMITED;
			}

			zephyrErr = task_mutex_lock((kmutex_t)mutex, timeout); /* TODO: check if that works from a FIBER context */
			err = _ZephyrErrToOsErr(zephyrErr);
#endif
		}
		/* else: mut is invalid, return E_OS_ERR */
	} else { /* service may not be called from an ISR */
		err = E_OS_ERR_NOT_ALLOWED;
	}

	return err;
}


/**
 * Create Mutex for Read lock and Write lock
 */

#ifndef CONFIG_OS_NONE
/**
 * Creates a Read/Write lock.
 *
 * Creates or reserves a rwlock object.
 *
 * @warning This service may panic if:
 *          - no mutex is available, or
 *          - when called from an ISR.
 *
 * Authorized execution levels:  task, fiber.
 *
 * @param rwlock: handler on the read/write lock,
 *
 */
void rwlock_init(struct rwlock_t *rwlock)
{
	rwlock->rwlock_rdmtx = mutex_create();
	rwlock->rwlock_wrmtx = mutex_create();
	rwlock->read_count = 0;
}

/**
 * Deletes a Read/Write lock.
 *
 * Disables a Read/Write lock mutexes that was reserved using rwlock_init. Deleting a rwlock while
 * a task is waiting (or will wait) for it to be freed may create a deadlock.
 *
 * @warning This service may panic if:
 *          - rwlock parameter is invalid, or
 *          - when called from an ISR.
 *
 * Authorized execution levels: task, fiber.
 * See also \ref rwlock_init
 *
 * @param rwlock: handler on the read/write lock to unlock read lock,
 *
 */
void rwlock_delete(struct rwlock_t *rwlock)
{
	mutex_delete(rwlock->rwlock_rdmtx);
	mutex_delete(rwlock->rwlock_wrmtx);
	rwlock->read_count = 0;
}

/**
 * Locks/takes a read mutex.
 *
 * This service may block while waiting on the mutex's
 * availability, depending on the timeout parameter.
 *
 * Authorized execution levels:  task, fiber.
 *
 * @param rwlock: handler on the read/write lock to lock read lock,
 *
 * @param timeout: maximum number of milliseconds to wait for the mutex. Special
 *            values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 */
void rwlock_rdlock(struct rwlock_t *rwlock, int32_t timeout)
{
	if (mutex_lock(rwlock->rwlock_rdmtx, timeout) != E_OS_OK) {
		return;
	}

	rwlock->read_count++;

	if (rwlock->read_count == 1) {
		if (mutex_lock(rwlock->rwlock_wrmtx, timeout) != E_OS_OK) {
			return;
		}
	}

	mutex_unlock(rwlock->rwlock_rdmtx);
}


/**
 * Unlocks/gives read mutex.
 *
 * This service may block while waiting on the mutex's
 * availability, depending on the timeout parameter.
 *
 * Authorized execution levels:  task, fiber.
 *
 * @param rwlock: handler on the read/write lock to unlock read lock,
 *
 */
void rwlock_rdunlock(struct rwlock_t *rwlock)
{
	mutex_lock(rwlock->rwlock_rdmtx, OS_WAIT_FOREVER);
	rwlock->read_count--;

	if (!rwlock->read_count) {
		mutex_unlock(rwlock->rwlock_wrmtx);
	}
	mutex_unlock(rwlock->rwlock_rdmtx);
}

/**
 * Locks/takes a write mutex.
 *
 * This service may block while waiting on the mutex's
 * availability, depending on the timeout parameter.
 *
 * Authorized execution levels:  task, fiber.
 *
 * @param rwlock: handler on the read/write lock to lock write lock,
 *
 * @param timeout: maximum number of milliseconds to wait for the mutex. Special
 *            values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 */
void rwlock_wrlock(struct rwlock_t *rwlock, int32_t timeout)
{
	mutex_lock(rwlock->rwlock_wrmtx, timeout);
}

/**
 * Unlocks/gives a write mutex.
 *
 * Authorized execution levels:  task, fiber.
 *
 * @param rwlock: handler on the read/write lock to unlock write lock,
 *
 */
void rwlock_wrunlock(struct rwlock_t *rwlock)
{
	mutex_unlock(rwlock->rwlock_wrmtx);
}
#endif
