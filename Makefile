DEBUG = -g

myar: myar.o
	gcc $(DEBUG) -o myar myar.o
myar.o: myar.c
	gcc $(DEBUG) -c myar.c