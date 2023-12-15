#ifndef CURSES6502_ARGUMENTS_H
#define CURSES6502_ARGUMENTS_H

extern char* bin_file;
extern int rom_size;
extern int rom_offset;

int arguments_read(int argc, char** argv);

void arguments_free(void);

#endif
