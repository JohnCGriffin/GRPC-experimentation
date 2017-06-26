
#include "cpp14.h"


// Test client of firehose. 


#include <iostream>

#include <grpc++/grpc++.h>

#include "trade.grpc.pb.h"

namespace hydrant {

  using std::shared_ptr;
  using std::unique_ptr;

  using grpc::Channel;
  using grpc::ClientContext;
  using grpc::ClientReader;
  using grpc::Status;

  using marketfeed::Trade; 
  using marketfeed::TickerSource;


  class Client {
    std::unique_ptr<TickerSource::Stub> stub_;
  
  public:
    Client(shared_ptr<Channel> channel)
      : stub_(TickerSource::NewStub(channel)) {}


    template <typename F>
    void attachFirehose (F callback) {

      ClientContext context;
      marketfeed::VoidMessage request;

      unique_ptr<ClientReader<Trade>> reader(stub_->ConnectFirehose(&context,request));

      Trade t;
      while (reader->Read(&t) && callback(t)) ;
      context.TryCancel();
    }
  };
}


int main(){

  auto ch = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

  hydrant::Client client(ch);

  int count=0;

  client.attachFirehose([&](marketfeed::Trade t){
      std::cout << t.timestamp() << " "
		<< t.transaction() << " "
		<< t.ticker() << " "
		<< t.price() << std::endl;
      return count++ < 20;
    });

  return 0;
}
