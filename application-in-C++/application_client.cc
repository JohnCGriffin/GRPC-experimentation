
#include "cpp14.h"

#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

#include <grpc++/grpc++.h>

#include "trade.grpc.pb.h"


using std::shared_ptr;
using std::unique_ptr;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using marketfeed::Trade; 
using ATS = marketfeed::ApplicationTickerSource;


class ExampleClient {

  unique_ptr<ATS::Stub> stub_;

public:

  explicit ExampleClient(shared_ptr<Channel> channel)
    : stub_(ATS::NewStub(channel)) {}


  void fetchMyStuff (auto tickers, long delay, auto tradeCallback) {

    // prepare request message
    marketfeed::FilterRequest req;
    req.set_delay(delay);
    for (auto item : tickers ){
      req.add_tickers(item);
    }

    // connect
    ClientContext context;
    auto reader(stub_->FilteredStream(&context,req));

    // Read trades indefinitely
    Trade trade;
    while (reader->Read(&trade)){
      tradeCallback(trade);
    }

    if (!reader->Finish().ok()){
      throw std::runtime_error("RPC Failure");
    }
  }

};



static auto split(auto s){
  using Iter = std::istream_iterator<std::string>;
  std::istringstream iss(s);
  return std::vector<std::string> (Iter(iss), Iter());
}


int main() {

  extern void update_display(Trade);

  auto tickers = split("AAPL AMZN AXP BA CSCO DD DIS F GE GS HD IBM INTC JNJ "
		       "KO LMT MCD MMM MRK MSFT PFS PG SYY T TRV V WMT XOM");

  auto ch = grpc::CreateChannel("localhost:50052", grpc::InsecureChannelCredentials());

  ExampleClient client(ch);

  client.fetchMyStuff(tickers, 2000, update_display);

  return 0;
}
