#ifndef COMMAND_H
#define COMMAND_H

// Command field length is limited to 4-bits right now
#define CMD_RESET_HPPS 0x1

void cmd_handle(unsigned cmd);
void cmd_reset_hpps();

#endif // COMMAND_H
