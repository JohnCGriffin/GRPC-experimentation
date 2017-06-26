
CXXFLAGS=--std=c++14 -I ../include


application_client: application_client.cc application_client_display.cc trade.pb.o trade.grpc.pb.o trade.pb.h
	g++ $(CXXFLAGS) -I . application_client.cc application_client_display.cc trade.pb.o trade.grpc.pb.o \
		`pkg-config --libs protobuf grpc++ grpc` \
		-lncurses \
		-Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl -o $@




trade.pb.h: ../trade.proto
	protoc -I ..  --cpp_out=. ../trade.proto

trade.pb.o: trade.pb.h
	g++ $(CXXFLAGS) -I . -c  trade.pb.cc && rm trade.pb.cc



trade.grpc.pb.h: ../trade.proto trade.pb.h
	protoc -I .. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../trade.proto

trade.grpc.pb.o: trade.grpc.pb.h
	g++ `pkg-config --cflags protobuf grpc`  -c -o trade.grpc.pb.o trade.grpc.pb.cc



clean:
	rm -f trade.*pb.* application_client
