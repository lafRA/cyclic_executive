DEBUG=-DNDEBUG

#scommentare questa riga per abilitare il informazioni di debug
#DEBUG=-g

FLAGS=-Wall
LINK=-lpthread -lrt
BUILD=gcc $(FLAGS) $(DEBUG) $^ -o $@ $(LINK)
COMPILE=gcc $(FLAGS) $(DEBUG) -c $< -o $@

.c.o:
	$(COMPILE)

all: task-ok task-not-ok

task-ok: executive.o task-ok.o
	$(BUILD)

task-not-ok: executive.o task-not-ok.o
	$(BUILD)

executive.o: executive.c task.h

task-ok.o: task-ok.c task.h

task-not-ok.o: task-not-ok.c task.h

clean:
	rm task-ok.o task-not-ok.o executive.o task-ok task-not-ok *~
