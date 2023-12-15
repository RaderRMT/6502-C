#include <ncurses.h>
#include <stdlib.h>
#include "arguments.h"
#include "cpu.h"

void load_bin(void) {
    FILE* file = fopen(bin_file, "r");
    fread(cpu_memory + rom_offset, rom_size, 1, file);
    fclose(file);
}

int main(int argc, char** argv) {
    if (arguments_read(argc, argv)) {
        arguments_free();
        return EXIT_SUCCESS;
    }

    load_bin();

    cpu_reset();

    initscr();
    timeout(0);
    noecho();

    int paused = 1;
    int single_step = 0;
    while (1) {
        int c = getch();
        if (c == 27) {
            break;
        }

        if (c == 'a') {
            single_step = !single_step;
        }

        if (c == 'p') {
            paused = !paused;
        }

        if (c == 'r') {
            cpu_reset();
        }

        if (!paused) {
            if (single_step) {
                if (c == 's') {
                    cpu_tick();
                }

                if (c == 'n') {
                    cpu_next_instruction();
                }
            } else {
                cpu_tick();
            }
        }

        mvprintw(0, 0, "a: %d  ", a);
        mvprintw(1, 0, "x: %d  ", x);
        mvprintw(2, 0, "y: %d  ", y);
        mvprintw(3, 0, "pc: %d    ", pc);
        mvprintw(4, 0, "Flags: C=%d, Z=%d, I=%d, D=%d, B=%d, V=%d, N=%d", FLAGSET(FLAG_CARRY), FLAGSET(FLAG_ZERO), FLAGSET(FLAG_INTERRUPT), FLAGSET(FLAG_DECIMAL), FLAGSET(FLAG_BREAK), FLAGSET(FLAG_OVERFLOW), FLAGSET(FLAG_NEGATIVE));
        mvprintw(5, 0, "Current instruction: 0x%02x", instruction);
        mvprintw(6, 0, "Single step: %d", single_step);

        refresh();
    }

    endwin();
    return EXIT_SUCCESS;
}
