
CXXFLAGS=--std=c++14 -I ../include

TARGETS=firehose firehose_client 

start: $(TARGETS) 
	@cat /dev/null


firehose: firehose.cc trade.pb.o trade.grpc.pb.o trade.pb.h
	g++ $(CXXFLAGS) -I . firehose.cc trade.pb.o trade.grpc.pb.o \
		`pkg-config --libs protobuf grpc++ grpc` \
		-Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o $@

firehose_client: firehose_client_main.cc trade.pb.o trade.grpc.pb.o trade.pb.h
	g++ $(CXXFLAGS) -I . firehose_client_main.cc trade.pb.o trade.grpc.pb.o \
		`pkg-config --libs protobuf grpc++ grpc` \
		-Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o $@




trade.pb.h: ../trade.proto
	protoc -I .. --cpp_out=. ../trade.proto

trade.pb.o: trade.pb.h
	g++ $(CXXFLAGS) -I . -c  trade.pb.cc && rm trade.pb.cc




trade.grpc.pb.h: ../trade.proto trade.pb.h
	protoc -I .. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../trade.proto

trade.grpc.pb.o: trade.grpc.pb.h
	g++ `pkg-config --cflags protobuf grpc`  -c -o trade.grpc.pb.o trade.grpc.pb.cc



clean:
	rm -f trade.*pb.* *.o $(TARGETS) 
