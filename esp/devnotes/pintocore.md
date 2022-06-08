# Error: xTaskCreateRestrictedPinnedToCore
## Solution 1 (probably won't work)
Go to your ESP-IDF folder and perform `git apply <esp-adf>/idf_patches/idf_v4.4_freertos.patch`, replacing <...> part with the path to your ESP-ADF folder.<br>
If errors occur, **DO NOT TRY TO PERFORM IT WITH `--reject` OPTION**, move to second solution instead:

## Solution 2
### Step 1
Check if `(...)esp-idf/components/freertos/tasks.c` file has a function named `xTaskCreateRestrictedPinnedToCore`. If not, 
insert the following to the end of that file:
```c
#if ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

	BaseType_t xTaskCreateRestrictedPinnedToCore( const TaskParameters_t * const pxTaskDefinition,
                                                  TaskHandle_t *pxCreatedTask,
                                                  const BaseType_t xCoreID)
	{
	TCB_t *pxNewTCB;
	BaseType_t xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

		configASSERT( pxTaskDefinition->puxStackBuffer );

		if( pxTaskDefinition->puxStackBuffer != NULL )
		{
			/* Allocate space for the TCB.  Where the memory comes from depends
			on the implementation of the port malloc function and whether or
			not static allocation is being used. */
			pxNewTCB = ( TCB_t * ) pvPortMallocTcbMem( sizeof( TCB_t ) );

			if( pxNewTCB != NULL )
			{
				/* Store the stack location in the TCB. */
				pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;

				/* Tasks can be created statically or dynamically, so note
				this task had a statically allocated stack in case it is
				later deleted.  The TCB was allocated dynamically. */
				pxNewTCB->ucStaticallyAllocated = tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB;

				prvInitialiseNewTask(	pxTaskDefinition->pvTaskCode,
										pxTaskDefinition->pcName,
										pxTaskDefinition->usStackDepth,
										pxTaskDefinition->pvParameters,
										pxTaskDefinition->uxPriority,
										pxCreatedTask, pxNewTCB,
										pxTaskDefinition->xRegions,
										xCoreID );

				prvAddNewTaskToReadyList( pxNewTCB, pxTaskDefinition->pvTaskCode, xCoreID );
				xReturn = pdPASS;
			}
		}

		return xReturn;
	}

#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
```

### Step 2
Similarly, check if `(...)esp-idf/components/freertos/include/tasks.h` has this function declared. If not, insert it to the end of that file:
```c
#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )

	BaseType_t xTaskCreateRestrictedPinnedToCore( const TaskParameters_t * const pxTaskDefinition,
                                                  TaskHandle_t *pxCreatedTask,
                                                  const BaseType_t xCoreID);


#endif
```
