/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "queue.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

TaskHandle_t ToggleTask1Handler = NULL;                           // elhandler beta3 elfunction
TaskHandle_t ToggleTask2Handler = NULL;
/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/
/*Execution time variables*/

unsigned int B1_inTime;
unsigned int B2_inTime;
unsigned int L1_inTime;
unsigned int L2_inTime;
unsigned int Tx_inTime;
unsigned int Rx_inTime;

unsigned int total_exeTime;
unsigned int cpu_load;

/*Task Handlers*/
BaseType_t xReturned;
TaskHandle_t Button1_Handler = NULL;
TaskHandle_t Button2_Handler = NULL;

TaskHandle_t Tx_Handler = NULL;
TaskHandle_t Rx_Handler = NULL;

TaskHandle_t Load1_Handler = NULL;
TaskHandle_t Load2_Handler = NULL;

/*Queue Handlers*/
QueueHandle_t Q1 = NULL;
QueueHandle_t Q2 = NULL;
QueueHandle_t Q3 = NULL;


/*RunTime stats buffer*/
char RTstats[300];

// set Hooks

void vApplicationTickHook (void)
{
	GPIO_write(PORT_0 , PIN0, PIN_IS_HIGH);
	GPIO_write(PORT_0 , PIN0, PIN_IS_LOW);
}





/******************************************************************************************************/
                                     /*Tasks Implemntations*/
/******************************************************************************************************/


/* ========================================================= */
													/* Task 1*/
/* ========================================================= */

void Button_1_Monitor( void * pvParameters )
{
	pinState_t Button_1_State;
	pinState_t PrevState = GPIO_read(PORT_1 , PIN0);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	signed char EdgeFlag = 0;

	for( ;; )
	{
		/* Read GPIO Input */
		Button_1_State = GPIO_read(PORT_1 , PIN0);
		
		/*Check */
		if( Button_1_State == PIN_IS_HIGH && PrevState == PIN_IS_LOW)
		{
			
			EdgeFlag = '+';   /* Positive Edge */
		}
		else if (Button_1_State == PIN_IS_LOW && PrevState == PIN_IS_HIGH)
		{
			
			EdgeFlag = '-';    /* Negative Edge */
		}
		else
		{
			EdgeFlag = '.';
		}
	
		
		xQueueOverwrite( Q1 , &EdgeFlag );              /* Send Data  */  
		                                           
		PrevState = Button_1_State;                   /* Update Button 1 State */	                                          
	                                           
		vTaskDelayUntil( &xLastWakeTime , 50);      /*  Periodicity is 50  */
	}
}


/* ========================================================= */
													/* Task 2*/
/* ========================================================= */

void Button_2_Monitor( void * pvParameters )
{
	pinState_t Button_2_State;
	pinState_t PrevState = GPIO_read(PORT_1 , PIN1);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	unsigned char EdgeFlag = 0;
	

	for( ; ; )
	{
		/* Read GPIO Input */
		Button_2_State = GPIO_read(PORT_1 , PIN1);
		
		/*Check for Edges*/
		if( Button_2_State == PIN_IS_HIGH && PrevState == PIN_IS_LOW)
		{
			EdgeFlag = '+';  /*Positive Edge*/
		}
		else if (Button_2_State == PIN_IS_LOW && PrevState == PIN_IS_HIGH)
		{
			EdgeFlag = '-';   /*Negative Edge*/
		}
		else
		{
			EdgeFlag = '.';
		}
		
		
		xQueueOverwrite( Q2 , &EdgeFlag );    /* Send Data */
		
		PrevState = Button_2_State;        /* Update Button1 State */
		
		vTaskDelayUntil( &xLastWakeTime , 50); /* Periodicity  is 50  */
	}
}


/* ========================================================= */
													/* Task 3*/
/* ========================================================= */

void Periodic_Transmitter (void * pvParameters )
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint8_t i = 0;
	char pcString[15];
	strcpy(pcString, "\n100ms Marker.");

	for( ; ; )
	{
		/*Send string characters over Queue Q3 to Uart_Receiver*/
		for( i=0 ; i<15 ; i++)
		{
			xQueueSend( Q3 , pcString+i ,100);
		}
		
		/*Periodicity: 100*/
		vTaskDelayUntil( &xLastWakeTime , 100);
	}
}

void Uart_Receiver (void * pvParameters )
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	signed char B1;
	signed char B2;
	char rxString[15];
	uint8_t i=0;
	for( ; ; )
	{
		/*Receive Button 1 State*/
		if( xQueueReceive( Q1, &B1 , 0) && B1 != '.')
		{
			/*Transmit if +/- edge detected*/
			xSerialPutChar('\n');		
			xSerialPutChar('B');
			xSerialPutChar('1');
			xSerialPutChar(':');
			xSerialPutChar(B1);
		}
		else
		{
			/*Transmit spaces to take the same execution time 
			*even if Received nothing */
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
		}
		
		/*Receive Button 2 state*/
		if( xQueueReceive( Q2, &B2 , 0) && B2 != '.')
		{
			/*Transmit if +/- edge detected*/
			xSerialPutChar('\n');		
			xSerialPutChar('B');
			xSerialPutChar('2');
			xSerialPutChar(':');
			xSerialPutChar(B2);
		}
		else
		{
			/*Transmit spaces to take the same execution time 
			*even if Received nothing */
			xSerialPutChar(' ');		
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
		}
		
		/*Receive String from Periodic_Transmitter*/
		if( uxQueueMessagesWaiting(Q3) != 0)
		{
			for( i=0 ; i<15 ; i++)
			{
				xQueueReceive( Q3, (rxString+i) , 0);
			}
			vSerialPutString( (signed char *) rxString, strlen(rxString));
			xQueueReset(Q3);
		}
		

		/*get RunTime Statistics Over Uart terminal*/
	/*  xSerialPutChar('\n');		
	    vTaskGetRunTimeStats(RTstats);
      vSerialPutString( (signed char *) RTstats, strlen(RTstats));
	*/
		/*Periodicity: 20*/
		vTaskDelayUntil( &xLastWakeTime , 20);
	}
}
	




void Load_1_Simulation ( void * pvParameters )
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint32_t i = 0;
	uint32_t x = 12000*5; /* (XTAL / 1000U)*time_in_ms  */
	for( ; ; )
	{
		for( i=0 ; i <= x; i++)
		{
			/*5ms delay*/
		}
		
		vTaskDelayUntil( &xLastWakeTime , 10);           /*Periodicity: 10*/  
	}
}

void Load_2_Simulation ( void * pvParameters )
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint32_t i = 0;
	uint32_t x = 12000*12; /* (XTAL / 1000U)*time_in_ms  */
		
	for( ; ; )
	{		
		for( i=0 ; i <= x; i++)
		{
			/*12ms delay*/
		}
		
		vTaskDelayUntil( &xLastWakeTime , 100);  /*Periodicity: 100*/
		
	}
}


/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
 {
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	Q1 = xQueueCreate( 1, sizeof(char) );
	Q2 = xQueueCreate( 1, sizeof(char) );
	Q3 = xQueueCreate( 15, sizeof(char) );
	
	 
/*****************************************************************************************************/
																			/* Create Tasks */
/*****************************************************************************************************/
         xTaskPeriodicCreate(
                       Button_1_Monitor,        /* Function that implements the task. */
                       "B1",     					/* Text name for the task. */
                       100,      			        	/* Stack size in words, not bytes. */
                       ( void * ) 0,         		/* Parameter passed into the task. */
                       1,						            /* Priority at which the task is created. */
                       &Button1_Handler		    /* Used to pass out the created task's handle. */
                          ,50);      				    /* Task Deadline */
                
         xTaskPeriodicCreate(
                       Button_2_Monitor,         /* Function that implements the task. */
                       "B2",                /* Text name for the task. */
                       100,      				         /* Stack size in words, not bytes. */
                       ( void * ) 0,    		     /* Parameter passed into the task. */
                       1,						             /* Priority at which the task is created. */
                       &Button2_Handler,		     /* Used to pass out the created task's handle. */
                       	50);      				       /* Task Deadline */
                                      
         xTaskPeriodicCreate(
                       Uart_Receiver,       /* Function that implements the task. */
                       "Rx",                /* Text name for the task. */
                       100,      			    	/* Stack size in words, not bytes. */
                       ( void * ) 0,    		/* Parameter passed into the task. */
                       1,						        /* Priority at which the task is created. */
                       &Rx_Handler,		      /* Used to pass out the created task's handle. */
                       20);      			    	/* Task Deadline */
                
                
         xTaskPeriodicCreate(
                       Periodic_Transmitter,      /* Function that implements the task. */
                       "Tx",                      /* Text name for the task. */
                       100,      			          	/* Stack size in words, not bytes. */
                       ( void * ) 0,    	      	/* Parameter passed into the task. */
                       1,					              	/* Priority at which the task is created. */
                       &Tx_Handler,		            /* Used to pass out the created task's handle. */
                       100);                      /* Task Deadline */
								
										
         xTaskPeriodicCreate(
               Load_1_Simulation,           /* Function that implements the task. */
               "L1",                        /* Text name for the task. */
               100,      				            /* Stack size in words, not bytes. */
               ( void * ) 0,    	        	/* Parameter passed into the task. */
               1,						                /* Priority at which the task is created. */
               &Load1_Handler,		          /* Used to pass out the created task's handle. */
				              10);      			     	/* Task Deadline */
	
	
					xTaskPeriodicCreate(
                       Load_2_Simulation,       /* Function that implements the task. */
                       "L2",                    /* Text name for the task. */
                       100,      			        	/* Stack size in words, not bytes. */
                       ( void * ) 0,    		    /* Parameter passed into the task. */
                       1,						            /* Priority at which the task is created. */
                       &Load2_Handler,		      /* Used to pass out the created task's handle. */
                        100);      				      /* Task Deadline */

										

	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/


