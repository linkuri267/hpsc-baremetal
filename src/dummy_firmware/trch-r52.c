#include <stdint.h>
#include <stdbool.h>

#include <libmspprintf/printf.h>

#include "uart.h"
#include "nvic.h"
#include "mailbox.h"
#include "reset.h"
#include "command.h"

inline void disable_fpu()
{
    /* Disable FPU: Code from M4 TRM */
    // CPACR is located at address 0xE000ED88 LDR.W R0, =0xE000ED88
    asm("LDR R0, =0xE000ED88");
    // Read CPACR
    asm("LDR R1, [R0]");
    // Set bits 20-23 to enable CP10 and CP11 coprocessors 
    asm("LDR R2, =0xFF0FFFFF");
    asm("AND R1, R1, R2");
    // Write back the modified value to the CPACR
    asm("STR R1, [R0]");  // wait for store to complete
    asm("DSB");
    //reset pipeline now the FPU is enabled ISB
    asm("ISB");
}

float gc;
float calculate(float a, float b) {

   if (a == 1.5) printf("argument is OK\n");
   else printf("argument is NOT OK\n");
   if (b == 3.0) printf("argument is OK\n");
   else printf("argument is NOT OK\n"); 
   float c = (a + b)/ b;

/*    asm("LDR R0, =0xE000ED88");
    // Read CPACR
    asm("LDR R1, [R0]");
    // Set bits 20-23 to enable CP10 and CP11 coprocessors 
    asm("LDR R2, =0xFF0FFFFF");
    asm("AND R1, R1, R2");
    // Write back the modified value to the CPACR
    asm("STR R1, [R0]");  // wait for store to complete
    asm("DSB");
    //reset pipeline now the FPU is enabled ISB
    asm("ISB");
*/
   if ((a+b)/b == (1.5 + 3.0)/3.0) printf("internal calculation is correct\n");
   else printf("NO: internal calculation is NOT correct\n");
   gc = (a + b) /b;
   if (c != gc) printf("Error\n");
   else printf("gc and (a+b)/b are the same\n");
   asm("svc #0"); 
   printf("about to return\n");
   return (a + b)/ b;
}

int notmain ( void )
{

#ifdef M4_UART
    cdns_uart_startup(); // init UART peripheral
    printf("TRCH\n");

    /* TODO: This doesn't make a difference: access succeeds without this */
    printf("PRV: svc #0\n");
//    asm("svc #0");

float a, b, c;
    a = 1.5;
    b = 3.0;
    c = calculate(a,b);
    if (c == gc) printf("return value is OK\n");
    else printf("NO: return value is NOT OK\n");
    if (c == (a+b)/b) printf("Equal\n");
    else printf("Not Equal\n");

    printf("%f = (%f + %f) / %f\n", c, a, b, b);

    nvic_int_enable(MBOX_HAVE_DATA_IRQ);
#endif
    reset_r52(/* first_boot */ true);
    mbox_init();
    /* after some delay */
    int total = 0;
    int i, j;
    for (i = 0; i < 1000000; i++) {
      for (j = 0; j < 1000; j++) {
        total++;
      }
    }
    printf("M4: is the delay long enough?\n");
    /* trigger interrupt to R52 */

    printf("M4: send message to R52\n");
    for (i = 0; i < 15; i++) 
       mbox_send(i);

//    uint32_t * addr = (unsigned int *) 0xff310000; /* IPI_TRIG */
    uint32_t val = (1 << 8); 	/* RPU0 */
    * ((uint32_t *) 0xff300000)= val;
    * ((uint32_t *) 0xff310000)= val;
    * ((uint32_t *) 0xff320000)= val;
    * ((uint32_t *) 0xff380000)= val;
    printf("M4: after trigger interrupt to R52\n");
   
    while (1) {
        asm("wfi");
    }
}
