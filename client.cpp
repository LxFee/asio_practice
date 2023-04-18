//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdio>
#include <iostream>
#include <memory>
#include <algorithm>
#include "utils.hpp"
#include "asio.hpp"

using asio::ip::tcp;

class tcping : public std::enable_shared_from_this<tcping> {
public:
  tcping(asio::io_context& io_context, tcp::endpoint endpoint) : socket_(io_context), endpoint_(endpoint), timer(io_context) {
    active++;
    total++;
  }
  ~tcping() {
    active--;
    dead++;
  }
  void doping() {
    auto self = shared_from_this();
    util::Clock clock;
    clock.start();
    socket_.async_connect(endpoint_, [this, self, clock](std::error_code ec) {
      if(ec) {
        pings[endpoint_.port()] = -1;
        rest_times--;
        if(rest_times <= 0) return ;
      } else {
        pings[endpoint_.port()] = clock.peek();
        rest_times = max_retry_times;
      }
      socket_.close();
      timer.expires_from_now(std::chrono::seconds(1));
      timer.async_wait([this, self](std::error_code ec) {
        doping();
      });
    });
  }

private:
  friend class tcping_helper;
  static int64_t pings[1 << 16];
  static int rank[1 << 16];
  static int active;
  static int dead;
  static int total;

  enum {max_retry_times = 1};
  int rest_times = max_retry_times;
  bool timer_timeout = false;

  tcp::socket socket_;
  tcp::endpoint endpoint_;
  asio::steady_timer timer;
};

int64_t tcping::pings[1 << 16];
int tcping::rank[1 << 16];
int tcping::active;
int tcping::dead;
int tcping::total;


class tcping_helper : public std::enable_shared_from_this<tcping_helper> {
public:
  tcping_helper(asio::io_context& io_context, asio::ip::address address, int bport, int eport) 
    : io_context_(io_context), address_(address), timer_(io_context), bport_(bport), eport_(eport) {
      if(eport_ >= (1 << 16)) eport_ = (1 << 16) - 1;
      if(bport_ < 0) bport_ = 0;
      if(eport_ < bport_) eport_ = bport_;
      port = bport_;

      tcping::active = 0;
      memset(tcping::pings, -1, sizeof tcping::pings);
      for(int i = 0; i < (1 << 16); i++) tcping::rank[i] = i;
    }
  
  void doit() {
    auto self = shared_from_this();
    timer_.async_wait([this, self](std::error_code ec){
      while(port <= eport_ && tcping::active < max_active_tcping) {
        std::make_shared<tcping>(io_context_, tcp::endpoint(address_, port))->doping();
        port++;
      }
      
      print();

      timer_.expires_from_now(std::chrono::seconds(1));
      timer_.async_wait([this,self](std::error_code ec){
        doit();
      });
    });
  }

private:
  void print() {
    std::sort(tcping::rank, tcping::rank + (1 << 16), [](int p1, int p2){
      if(tcping::pings[p1] < 0) return false;
      if(tcping::pings[p2] < 0) return true;
      return tcping::pings[p1] < tcping::pings[p2];
    });
    std::cout << "\033[2J\033[2H";
    std::cout << "ip: " << address_ << "\n"; 
    std::cout << "range: " << bport_ << "-" << eport_ << "(" << eport_ - bport_ + 1 << ")" << "\n";
    std::cout << "total: " << tcping::total << "\n";
    std::cout << "dead: " << tcping::dead << "\n";
    std::cout << "live: " << tcping::active << "\n"; 

    for(int i = 0; i < (1 << 16); i++) {
      if(tcping::pings[tcping::rank[i]] < 0) continue;
      std::cout << tcping::rank[i] << ": " << tcping::pings[tcping::rank[i]] << "ms\n";
    }
    std::cout.flush();
  }
  
  enum {max_active_tcping = 5000};
  asio::io_context& io_context_;
  asio::ip::address address_;
  asio::steady_timer timer_;
  int bport_, eport_, port;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2) {
      std::cerr << "Usage: scan_port <host> [port[:port]]\n";
      return 1;
    }
    int bport = 1, eport = 65535;
    if (argc == 3) {
      auto ports = std::string_view(argv[2]);
      auto sp = ports.find(':');
      if(sp != std::string_view::npos) {
        bport = atoi(ports.substr(0, sp).data());
        eport = atoi(ports.substr(sp + 1).data());
      } else {
        bport = eport = atoi(argv[2]);
      }
    }

    asio::io_context io_context;
    tcp::resolver resolver(io_context);
    auto result = resolver.resolve(argv[1], "0");
    asio::ip::address address;
    if(result.size() > 1) {
      int items = 0;
      for(auto it = result.begin(); it != result.end(); it++) {
        std::cout << ++items << "\t" << it->endpoint().address() << "\n";
      }
      std::cout.flush();
      int id;
      while(1) {
        std::cout << "choose [1-" << items << "]: ";
        std::cin >> id;
        if(id >= 1 && id <= items) break;
      }
      auto it = result.begin();
      while(id - 1 > 0) {
        it++;
        id--;
      }
      address = it->endpoint().address();
    } else {
      address = result.begin()->endpoint().address();
    }
    std::make_shared<tcping_helper>(io_context, address, bport, eport)->doit();

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}