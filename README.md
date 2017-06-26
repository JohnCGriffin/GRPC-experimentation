## Go and C++ experimentation with GRPC and Protocol Buffers

This project uses version 3 protocol buffers under GRPC to effect all communication between two servers and one client.
The three pieces make a mock stock market display using a couple of hours of ticker data from one day in 2013. 

![mock stock market display](/images/ZestyPic.png)

The screen above is split with tmux.  The bottom pane shows the first server.  It takes a single source of data in high quantity, i.e. a trade per millisecond, and offers it up to any number of clients as a GRPC stream.  Because the stream is heavy, it's called "firehose."  In the protocol file below, see the RPC service method ``ConnectFirehose``.


```
syntax = "proto3";

package marketfeed;

message Trade {
  string ticker = 1;
  int64 timestamp = 2;
  double price = 3;
  int64 transaction = 4;
  double open = 5;
  sint32 direction = 6; // -1, 0, 1
}

message VoidMessage {}

message FilterRequest {
  int64 delay = 1;
  repeated string tickers = 2;
}


service TickerSource {
  rpc ConnectFirehose (VoidMessage) returns (stream Trade){}
  
}

service ApplicationTickerSource {
  rpc FilteredStream (FilterRequest) returns (stream Trade){}
}
```

While the firehose supports multiple clients, only one really connects.  The middle piece is a GO program that reads the firehose via GRPC stream in a GO routine, updating a map of ticker information.  It also is a server for clients that want throttled and coalesced trades, filtered by only those tickers given in request.  That service is also via GRPC streams. That service is shown above as ``ApplicationTickerSource``.

Finally, a sample application request a slowly updating and filtered stream from the GO process via GRPC.  In this case, it's a C++ application using ncurses.

# Easy?

Honestly, yes and no.  It's easy after it's done.  Hopefully somebody will benefit from the example.






