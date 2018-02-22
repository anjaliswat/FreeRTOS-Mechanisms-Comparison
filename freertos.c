/* freertos includes. */
#include "freertos.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
#include "semphr.h"

/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"

#define TIMER_ID    1
#define DELAY_10_SECONDS    10000UL
#define DELAY_1_SECOND      1000UL
#define TIMER_CHECK_THRESHOLD   9

#define DO_NOT_RUN 0 

#define IPC_METHOD_QUEUE 			0
#define IPC_METHOD_EVENT_GROUP	 		1
#define IPC_METHOD_SEMAPHORE 			2
#define IPC_METHOD_DIRECT_TASK_NOTIFICATION 	3

#define DUMMY_TASKS 1 
#define DUMMY_TASK_COUNT 3 
#define DUMMY_TASK_DELAY_EN 0
#define DUMMY_TASK_DELAY 1 
#define DEBUG_MESSAGES_DUMMY 0

//#define IPC_METHOD_TYPE_SELECTED IPC_METHOD_QUEUE

#define IPC_METHOD_TYPE_SELECTED IPC_METHOD_EVENT_GROUP
#define IPC_EG_TASK_COUNT	20 // Max Value 23 
#define IPC_EG_ITERATION	3 
#define IPC_EG_SYNC_INTERVAL	200 //the higher the slower??
#define IPC_EG_TASK_DELAY_EN	1
#define IPC_EG_TASK_DELAY	10
#define DEBUG_MESSAGES_EGSLV	0
#define DEBUG_MESSAGES_EGMST	0
#define DEBUG_MESSAGES_EGMST_TM	0
#define HEX_VALUE_01 0x01

//#define IPC_METHOD_TYPE_SELECTED IPC_METHOD_SEMAPHORE
#define IPC_SEM_TASK_COUNT	20 
#define IPC_SEM_ITERATION	3
#define IPC_SEM_SYNC_INTERVAL	200 //the higher the slower??
#define IPC_SEM_TASK_DELAY_EN	1 
#define IPC_SEM_TASK_DELAY	1 
#define DEBUG_MESSAGES_SEMSLV	0
#define DEBUG_MESSAGES_SEMMST	0
#define DEBUG_MESSAGES_SEMMST_TM 0

//#define IPC_METHOD_TYPE_SELECTED IPC_METHOD_DIRECT_TASK_NOTIFICATION
#define IPC_DTN_TASK_COUNT	20 
#define IPC_DTN_ITERATION	3
#define IPC_DTN_SYNC_INTERVAL	200 //the higher the slower??
#define IPC_DTN_TASK_DELAY_EN	1
#define IPC_DTN_TASK_DELAY	1
#define DEBUG_MESSAGES_DTNSLV	0
#define DEBUG_MESSAGES_DTNMST	0
#define DEBUG_MESSAGES_DTNMST_TM 0

#define DEBUG_MESSAGES_CR 	0

/*Dummy Task*/
static void prvDummyTask( void *pvParameters );

/* Synch Test Functions Below. */

static void CRITICAL_REGION_FUNCTION( EventBits_t slave_id );
static void prvConsTask_test( void *pvParameters );

static void prvConsTask_EG( void *pvParameters );
static void prvMstrTask_EG( void *pvParameters );
static BaseType_t prvMasterLauncher_EG();

static void prvConsTask_SEM( void *pvParameters );
static void prvMstrTask_SEM( void *pvParameters );
static BaseType_t prvMasterLauncher_SEM( void );

static void prvConsTask_DTN( void *pvParameters );
static void prvMstrTask_DTN( void *pvParameters );
static BaseType_t prvMasterLauncher_DTN( void );

/* Project variables below. */
static volatile uint32_t ulTestMasterCycles = 0;
static volatile uint32_t ulTestInt0 = 0;
static TickType_t xTotalTimeSuspended;
static TickType_t xTimeIteration;
static EventGroupHandle_t xEventGroup = NULL;
static SemaphoreHandle_t xSemaphore[IPC_SEM_TASK_COUNT+1];
/* Handles to the tasks that take part in the synchronisation calls. */
static TaskHandle_t xConsumerEG[IPC_EG_TASK_COUNT];
static TaskHandle_t xConsumerSEM[IPC_SEM_TASK_COUNT];
static TaskHandle_t xConsumerDTN[IPC_DTN_TASK_COUNT];
static TaskHandle_t xMaster = NULL;
static volatile uint32_t xEventGroupEG_status = 0;
static volatile uint32_t xTaskNotifyWait_status = 0;
static int sync_bits_eg_master;
static int hex_val_shifted;

int main( void )
{
    int method_type = IPC_METHOD_TYPE_SELECTED;
    int run_dummy_tasks = DUMMY_TASKS;
    int num_of_dummy_tasks = DUMMY_TASK_COUNT;
    int debug_prints_dummy = DEBUG_MESSAGES_DUMMY; 
    int debug_prints_EGslave = DEBUG_MESSAGES_EGSLV; 
    int debug_prints_SEMslave = DEBUG_MESSAGES_SEMSLV; 
    int debug_prints_DTNslave = DEBUG_MESSAGES_DTNSLV; 
    int i;
    char dummytaskname[64] = "DummyTask";
    char consumertaskname[64] = "Consumer";

    xil_printf( " \r\n" );
    xil_printf( " \r\n" );
    xil_printf( " ============================================= \r\n" );
    xil_printf( "  OS N-way Relay Synchronization Test Program  \r\n" );
    xil_printf( " ============================================= \r\n" );
    xil_printf( " \r\n" );
    xil_printf( " \r\n" );

    if(run_dummy_tasks == 1 ) 
    {
    	if (debug_prints_dummy == 1) 
	{
    		xil_printf(" ----  Dummy Tasks Enabled  ----\r\n");
	}

    	/*Dummy Tasks Set with Higher Priority*/

	for(i=0;i<num_of_dummy_tasks;i++) 
	{
	    itoa(i,dummytaskname+9,10);
    	    if (debug_prints_dummy == 1) 
	    {
    		xil_printf( " ### DUMMY TASKS #####  %s \r\n",dummytaskname);
	    }
		
    	    xTaskCreate( prvDummyTask, (char *) dummytaskname, configMINIMAL_STACK_SIZE, (void *) i, tskIDLE_PRIORITY+1, NULL );
	}
    }

  
    if (method_type == IPC_METHOD_QUEUE)
    {
    	xil_printf( " -------------------- \r\n" );
    	xil_printf( "  Using QUEUE method  \r\n" );
    	xil_printf( " -------------------- \r\n\n" );
    }
    else if (method_type == IPC_METHOD_EVENT_GROUP)
    {
    	xil_printf( " -------------------------- \r\n" );
    	xil_printf( "  Using EVENT GROUP method  \r\n" );
    	xil_printf( " -------------------------- \r\n\n" );
    	int num_of_tasks = IPC_EG_TASK_COUNT;
    	int hex_val;
	
    	/*Event Group Synchronization Method Tasks*/
	for(i=0;i<num_of_tasks;i++) 
	{
	    itoa(i,consumertaskname+8,10);
    	    if (debug_prints_EGslave == 1) { xil_printf("  %s \r\n",consumertaskname); }
 	    hex_val = HEX_VALUE_01 << i;
    	    xTaskCreate( prvConsTask_EG, (char *) consumertaskname, configMINIMAL_STACK_SIZE, ( void * ) hex_val, tskIDLE_PRIORITY+1, &xConsumerEG[i]);
    	    if (debug_prints_EGslave == 1) 
	    {
    	    xil_printf(" Task %d syncbits %X handle = 0x%X \r\n",i,hex_val,xConsumerEG[i]);
	    }
	}
        sync_bits_eg_master = hex_val<<1;
	xTaskCreate( prvMstrTask_EG, "Master", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL );
    }
    else if (method_type == IPC_METHOD_SEMAPHORE)
    {
    	xil_printf( " -------------------------- \r\n" );
    	xil_printf( "  Using SEMAPHORE method  \r\n" );
    	xil_printf( " -------------------------- \r\n\n" );
    	int num_of_tasks = IPC_SEM_TASK_COUNT;
    /*Sempahore Synchronization Method Tasks*/
	for(i=0;i<num_of_tasks;i++) 
	{
	    itoa(i,consumertaskname+8,10);
    	    if (debug_prints_SEMslave == 1) 
	    { 
		    xil_printf("  %s \r\n",consumertaskname); 
	    }
    	    xTaskCreate( prvConsTask_SEM, (char *) consumertaskname, configMINIMAL_STACK_SIZE, ( void * ) i, tskIDLE_PRIORITY+1, &xConsumerSEM[i]);
    	    if (debug_prints_SEMslave == 1) 
	    {
    	    	xil_printf(" Task %d handle = 0x%X \r\n",i,xConsumerSEM[i]);
	    }
	}
    	xTaskCreate( prvMstrTask_SEM, "Master", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &xMaster );
    }
    else if (method_type == IPC_METHOD_DIRECT_TASK_NOTIFICATION)
    {
    	xil_printf( " --------------------------------------- \r\n" );
    	xil_printf( "  Using DIRECT TASK NOTIFICATION method  \r\n" );
    	xil_printf( " --------------------------------------- \r\n\n" );
    	int num_of_tasks = IPC_DTN_TASK_COUNT;

	for(i=0;i<num_of_tasks;i++) 
	{
	    itoa(i,consumertaskname+8,10);
    	    if (debug_prints_DTNslave == 1) 
	    {
	    	xil_printf("  %s \r\n",consumertaskname); 
	    }
    	    xTaskCreate( prvConsTask_DTN, (char *) consumertaskname, configMINIMAL_STACK_SIZE, ( void * ) i, tskIDLE_PRIORITY+1, &xConsumerDTN[i]);
    	    if (debug_prints_DTNslave == 1) 
	    {
    	    	xil_printf(" Task %d handle = 0x%X \r\n",i,xConsumerDTN[i]);
	    }
	}
    	xTaskCreate( prvMstrTask_DTN, "Master", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &xMaster );
    }
    else
    {
    	xil_printf( " ---------------------------- \r\n" );
    	xil_printf( "  Valid Method not selected   \r\n" );
    	xil_printf( " ---------------------------- \r\n\n" );
    }
    
    vTaskStartScheduler();
    for( ;; );
}

static void prvDummyTask( void *pvParameters )
{
    int task_index = ( int ) pvParameters;
    int debug_prints_dummy = DEBUG_MESSAGES_DUMMY; 
    int dummy_task_delay_en = DUMMY_TASK_DELAY_EN; 
    
    if (debug_prints_dummy == 1) 
    {
	    xil_printf("Starting Dummy Process : %d \r\n",task_index);
    }


    for( ;; )
    {
        for(int i = 0; i < 1000; i++)
        { 
    	    if (debug_prints_dummy == 1) {
    		xil_printf("Dummy Process %d : %d \r\n",task_index,ulTestInt0);
	    }
            if (ulTestInt0 == 10000)
               ulTestInt0 = 0;
            else
               ++ulTestInt0;
        }
    if(dummy_task_delay_en) { vTaskDelay(DUMMY_TASK_DELAY); }
    }
}

static void prvConsTask_test( void *pvParameters )
{
    EventBits_t testParam;
    testParam = ( EventBits_t ) pvParameters;

    for( ;; )
    {
	xil_printf(" =======================================\r\n");
	xil_printf(" TEST Slave %d : %X : Process suspending\r\n",testParam,testParam);
	xil_printf(" =======================================\r\n");
        vTaskSuspend( NULL );
    }
}
static void prvConsTask_EG( void *pvParameters )
{
    EventBits_t uxSynchronisationBit;
    uxSynchronisationBit = ( EventBits_t ) pvParameters;
    int debug_prints_EGslave = DEBUG_MESSAGES_EGSLV; 
    int eg_task_delay_en = IPC_EG_TASK_DELAY_EN; 

    for( ;; )
    {
        if (debug_prints_EGslave == 1) 
	{
	       	xil_printf(" EG Slave: Process suspending\r\n");
	}
        vTaskSuspend( NULL );
        if (debug_prints_EGslave == 1) 
	{
        	xil_printf(" EG Slave: Process 0x%x queued, Process now waiting on Master to Set Event Group\r\n",uxSynchronisationBit);
	}

        xEventGroupEG_status = xEventGroupWaitBits( xEventGroup,           /* EvenetGroup Handle.*/
                                                    uxSynchronisationBit,  /* The bit to wait for. */
                                                    pdTRUE,                /* Clear the bit on exit. */
                                                    pdTRUE,                /* Wait for all the bits. */
                                                    IPC_EG_SYNC_INTERVAL );       /* Block indefinitely to wait for the condition to be met. */

	vTaskDelay(25);
        if (debug_prints_EGslave == 1) 
	{
        	xil_printf(" EG Slave: Critical Region \r\n");
	}

        CRITICAL_REGION_FUNCTION(uxSynchronisationBit);

        if (debug_prints_EGslave == 1) 
	{
        	xil_printf(" EG Slave:-----> EG Status = 0x%X \r\n", xTaskNotifyWait_status);
        	xil_printf(" EG Slave: Process completed\r\n");
        	xil_printf(" \r\n");
        	xil_printf(" \r\n");
	}
        xEventGroupSetBits(xEventGroup, (uxSynchronisationBit << 1)); //shift 1 in
        if (debug_prints_EGslave == 1) 
	{
    		xil_printf(" \r\n");
    		xil_printf(" EG Slave: Setting Event Group for Consumer 0x%X \r\n",uxSynchronisationBit);
    		xil_printf(" \r\n");
	}
    if(eg_task_delay_en) { vTaskDelay(IPC_EG_TASK_DELAY); }
    }
}

static BaseType_t prvMasterLauncher_EG()
{
    TickType_t xTimeBefore, xTimeNow;
    int num_of_tasks =IPC_EG_TASK_COUNT;
    int debug_prints_EGmaster = DEBUG_MESSAGES_EGMST; 
    int debug_prints_EGmaster_timeMeasure = DEBUG_MESSAGES_EGMST_TM; 
    int i;
    
    if (debug_prints_EGmaster == 1) 
    {
	xil_printf(" EG Master: Resuming the Slave tasks that are blocked\r\n");
    }

    for(i=0;i<num_of_tasks;i++) { vTaskResume( xConsumerEG[i] ); }

    if (debug_prints_EGmaster == 1) 
    {
    xil_printf(" EG Master: Slave tasks are now queued, initiating chain\r\n");
    }

    xTimeBefore = xTaskGetTickCount(); //one tick is 10ms
    if (debug_prints_EGmaster_timeMeasure == 1) 
    {
    	xil_printf(" =====>> EG Master: Time Start : %d  <<===== \r\n",xTimeBefore);
    }

    if (debug_prints_EGmaster == 1) { xil_printf(" EG Master: Set Event Group for Consumer 0\r\n"); }

    xEventGroupSetBits(xEventGroup, 0x01);

    if (debug_prints_EGmaster == 1) { xil_printf(" EG Master: Wait for Event Group from last slave\r\n"); }
    xEventGroupWaitBits( xEventGroup, sync_bits_eg_master,            /* The bit to wait for. */
                                      pdTRUE,         /* Clear the bits on exit. */
                                      pdTRUE,          /* Wait for all the bits (only one in this case anyway). */
                                      IPC_EG_SYNC_INTERVAL ); /* Block indefinitely to wait for the condition to be met. */

    if (debug_prints_EGmaster == 1) { xil_printf(" EG Master: Got Event Group\r\n"); }

    if (debug_prints_EGmaster == 1) { xil_printf("\r\n EG Master: Chain Completed\r\n"); }

    xTimeNow = xTaskGetTickCount();

    if (debug_prints_EGmaster_timeMeasure == 1) 
    {
    	xil_printf(" =====>> EG Master: Time Stop  : %d  <<===== \r\n",xTimeNow);
    }

    xTimeIteration=xTimeNow-xTimeBefore;

    if (debug_prints_EGmaster_timeMeasure == 1) 
    {
    	xil_printf(" =====>> EG Master: Time taken for Iteration : %d <<===== \r\n",xTimeIteration);
    }

    return pdFALSE;
}

static void prvMstrTask_EG( void *pvParameters )
{
    BaseType_t xError;
    int num_of_tasks = IPC_EG_TASK_COUNT;
    int debug_prints_EGmaster = DEBUG_MESSAGES_EGMST; 
    int i;
    
    xEventGroup = xEventGroupCreate();
    ulTestMasterCycles = 0;
    xTotalTimeSuspended = 0;

    if (debug_prints_EGmaster == 1) 
    {
    	xil_printf(" EG Master: Test Start!\r\n");
    }
    
    for(i = 0; i<IPC_EG_ITERATION; i++)
    {

    	xil_printf("\n@@@@@@@@@@@@@@@@ ITERATION %d @@@@@@@@@@@@@@@@@@@@@@\r\n",i);
        xError = prvMasterLauncher_EG();
    	xTotalTimeSuspended = xTotalTimeSuspended + xTimeIteration; 

    if (debug_prints_EGmaster == 1) { xil_printf(" Iterations : %d  \r\n",i); }

    if (debug_prints_EGmaster == 1) 
     {
 	xil_printf(".................................\r\n");
 	xil_printf(" prvMasterLauncher_EG Error : %d \r\n", xError);
 	xil_printf(".................................\r\n");
     }
        /* Now all the other tasks should have completed and suspended
        themselves ready for the next go around the loop. */

    for(int j=0;j<num_of_tasks;j++) { while( eTaskGetState( xConsumerEG[i] ) != eSuspended ) {} };
    
    /* Only increment the cycle variable if no errors have been detected. */
    if( xError == pdFALSE )
       {
           ulTestMasterCycles++;
       }
    }

    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("-------------------------------------------\r\n");
    xil_printf(" EG Master: The chain completed %d cycles. \r\n", ulTestMasterCycles);
    xil_printf("-------------------------------------------\r\n");
    xil_printf("\r\n");
    xil_printf("-------------------------------------\r\n");
    xil_printf(" EG Master: The time delta is %2d mS \r\n", xTotalTimeSuspended);    
    xil_printf("-------------------------------------\r\n");
    xil_printf("\r\n");
}


static void prvConsTask_SEM( void *pvParameters )
{
    int sem_index = ( int ) pvParameters;
    int debug_prints_SEMslave = DEBUG_MESSAGES_SEMSLV; 
    int sem_task_delay_en = IPC_SEM_TASK_DELAY_EN; 

    for( ;; )
    {
        
    if (debug_prints_SEMslave == 1) 
    {
    	xil_printf("SEM Slave %d: (1) Suspend Task \r\n",sem_index);
    }

    vTaskSuspend( NULL );

    if (debug_prints_SEMslave == 1) 
    {
    	xil_printf("SEM Slave %d: (1) Resumed by Master \r\n",sem_index);
    }

    vTaskDelay(IPC_SEM_TASK_DELAY);

    vTaskResume( xMaster );

    if (debug_prints_SEMslave == 1) 
    {
       	xil_printf("SEM Slave %d: Waiting to take sempahore \r\n", sem_index);
    }

    while ( xSemaphoreTake(xSemaphore[sem_index], ( TickType_t ) IPC_SEM_SYNC_INTERVAL) != pdTRUE ) 
    {
	if (debug_prints_SEMslave == 1) 
    	{
       		xil_printf("SEM Slave %d: Semaphore Take Failed (1) \r\n",sem_index);
    	} 
    } 
        vTaskDelay(1);
        if (debug_prints_SEMslave == 1) 
	{
        	xil_printf(" SEM Slave: Critical Region \r\n");
	}

        CRITICAL_REGION_FUNCTION(sem_index);

        if (debug_prints_SEMslave == 1) 
	{
        	xil_printf("SEM Slave %d: Process queued, Process now waiting on Semaphore %d\r\n", sem_index, sem_index);
	}

        while ( xSemaphoreGive(xSemaphore[sem_index+1]) != pdPASS ) {} 

    	if (debug_prints_SEMslave == 1) 
    	{
        	xil_printf("\n SEM Slave %d : Semaphore Given for Slave %d (3.1) \r\n",sem_index, sem_index+1);
    	}

    if(sem_task_delay_en) { vTaskDelay(IPC_SEM_TASK_DELAY); }
    }
}

static BaseType_t prvMasterLauncher_SEM( void )
{
    TickType_t xTimeBefore, xTimeNow;
    int num_of_tasks = IPC_SEM_TASK_COUNT;
    int debug_prints_SEMmaster = DEBUG_MESSAGES_SEMMST; 
    int debug_prints_SEMmaster_timeMeasure = DEBUG_MESSAGES_SEMMST_TM; 

    if (debug_prints_SEMmaster == 1) 
    {
    xil_printf(" Semaphore 0 value = 0x%X \r\n",xSemaphore[0]);
    }

    if (debug_prints_SEMmaster == 1) 
    {
    	xil_printf("SEM Master: (1) Resuming all Slave Tasks \r\n");
    }

    for(int m=0;m<num_of_tasks;m++) { vTaskResume( xConsumerSEM[m] ); }

    if (debug_prints_SEMmaster == 1) 
    {
    	xil_printf("SEM Master: Suspend Master Task \r\n");
    }

    vTaskSuspend( NULL );

    if (debug_prints_SEMmaster == 1) 
    {
    	xil_printf("SEM Master: Resumed by one of the Consumer \r\n");
    }

//    vTaskDelay(IPC_SEM_TASK_DELAY); // Adding delay to prevend the vTaskResume in this master task to occur before the vsuspend in slave task.

    if (debug_prints_SEMmaster == 1) 
    {
    	xil_printf("SEM Master: Slave tasks are now queued, initiating chain\r\n");
    }

    xTimeBefore = xTaskGetTickCount(); //one tick is 10ms
    if (debug_prints_SEMmaster_timeMeasure == 1) {
    	xil_printf(" =====>> SEM Master: Time Start : %d  <<===== \r\n",xTimeBefore);
    }

    if (debug_prints_SEMmaster == 1) 
    {
    	xil_printf("SEM Master: Give Semaphore 0 \r\n");
    }

    while ( xSemaphoreGive(xSemaphore[0]) != pdPASS ) {}

    //attempt to take the last semaphore in the chain
    if (debug_prints_SEMmaster == 1) 
    {
   	 xil_printf("SEM Master: Wait for Semaphore %d \r\n",num_of_tasks);
    }

    while ( xSemaphoreTake(xSemaphore[num_of_tasks], ( TickType_t ) IPC_SEM_SYNC_INTERVAL) != pdPASS ) {} 

    xTimeNow = xTaskGetTickCount();

    if (debug_prints_SEMmaster_timeMeasure == 1) {
    	xil_printf(" =====>> SEM Master: Time Stop  : %d  <<===== \r\n",xTimeNow);
    }

    xTimeIteration=xTimeNow-xTimeBefore;

    if (debug_prints_SEMmaster_timeMeasure == 1) 
    {
    	xil_printf(" =====>> SEM Master: Time taken for Iteration : %d <<===== \r\n",xTimeIteration);
    }

    return pdFALSE;
}

static void prvMstrTask_SEM( void *pvParameters )
{
    BaseType_t xError;
    int num_of_tasks = IPC_SEM_TASK_COUNT;
    int debug_prints_SEMmaster = DEBUG_MESSAGES_SEMMST; 
    
    ulTestMasterCycles = 0;
    xTotalTimeSuspended = 0;

    for(int i = 0; i < num_of_tasks+1; i++){

        //xSemaphore[i] = xSemaphoreCreateCounting( 1, 0 );
        xSemaphore[i] = xSemaphoreCreateBinary();

	if (xSemaphore[i] != NULL ) 
	{
    		if (debug_prints_SEMmaster == 1) 
    		{
			xil_printf(" SEM Master :  Semaphore %d Created with handle 0x%X \r\n",i,xSemaphore[i]);
    		}
	}
	else
	{
    		if (debug_prints_SEMmaster == 1) 
    		{
			xil_printf(" SEM Master :  Semaphore %d Create FAILED : 0x%X \r\n",i,xSemaphore[i]);
    		}
	}
    }

    
    if (debug_prints_SEMmaster == 1) 
    {
    xil_printf("\n\nSEM Master: Test Start!\r\n\n");
    }

    for(int k = 0; k<IPC_SEM_ITERATION; k++)
    {
    	xil_printf("\n@@@@@@@@@@@@@@@@ ITERATION %d @@@@@@@@@@@@@@@@@@@@@@\r\n",k);
        xError = prvMasterLauncher_SEM();
    	xTotalTimeSuspended = xTotalTimeSuspended + xTimeIteration; 
	
        /* Now all the other tasks should have completed and suspended
        themselves ready for the next go around the loop. */

    for(int j=0;j<num_of_tasks;j++) { while( eTaskGetState( xConsumerSEM[j] ) != eSuspended ) {} };
    
    /* Only increment the cycle variable if no errors have been detected. */
    if( xError == pdFALSE )
     {
       ulTestMasterCycles++;
     }
    else
     {
      if (debug_prints_SEMmaster == 1) { xil_printf("SEM Master ERROR: Iteration %d completed with Errors\r\n", k); }
     }

   } 
    
    xil_printf("------------------------------------------\r\n");
    xil_printf(" SEM Master: The chain completed %d cycles\r\n", ulTestMasterCycles);
    xil_printf("------------------------------------------\r\n");
    xil_printf("\r\n");
    xil_printf("--------------------------------------\r\n");
    xil_printf(" SEM Master: The time delta is %2d mS\r\n", xTotalTimeSuspended);
    xil_printf("--------------------------------------\r\n");

}


static void prvConsTask_DTN( void *pvParameters )
{
    EventBits_t uTaskID;
    uTaskID = ( EventBits_t ) pvParameters;
    uint32_t ulNotifiedValueSlv;
    int debug_prints_DTNslave = DEBUG_MESSAGES_DTNSLV; 
    int dtn_task_delay_en = IPC_DTN_TASK_DELAY_EN; 
    int num_of_tasks = IPC_DTN_TASK_COUNT;

    for( ;; )
    {
    	if (debug_prints_DTNslave == 1) 
	{
	       	xil_printf(" DTN Slave %d : Process suspending\r\n",uTaskID);
	}

        vTaskSuspend( NULL );

    	if (debug_prints_DTNslave == 1) 
	{
        	xil_printf(" DTN Slave: Process 0x%x queued, Process now waiting on Master to Set Task Notification \r\n",uTaskID);
	}

        while (xTaskNotifyWait(0x00,0xFF,&ulNotifiedValueSlv,IPC_DTN_SYNC_INTERVAL) != pdTRUE) 
	{
    		if (debug_prints_DTNslave == 1) 
		{
        	xil_printf(" DTN Slave %d : Notify Wait Loop Status \r\n",uTaskID);
		}
	}

    	if (debug_prints_DTNslave == 1) 
	{
        xil_printf(" DTN Slave %d :-----> DTN Status = 0x%X \r\n",uTaskID,ulNotifiedValueSlv);
	}

    	int hex_val;
	for(int p=0;p<num_of_tasks;p++)
	{
 	 	hex_val = HEX_VALUE_01 << p;
		if(p<num_of_tasks-1) {
			if( (ulNotifiedValueSlv & hex_val) != 0 ) {
    				if (debug_prints_DTNslave == 1) { xil_printf(" DTN Slave %d Received Notification \r\n",p); }
				hex_val_shifted = hex_val<<1;
        			CRITICAL_REGION_FUNCTION(uTaskID);
    				if(xTaskNotify(xConsumerDTN[p+1],hex_val_shifted,eSetBits)==pdPASS) {
					if (debug_prints_DTNslave == 1) {xil_printf(" DTN Slave %d : Consumer1 Notified \r\n",uTaskID);}
				}
	  		}
		} else {
			if( (ulNotifiedValueSlv & hex_val) != 0 ) {
    				if (debug_prints_DTNslave == 1) { xil_printf(" DTN Slave %d Received Notification \r\n",p); }
				hex_val_shifted = hex_val<<1;
    				if(xTaskNotify(xMaster,hex_val_shifted,eSetBits)==pdPASS) {
					if (debug_prints_DTNslave == 1) {xil_printf(" DTN Slave %d : Master Notified \r\n",uTaskID);}
				}
			}
		}
	}

        if (debug_prints_DTNslave == 1) 
	{
        	xil_printf(" DTN Slave %d : Process completed\r\n",uTaskID);
        	xil_printf(" \r\n");
        	xil_printf(" \r\n");
	}

    if(dtn_task_delay_en) { vTaskDelay(IPC_DTN_TASK_DELAY); }
    }
}

static void prvMstrTask_DTN( void *pvParameters )
{
    BaseType_t xError;
    int num_of_tasks = IPC_DTN_TASK_COUNT;
    int debug_prints_DTNmaster = DEBUG_MESSAGES_DTNMST; 
    
    ulTestMasterCycles = 0;
    xTotalTimeSuspended = 0;

    if (debug_prints_DTNmaster == 1) 
    {
    	xil_printf(" DTN Master: Test Start!\r\n");
    }
    
    for(int i = 0; i<IPC_DTN_ITERATION; i++)
    {

    	xil_printf("\n@@@@@@@@@@@@@@@@ ITERATION %d @@@@@@@@@@@@@@@@@@@@@@\r\n",i);
        xError = prvMasterLauncher_DTN();
    	xTotalTimeSuspended = xTotalTimeSuspended + xTimeIteration; 

    if (debug_prints_DTNmaster == 1) 
     {
 	xil_printf(".................................\r\n");
 	xil_printf(" prvMasterLauncher_DTN Error : %d \r\n", xError);
 	xil_printf(".................................\r\n");
     }
        /* Now all the other tasks should have completed and suspended
        themselves ready for the next go around the loop. */

    for(int j=0;j<num_of_tasks;j++) { while( eTaskGetState( xConsumerDTN[i] ) != eSuspended ) {} };

    /* Only increment the cycle variable if no errors have been detected. */
    if( xError == pdFALSE )
     {
        ulTestMasterCycles++;
     }
    }

    xil_printf("\r\n");
    xil_printf("\r\n");
    xil_printf("-------------------------------------------\r\n");
    xil_printf(" DTN Master: The chain completed %d cycles. \r\n", ulTestMasterCycles);
    xil_printf("-------------------------------------------\r\n");
    xil_printf("\r\n");
    xil_printf("-------------------------------------\r\n");
    xil_printf(" DTN Master: The time delta is %2d mS \r\n", xTotalTimeSuspended);    
    xil_printf("-------------------------------------\r\n");
    xil_printf("\r\n");
}


static BaseType_t prvMasterLauncher_DTN( void )
{
    TickType_t xTimeBefore, xTimeNow;
    int num_of_tasks = IPC_DTN_TASK_COUNT;
    int debug_prints_DTNmaster = DEBUG_MESSAGES_DTNMST; 
    uint32_t ulNotifiedValueMst;
    int debug_prints_DTNslave = DEBUG_MESSAGES_DTNSLV; 
    int debug_prints_DTNmaster_timeMeasure = DEBUG_MESSAGES_DTNMST_TM; 
    
    if (debug_prints_DTNmaster == 1) 
    {
	xil_printf(" DTN Master: Resuming the Slave tasks that are blocked\r\n");
    }
    for(int i=0;i<num_of_tasks;i++) { vTaskResume( xConsumerDTN[i] ); }

    if (debug_prints_DTNmaster == 1) 
    {
    xil_printf(" DTN Master: Slave tasks are now queued, initiating chain\r\n");
    }

    xTimeBefore = xTaskGetTickCount(); //one tick is 10ms

    if (debug_prints_DTNmaster == 1) 
    {
    xil_printf(" \r\n");
    xil_printf(" DTN Master: Setting Notification for Consumer 0 \r\n");
    xil_printf(" \r\n");
    }


    if(xTaskNotify(xConsumerDTN[0],0x01,eSetBits)==pdPASS) {if (debug_prints_DTNmaster == 1) {xil_printf(" DTN Master : Consumer0 Notified \r\n");}}

    while (xTaskNotifyWait(0x00,0xFF,&ulNotifiedValueMst,IPC_DTN_SYNC_INTERVAL) != pdTRUE) 
    {
    	if (debug_prints_DTNslave == 1) 
	{
   		xil_printf(" DTN Master : Notify Wait Loop Status \r\n");
	}
    }

    if( (ulNotifiedValueMst & hex_val_shifted ) != 0 ) 
    {
   	if (debug_prints_DTNmaster == 1) { xil_printf(" DTN Master Received Notification \r\n"); }
    }

    if (debug_prints_DTNmaster == 1) 
    {
    	xil_printf("\r\n");
    	xil_printf(" DTN Master: Chain Completed\r\n");
    	xil_printf("\r\n");
    }

    xTimeNow = xTaskGetTickCount();

    if (debug_prints_DTNmaster_timeMeasure == 1) 
    {
    	xil_printf(" =====>> DTN Master: Time Start : %d  <<===== \r\n",xTimeBefore);
    	xil_printf(" =====>> DTN Master: Time Stop  : %d  <<===== \r\n",xTimeNow);
    }

    xTimeIteration=xTimeNow-xTimeBefore;

    if (debug_prints_DTNmaster_timeMeasure == 1) 
    {
    	xil_printf(" =====>> DTN Master: Time taken for Iteration : %d <<===== \r\n",xTimeIteration);
    }

    return pdFALSE;
}

static void CRITICAL_REGION_FUNCTION( EventBits_t slave_id )
{
    int debug_prints_cr = DEBUG_MESSAGES_CR; 

	taskENTER_CRITICAL();
	
	if (debug_prints_cr ==1) {
        xil_printf(" Slave %5X	 : Accessing Critical Region\r\n",slave_id);
	}

        for(int k = 0; k < 100000; k++)
        { 
            if (ulTestInt0 == 10000)
               ulTestInt0 = 0;
            else
               ++ulTestInt0;
        }

    	taskEXIT_CRITICAL();

}
