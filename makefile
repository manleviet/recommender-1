all:handledata data2bin handlequery
	echo "all made"
handledata:handledata.o -lm
	g++ handledata.cpp  -O3 -o handledata
data2bin:data2bin.o 
	g++ data2bin.cpp  -O3 -o data2bin
handlequery:handlequery.o
	g++ handlequery.cpp -O3 -o handlequery
clean:
	rm *.o
