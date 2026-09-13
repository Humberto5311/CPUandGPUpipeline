# Makefile para compilar o motor gráfico como uma biblioteca dinâmica (.so) no Linux
CXX = g++
CXXFLAGS = -O3 -std=c++11 -fPIC -Wall
LIBS = -lOpenCL

TARGET = libmotor_grafico.so
SRCS = processamento.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -shared -o $(TARGET) $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET)
