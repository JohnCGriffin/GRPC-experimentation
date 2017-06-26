package main

import (
	"context"
	"fmt"
	"io"
	"net"
	"sync"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/grpclog"

	marketfeed ".."
)

var lastTrades = make(map[string]*marketfeed.Trade)
var mtx sync.RWMutex

// used by processFireHose
func updateLastTrade(trade *marketfeed.Trade) {
	mtx.Lock()
	defer mtx.Unlock()

	lastTrades[trade.Ticker] = trade
}

// used by clients of this server
func getUpdates(tickers []string, lastTransction int64) []*marketfeed.Trade {

	result := make([]*marketfeed.Trade, 0, len(tickers))

	mtx.RLock()
	defer mtx.RUnlock()

	for _, key := range tickers {
		t, ok := lastTrades[key]
		if ok && t.Transaction > lastTransction {
			result = append(result, t)
		}
	}

	return result
}

func processFirehoseInBackground() {
	conn, err := grpc.Dial("127.0.0.1:50051", grpc.WithInsecure())
	if err != nil {
		grpclog.Fatalf("fail to dial: %v", err)
	}

	client := marketfeed.NewTickerSourceClient(conn)

	stream, err := client.ConnectFirehose(context.Background(), &marketfeed.VoidMessage{})

	if err != nil {
		panic(err)
	}

	go func() {

		defer conn.Close()

		for {
			trade, err := stream.Recv()
			if err == io.EOF {
				break
			}
			if err != nil {
				grpclog.Fatal(err)
			}
			updateLastTrade(trade)
		}

	}()

}

type appTickerSource struct{}

func (ats *appTickerSource) FilteredStream(req *marketfeed.FilterRequest, stream marketfeed.ApplicationTickerSource_FilteredStreamServer) error {
	fmt.Println("Filtering Server: client connected")
	lastTransaction := marketfeed.Trade{}.Timestamp
	var err error
	for err == nil {
		for _, t := range getUpdates(req.Tickers, lastTransaction) {
			if t.Transaction > lastTransaction {
				lastTransaction = t.Transaction
			}
			err = stream.Send(t)
		}
		if err == nil {
			time.Sleep(time.Duration(req.Delay) * time.Millisecond)
		}
	}
	fmt.Println("Filtering Server: client disconnected")
	return err
}

func main() {
	// connect to feed
	processFirehoseInBackground()

	lis, err := net.Listen("tcp", "127.0.0.1:50052")
	if err != nil {
		panic(err)
	}

	grpcServer := grpc.NewServer()
	marketfeed.RegisterApplicationTickerSourceServer(grpcServer, new(appTickerSource))
	grpcServer.Serve(lis)
}
