#ifndef COMMANDS_H
#define COMMANDS_H

void cmd_list(int fd);
void cmd_get (int fd, int rec_no);
void cmd_put (int fd, int rec_no);
void cmd_add (int fd);

#endif