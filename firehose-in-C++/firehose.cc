
#include "cpp14.h"


#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <condition_variable>

#include <grpc++/grpc++.h>

#include "trade.grpc.pb.h"

using namespace std;
using namespace std::chrono;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::InsecureServerCredentials;

using marketfeed::Trade;
using marketfeed::VoidMessage;



// Encapsulation of trade updating and client notification
static class {

  condition_variable cv;
  mutex cv_m;

  long _transaction = 0;
  unordered_map<string,Trade> trades;

public:

  // Called by worker threads
  long transaction(){
    unique_lock<mutex> lk(cv_m);
    return _transaction;
  }

  // Called by worker threads 
  auto fetchUpdates(long lastTransaction){
    
    vector<Trade> result;

    {
      unique_lock<mutex> lk(cv_m);
      cv.wait(lk);
      
      // if not spurious wakeup
      if (_transaction != lastTransaction){
	for (auto &kv : trades){
	  if(kv.second.transaction() > lastTransaction){
	    result.push_back(kv.second);
	  }
	}
      }
    }
    
    sort(result.begin(),
	 result.end(),
	 [](Trade& a, Trade& b){
	   auto diff = a.timestamp() - b.timestamp();
	   return diff ? (diff < 0) : (a.ticker() < b.ticker());
	 });
    
    return result;
  }

  // Called by master_thread
  void update(marketfeed::Trade trd ){
    {
      unique_lock<mutex> lk(cv_m);

      auto tck		= trd.ticker();
      auto old		= trades[tck];
      auto open		= old.open() ? old.open() : trd.price();
      auto diff		= trd.price() - old.price();
      auto direction	= (diff < 0) ? -1 : (diff > 0) ? 1 : 0;

      trd.set_transaction(++_transaction);
      trd.set_direction(direction);
      trd.set_open(open);
      
      trades[tck] = trd;
    }

    // This is incorrect, but grouping by 10 millis drops the load a lot
    {
      static long lastNotifyTimestamp;
      // the std::abs protects against a wonky trade timestamp
      if (abs(trd.timestamp() - lastNotifyTimestamp) > 10){
	cv.notify_all();
	lastNotifyTimestamp = trd.timestamp();
      }
    }
  }
  
} LastTrades;






class TickerSourceServiceImpl final : public marketfeed::TickerSource::Service {
 
  Status ConnectFirehose(ServerContext*,
			 const VoidMessage*,
			 ServerWriter<Trade>* writer) final override
  {
    cout << "Firehose Server: client connected" << endl;

    auto writeResult = true;
    while (writeResult) {
      auto lastTransaction = LastTrades.transaction();
      auto newTrades = LastTrades.fetchUpdates(lastTransaction);
      for (auto t : newTrades){
	writeResult = writer->Write(t);
      }
    }
    
    cout << "Firehose Server: client disconnected" << endl;
    return Status::OK;
  }

};


// This reads raw data an initiates distribution
// to clients via LastTrades.update()

void process_original_data_source() {

  auto current_millis = [](){
    auto since_epoch = system_clock::now().time_since_epoch();
    return duration_cast<milliseconds>(since_epoch).count();
  };

  auto ms_base = current_millis();

  ifstream ifs("the-data.txt");
  if (!ifs){
    return void(cerr << "failed to open data source" << endl);
  }

  string ticker;
  decltype(ms_base) ms;
  double price;
  
  while (ifs >> ticker >> ms >> price){

    auto adjusted_ms = ms + ms_base;
    auto ms_to_wait = adjusted_ms - current_millis();
    
    if (ms_to_wait > 0){
      this_thread::sleep_for(milliseconds(ms_to_wait));
    }
    
    marketfeed::Trade trd;
    trd.set_ticker(ticker);
    trd.set_timestamp(adjusted_ms);
    trd.set_price(price);
    
    LastTrades.update(trd);
  }

}
  


int main()
{
  TickerSourceServiceImpl service;

  auto server = ServerBuilder()
    .AddListeningPort("localhost:50051", InsecureServerCredentials())
    .RegisterService(&service)
    .BuildAndStart();
			
  thread master(process_original_data_source);

  cout << "FireHose Server started" << endl;

  server->Wait();
    
  return 0;
}

