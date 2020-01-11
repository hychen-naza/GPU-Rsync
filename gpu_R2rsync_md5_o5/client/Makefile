CXX = g++
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl
PROTOC = protoc
PROTOS_PATH = ./
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
CLIENT_TYPE = sync_client


$(CLIENT_TYPE) : rsync.grpc.pb.o rsync.pb.o $(CLIENT_TYPE).o checksum.o
	$(CXX) $^ $(LDFLAGS) -o $@

$(CLIENT_TYPE).o : $(CLIENT_TYPE).cc checksum.cc checksum.h
	$(CXX) -c -std=c++11 $(CLIENT_TYPE).cc

checksum.o : checksum.cc checksum.h
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
	rm client



