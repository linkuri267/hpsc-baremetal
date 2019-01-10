// This exists because we don't have an ability to dynamically register the
// ISRs (i.e. set the function pointer in the vector table) at time of when the
// driver creates an instance for the device. Instances may be created and
// destroyed independently by different parts of the application, e.g. by a
// standalone test or by the main loop -- but without dynamic ISR registration
// all users need to go through the same ISR function, which implies that all
// users must initialize the same pointer to the instance they create. We hold
// that pointer and the ISR functions here.

#if TEST_RTI_TIMER
#include "rti-timer.h"
struct rti_timer *trch_rti_timer;
void rti_timer_trch_isr() { rti_timer_isr(trch_rti_timer); };
#endif // TEST_RTI_TIMER
