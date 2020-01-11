CXX = g++
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl
PROTOC = protoc
PROTOS_PATH = ./
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`



sync_client : sync_client.o checksum.o rsync.grpc.pb.o rsync.pb.o
	$(CXX) $^ $(LDFLAGS) -o $@

sync_client.o : sync_client.cc checksum.cc checksum.h
	$(CXX) -c -std=c++11 sync_client.cc

checksum.o : checksum.cc checksum.h
	$(CXX) -c checksum.cc

rsync.grpc.pb.o : rsync.grpc.pb.cc
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



