all: Thread_Cligne_10_Hz_time
Thread_Cligne_10_Hz_time: Thread_Cligne_10_Hz_time.c
	gcc -o lab1 Thread_Cligne_10_Hz_time.c -lbcm2835 -pthread
clean:
	rm -f lab1
