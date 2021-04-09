CFLAGS= -std=c89


libconnection.so: ./connection_library.o ./queue.o ./linked_list.o ./library.o 
	$(CC) -o $@ $^ -shared


libconnection.a: ./connection_library.o ./queue.o ./linked_list.o ./library.o 
	$(AR) crv $@ $?

#Flags: CFLAGS= -std=c89 #-pedantic -Wall -Wextra 

clean:
	rm -f *.so *.o *.bak *~ *.tmp *.a