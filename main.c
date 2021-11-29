#include "smallsh.h"

int main() {
    while (userin() != EOF) procline();
    return 0;
}