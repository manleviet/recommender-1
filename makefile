handledata:handledata.o -lm
	g++ handledata.cpp  -O3 -o handledata
all:handledata 
	echo "all made"
clean:
	rm *.o
