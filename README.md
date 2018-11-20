# Bare metal code for HPSC Chiplet

Includes test code for TRCH (Cortex-M4) and RTPS (Cortex-R52).

Also see [readme.html in RTPS](rtps/readme.html) for explanation of startup
assembly code for R52 used by the rest of the RTPS codebase in this repo.

Common code includes drivers and some common library code.

Drivers:

* Driver for ARM MMU-500
* Driver for ARM DMA-330 (credit: based on Linux driver)
* Driver for HPSC Mailbox
* Driver for HPSC Watchdog Timer (WDT)

Library code:

* printf library (credit: Marco Paland, PALANDesign)
* Assert and panic
* Generic interface for interrupt controllers
* A block allocator
* Allocator for dynamic sets of objects in static arrays
* Mailbox link abstraction for communication using a pair of mailboxes
* Simple framework for client-server command processing
