all: Cligne_1_Hz
Cligne_1_Hz: Cligne_1_Hz.c
        gcc -o lab1 Cligne_1_Hz.c -lbcm2835
clean:
        rm -f lab1