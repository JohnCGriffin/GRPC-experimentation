

filter-server/filter-server: filter-server/main.go go-stuff
	cd filter-server && go build

go-stuff: ../trade.proto
	protoc -I .. ../trade.proto --go_out=plugins=grpc:.

clean:
	rm -f filter-server/filter-server trade.pb.go

