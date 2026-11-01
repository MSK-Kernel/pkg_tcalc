#include <stdint.h>
#include <stddef.h>
#include "tanja.h"
#include "utf8.h"
#define TCALC_LINE_MAX 128

static char line[TCALC_LINE_MAX];
static const char *expr;
static int parse_error;
static int divide_by_zero;

static void tcalc_read_line(char *buf, int cap) {
    int n = 0;
    if (cap <= 0) return;
    for (;;) {
        int key = get_key();
        if (key == KEY_ENTER || key == '\n' || key == '\r') {
            console_putc('\n');
            break;
        }
        if (key == KEY_BACKSPACE || key == '\b') {
            if (n > 0) {
                --n;
                console_putc('\b');
                console_putc(' ');
                console_putc('\b');
            }
            continue;
        }
        if (key >= 32 && key <= 126 && n < cap - 1) {
            buf[n++] = (char)key;
            console_putc((char)key);
        }
    }
    buf[n] = '\0';
}

static void skip_spaces(void) {
    while (*expr == ' ' || *expr == '\t') ++expr;
}

static int parse_sum(void);

static int parse_number(void) {
    int value = 0;
    skip_spaces();
    if (*expr == '+') {
        ++expr;
        return parse_number();
    }
    if (*expr == '-') {
        ++expr;
        return -parse_number();
    }
    if (*expr == '(') {
        ++expr;
        value = parse_sum();
        skip_spaces();
        if (*expr != ')') parse_error = 1;
        else ++expr;
        return value;
    }
    if (*expr < '0' || *expr > '9') {
        parse_error = 1;
        return 0;
    }
    while (*expr >= '0' && *expr <= '9') {
        value = value * 10 + (*expr - '0');
        ++expr;
    }
    return value;
}

static int parse_product(void) {
    int value = parse_number();
    for (;;) {
        int rhs;
        char op;
        skip_spaces();
        op = *expr;
        if (op != '*' && op != '/' && op != '%') break;
        ++expr;
        rhs = parse_number();
        if (op == '*') value *= rhs;
        else if (rhs == 0) {
            divide_by_zero = 1;
            parse_error = 1;
            value = 0;
        } else if (op == '/') value /= rhs;
        else value %= rhs;
    }
    return value;
}

static int parse_sum(void) {
    int value = parse_product();
    for (;;) {
        int rhs;
        char op;
        skip_spaces();
        op = *expr;
        if (op != '+' && op != '-') break;
        ++expr;
        rhs = parse_product();
        if (op == '+') value += rhs;
        else value -= rhs;
    }
    return value;
}

static void print_signed(int value) {
    uint32_t magnitude;
    if (value < 0) {
        console_putc('-');
        magnitude = 0u - (uint32_t)value;
    } else {
        magnitude = (uint32_t)value;
    }
    print_dec(magnitude);
}

static void evaluate_line(void) {
    int result;
    expr = line;
    parse_error = 0;
    divide_by_zero = 0;
    result = parse_sum();
    skip_spaces();
    if (*expr != '\0') parse_error = 1;
    if (parse_error) {
        if (divide_by_zero) print("Error: division or modulo by zero.\n");
        else print("Error: invalid expression. Try numbers, + - * / %, and parentheses.\n");
        return;
    }
    print_signed(result);
    print("\n");
}

void main(char *args) {
    (void)args;
    print("\nTCalc 2026.11.01\n");
    print("(C) Copyright 2026 The TanjaOS Project\n");
    print("Enter an equation using + - * / %, and parentheses.\n");
    print("Type 'help' for help, or 'exit' and 'quit' to exit.\n\n");
    for (;;) {
        print("tcalc> ");
        tcalc_read_line(line, TCALC_LINE_MAX);
        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            break;
        }
        if (strcmp(line, "help") == 0) {
            print("Examples: 2 + 3, 10 / 2, (4 + 5) * 3\n");
            print("Operators: + - * / % (integer arithmetic; * and / first)\n");
            print("Commands: help, quit\n");
            continue;
        }
        if (line[0] == '\0') continue;
        evaluate_line();
    }
}
