#include <ncurses.h>
#include <stdlib.h>
#include <sys/param.h>
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

    WINDOW* main_window = initscr();
    timeout(0);
    noecho();
    curs_set(0);

    int width;
    int height;
    getmaxyx(main_window, height, width);

    int middle = width / 2;

    WINDOW* disassembly = newwin(height, middle, 0, 0);
    WINDOW* flags = newwin(5, middle, 0, middle);
    WINDOW* zero_page = newwin(10, middle, 5, middle);
    WINDOW* call_stack = newwin(10, middle, 15, middle);
    WINDOW* memory_viewer = newwin(height - 25, middle, 25, middle);
    refresh();

    scrollok(memory_viewer, TRUE);
    keypad(main_window, TRUE);
    keypad(memory_viewer, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);

    int zero_page_first_line = 0;
    int memory_viewer_first_line = read16(0xFFFC) / 16;

    int c;
    while ((c = getch()) != 'p') {
        cpu_tick();

        if (c == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON4_PRESSED) {
                    if (event.x >= middle && event.y >= 5 && event.y <= 14) {
                        if (zero_page_first_line > 0) {
                            zero_page_first_line--;
                        }
                    }

                    if (event.x >= middle && event.y >= 25) {
                        if (memory_viewer_first_line > 0) {
                            memory_viewer_first_line--;
                        }
                    }
                }

                if (event.bstate & BUTTON5_PRESSED) {
                    if (event.x >= middle && event.y >= 5 && event.y <= 14) {
                        if (zero_page_first_line < 8) {
                            zero_page_first_line++;
                        }
                    }

                    if (event.x >= middle && event.y >= 25) {
                        if (memory_viewer_first_line < (4096 - (height - 25))) {
                            memory_viewer_first_line++;
                        }
                    }
                }
            }
        }

        box(disassembly, 0, 0);
        box(flags, 0, 0);
        box(zero_page, 0, 0);
        box(call_stack, 0, 0);
        box(memory_viewer, 0, 0);

        mvwprintw(disassembly, 0, 2, "Disassembly");
        mvwprintw(flags, 0, 2, "Flags & Registers");
        mvwprintw(flags, 1, 1, "A: %d   ", a);
        mvwprintw(flags, 2, 1, "X: %d   ", x);
        mvwprintw(flags, 3, 1, "Y: %d   ", y);

        mvwprintw(flags, 1, 11, "Stack Pointer: %d     ", sp);
        mvwprintw(flags, 2, 11, "Program Counter: %d     ", pc);
        mvwprintw(flags, 3, 11, "Flags: C=%d, Z=%d, I=%d, D=%d, B=%d, V=%d, N=%d", FLAGSET(FLAG_CARRY), FLAGSET(FLAG_ZERO), FLAGSET(FLAG_INTERRUPT), FLAGSET(FLAG_DECIMAL), FLAGSET(FLAG_BREAK), FLAGSET(FLAG_OVERFLOW), FLAGSET(FLAG_NEGATIVE));

        mvwprintw(zero_page, 0, 2, "Zero-Page");
        mvwprintw(call_stack, 0, 2, "Call Stack");
        mvwprintw(memory_viewer, 0, 2, "Memory Viewer");

        for (int i = 0; i < height - 27; i++) {
            int address = (memory_viewer_first_line + i) * 16;
            mvwprintw(memory_viewer, i + 1, 1, "%04X: ", address);

            for (int j = 0; j < 16; j++) {
                mvwprintw(memory_viewer, i + 1, j + (2 * j) + 8 + (j >= 8 ? 1 : 0), "%02X", cpu_memory[address + j]);
            }
        }

        for (int i = 0; i < 8; i++) {
            int address = (zero_page_first_line + i) * 16;
            mvwprintw(zero_page, i + 1, 1, "%04X: ", address);

            for (int j = 0; j < 16; j++) {
                mvwprintw(zero_page, i + 1, j + (2 * j) + 8 + (j >= 8 ? 1 : 0), "%02X", cpu_memory[address + j]);
            }
        }

        wrefresh(disassembly);
        wrefresh(flags);
        wrefresh(zero_page);
        wrefresh(call_stack);
        wrefresh(memory_viewer);
    }

    endwin();
    return EXIT_SUCCESS;
}
