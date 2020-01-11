CXX = g++
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl
PROTOC = protoc
PROTOS_PATH = ./
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

server : rsync.grpc.pb.o rsync.pb.o server.o checksum.o hashtable.o 
	$(CXX) $^ $(LDFLAGS) -o $@

server.o : server.cc checksum.cc hashtable.cc checksum.h hashtable.h type.h
	$(CXX) -c -std=c++11 server.cc

checksum.o : checksum.cc checksum.h type.h
	$(CXX) -c checksum.cc 

hashtable.o : hashtable.cc hashtable.h type.h
	$(CXX) -c hashtable.cc

rsync.grpc.pb.o : rsync.grpc.pb.cc rsync.pb.cc
	$(CXX) -std=c++11 -g -I/usr/local/include -pthread  -c -o rsync.grpc.pb.o rsync.grpc.pb.cc

rsync.pb.o : rsync.pb.cc
	$(CXX) -std=c++11 -g -I/usr/local/include -pthread  -c -o rsync.pb.o rsync.pb.cc

%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<


clean:
	rm -rf *.o
	rm server
