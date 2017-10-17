/*----------------------MAILBOXES WRITE SET REGISTERS --------------------------*/
#define CORE0_MAILBOX0  0x4000008CUL
#define CORE1_MAILBOX0  0x4000009CUL
#define CORE2_MAILBOX0  0x400000ACUL
#define CORE3_MAILBOX0  0x400000BCUL
/*------------------------------------------------------------------------------*/

#define MAILBOX_BASE 0x3F00B880UL

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
/*--------------------------------------------------------------------------}
;{               RASPBERRY PI MAILBOX HARRDWARE REGISTERS					}
;{-------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__, aligned(4))) _PI_MAILBOX {
	const uint32_t Read0;											// 0x00         Read data from VC to ARM
	uint32_t Unused[3];												// 0x04-0x0F
	uint32_t Peek0;													// 0x10
	uint32_t Sender0;												// 0x14
	uint32_t Status0;												// 0x18         Status of VC to ARM
	uint32_t Config0;												// 0x1C        
	uint32_t Write1;												// 0x20         Write data from ARM to VC
	uint32_t Unused2[3];											// 0x24-0x2F
	uint32_t Peek1;													// 0x30
	uint32_t Sender1;												// 0x34
	uint32_t Status1;												// 0x38         Status of ARM to VC
	uint32_t Config1;												// 0x3C 
} PI_MAILBOX;

/*--------------------------------------------------------------------------}
{	                  ENUMERATED MAILBOX CHANNELS							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	MB_CHANNEL_POWER = 0x0,								// Mailbox Channel 0: Power Management Interface 
	MB_CHANNEL_FB = 0x1,								// Mailbox Channel 1: Frame Buffer
	MB_CHANNEL_VUART = 0x2,								// Mailbox Channel 2: Virtual UART
	MB_CHANNEL_VCHIQ = 0x3,								// Mailbox Channel 3: VCHIQ Interface
	MB_CHANNEL_LEDS = 0x4,								// Mailbox Channel 4: LEDs Interface
	MB_CHANNEL_BUTTONS = 0x5,							// Mailbox Channel 5: Buttons Interface
	MB_CHANNEL_TOUCH = 0x6,								// Mailbox Channel 6: Touchscreen Interface
	MB_CHANNEL_COUNT = 0x7,								// Mailbox Channel 7: Counter
	MB_CHANNEL_TAGS = 0x8,								// Mailbox Channel 8: Tags (ARM to VC)
	MB_CHANNEL_GPU = 0x9,								// Mailbox Channel 9: GPU (VC to ARM)
} MAILBOX_CHANNEL;

/*==========================================================================}
{		     POINTER TO A MAILBOX STRUCT AT MAILBOX_BASE ADDRESS			}
{==========================================================================*/
volatile PI_MAILBOX* MAILBOX = (PI_MAILBOX*)MAILBOX_BASE;

/*==========================================================================}
{		  PUBLIC PI MAILBOX ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/
#define MAIL_EMPTY	0x40000000		/* Mailbox Status Register: Mailbox Empty */
#define MAIL_FULL	0x80000000		/* Mailbox Status Register: Mailbox Full  */

/*-[mailbox_write]----------------------------------------------------------}
. This will execute the sending of the given data block message thru the
. mailbox system on the given channel.
. RETURN: True for success, False for failure.
. 04Jul17 LdB
.--------------------------------------------------------------------------*/
bool mailbox_write (MAILBOX_CHANNEL channel, uint32_t message)
{
	uint32_t value;													// Temporary read value
	if (channel > MB_CHANNEL_GPU)  return false;					// Channel error
	message &= ~(0xF);												// Make sure 4 low channel bits are clear 
	message |= channel;												// OR the channel bits to the value							
	do {
		value = MAILBOX->Status1;									// Read mailbox1 status from GPU	
	} while ((value & MAIL_FULL) != 0);								// Make sure arm mailbox is not full
	MAILBOX->Write1 = message;										// Write value to mailbox
	return true;													// Write success
}

/*-[mailbox_read]-----------------------------------------------------------}
. This will read any pending data on the mailbox system on the given channel.
. RETURN: The read value for success, 0xFEEDDEAD for failure.
. 04Jul17 LdB
.--------------------------------------------------------------------------*/
uint32_t mailbox_read(MAILBOX_CHANNEL channel)
{
	uint32_t value;													// Temporary read value
	if (channel > MB_CHANNEL_GPU)  return 0xFEEDDEAD;				// Channel error
	do {
		do {
			value = MAILBOX->Status0;								// Read mailbox0 status
		} while ((value & MAIL_EMPTY) != 0);						// Wait for data in mailbox
		value = MAILBOX->Read0;										// Read the mailbox	
	} while ((value & 0xF) != channel);								// We have response back
	value &= ~(0xF);												// Lower 4 low channel bits are not part of message
	return value;													// Return the value
}

/*-[mailbox_tag_message]----------------------------------------------------}
. This will post and execute the given variadic data onto the tags channel
. on the mailbox system. You must provide the correct number of response
. uint32_t variables and a pointer to the response buffer. You nominate the
. number of data uint32_t for the call and fill the variadic data in. If you
. do not want the response data back the use NULL for response_buf pointer.
. RETURN: True for success and the response data will be set with data
.         False for failure and the response buffer is untouched.
. 04Jul17 LdB
.--------------------------------------------------------------------------*/
bool mailbox_tag_message (uint32_t* response_buf,					// Pointer to response buffer 
	                      uint8_t data_count,						// Number of uint32_t data following
	                      ...)										// Variadic uint32_t values for call
{
	uint32_t __attribute__((aligned(16))) message[32];
	va_list list;
	va_start(list, data_count);										// Start variadic argument
	message[0] = (data_count + 3) * 4;								// Size of message needed
	message[data_count + 2] = 0;									// Set end pointer to zero
	message[1] = 0;													// Zero response message
	for (int i = 0; i < data_count; i++) {
		message[2 + i] = va_arg(list, uint32_t);					// Fetch next variadic
	}
	va_end(list);													// variadic cleanup								
	mailbox_write(MB_CHANNEL_TAGS, (uintptr_t)&message[0] | 0xC0000000);// Write message address to mailbox adjusted to GPU address
	mailbox_read(MB_CHANNEL_TAGS);									// Wait for write response	
	if (message[1] == 0x80000000) {
		if (response_buf) {											// If buffer NULL used then don't want response
			for (int i = 0; i < data_count; i++)
				response_buf[i] = message[2 + i];					// Transfer out each response message
		}
		return true;												// message success
	}
	return false;													// Message failed
}

void led_on(void); // this function blinks an LED
void delay(void);
extern volatile uint32_t CoreReadyCount;

int main()
{
	volatile uint32_t  *mailbox;
	// Get CORE1 to start at 0x8000 like Core0 you need stack set etc
	// It will start parked by the bootloader at 0x4000009C
	mailbox = (uint32_t*)CORE1_MAILBOX0;
	*mailbox = 0x8000;

	while (CoreReadyCount == 1) {}; // Wait for core ready count to go to 2

	// Get CORE2 to start at 0x8000 like Core0 you need stack set etc
	// It will start parked by the bootloader at 0x400000AC
	mailbox = (uint32_t*)CORE2_MAILBOX0;
	*mailbox = 0x8000;

	while (CoreReadyCount == 2) {}; // Wait for core ready count to go to 3

	// Get CORE3 to start at 0x8000 like Core0 you need stack set etc
	// It will start parked by the bootloader at 0x400000BC
	mailbox = (uint32_t*)CORE3_MAILBOX0;
	*mailbox = 0x8000;

	while (CoreReadyCount == 3) {}; // Wait for core ready count to go to 2

	mailbox = (uint32_t*) CORE1_MAILBOX0;
	*mailbox= (uintptr_t)led_on;

	while(1);
	return 0;
}

void led_on()
{

	while(1)
	{

		mailbox_tag_message(
			0,
			5,
			0x00038041,
			8,
			8,
			130,
			1);   // LED ON
		delay();
		mailbox_tag_message(
			0,
			5,
			0x00038041,
			8,
			8,
			130,
			0);  // LED OFF
		delay();
	}
}

void delay()
{
	unsigned long count=0;
	for (count = 0; count <= 500000; count++) { asm("NOP"); };
}
