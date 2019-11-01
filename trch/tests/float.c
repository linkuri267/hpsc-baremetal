#include <stdbool.h>

#include "console.h"

#include "test.h"

static float gc;

static void enable_fpu()
{
    asm("LDR R0, =0xE000ED88");
    // Read CPACR
    asm("LDR R1, [R0]");
    // Set bits 20-23 to enable CP10 and CP11 coprocessors 
    asm("LDR R2, =0x00F00000");
    asm("ORR R1, R1, R2");
    // Write back the modified value to the CPACR
    asm("STR R1, [R0]");  // wait for store to complete
    asm("DSB");
    //reset pipeline now the FPU is enabled ISB
    asm("ISB");
}

static void disable_fpu()
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

static float calculate(float a, float b) {

    if (a == 1.5) printf("argument is OK\r\n");
    else printf("argument is NOT OK\r\n");
    if (b == 3.0) printf("argument is OK\r\n");
    else printf("argument is NOT OK\r\n"); 
    float c = (a + b)/ b;

    if ((a+b)/b == (1.5 + 3.0)/3.0) printf("internal calculation is correct\r\n");
    else printf("NO: internal calculation is NOT correct\r\n");
    gc = (a + b) /b;
    if (c != gc) printf("Error\r\n");
    else printf("gc and (a+b)/b are the same\r\n");
    asm("svc #0"); 
    printf("about to return\r\n");
    return (a + b)/ b;
}

int test_float()
{
    int rc = 0;
    float a, b, c;

    enable_fpu();

    a = 1.5;
    b = 3.0;
    c = calculate(a,b);

    if (c == gc) {
        printf("return value is OK\r\n");
    } else {
        printf("NO: return value is NOT OK\r\n");
        rc = 1;
    }

    if (c == (a+b)/b) {
        printf("Equal\r\n");
    } else {
        printf("Not Equal\r\n");
        rc = 1;
    }

    printf("%f = (%f + %f) / %f\r\n",
           FLOAT_ARG(c), FLOAT_ARG(a),
           FLOAT_ARG(b), FLOAT_ARG(b));

    disable_fpu();
    return rc;
}
