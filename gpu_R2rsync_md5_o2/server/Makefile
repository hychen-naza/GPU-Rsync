CXX = g++
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lstdc++ -lpthread -lcudart -lm -L/usr/local/cuda/lib64 -ldl
PROTOC = protoc
PROTOS_PATH = ./
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`


server : rsync.grpc.pb.o rsync.pb.o ServerFunction.o hash.o checksum.o new_file.o server.o  
	$(CXX) $^ $(LDFLAGS) -o $@

server.o : server.cu ServerFunction.cu hash.cu new_file.cu hash.h type.h new_file.h
	nvcc -std=c++11 -c server.cu -arch=sm_30 -lstdc++ 

ServerFunction.o : ServerFunction.cu hash.cu hash.h
	nvcc -std=c++11 -c ServerFunction.cu -arch=sm_30 -lstdc++ 

hash.o : hash.cu hash.h type.h
	nvcc -c hash.cu -arch=sm_30 -lstdc++ 

new_file.o : new_file.cu hash.cu checksum.cc checksum.h type.h new_file.h
	nvcc -c new_file.cu -arch=sm_30 -lstdc++ 

checksum.o : checksum.cc checksum.h type.h
	$(CXX) -c checksum.cc 

rsync.grpc.pb.o : rsync.grpc.pb.cc rsync.pb.cc
	$(CXX) -std=c++11 -g -I/usr/local/include -pthread  -c -o rsync.grpc.pb.o rsync.grpc.pb.cc

rsync.pb.o : rsync.pb.cc
	$(CXX) -std=c++11 -g -I/usr/local/include -pthread  -c -o rsync.pb.o rsync.pb.cc	

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

clean:
	rm -rf *.o
	rm server
