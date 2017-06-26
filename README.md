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

Finally, a sample application requests a slowly updating and filtered stream from the GO process via GRPC.  In this case, it's a C++ application using ncurses.

### Learned

1. In C++, compile the generated .cc files and throw them away; just keep headers.
2. To terminate a long incoming stream from a client, use TryCancel().
3. Even if an RPC service takes something simple like a number, or even nothing at all, you need a proto message 
    for it.  Notice that ``ConnectFirehose`` requires an empty VoidMessage.
4. The generated GO code is more lucid than the C++, but runs 50% slower than C++.
    
### Still Baffled

1. Should I be using version 2 or 3?  I chose 3.
2. The GO implementation of GRPC seems to be disconnected from the other supported languages and the generate code indicates that it's using Version 2 protocol buffers, even with 3 specified.  It works nonetheless.






