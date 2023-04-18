#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <iostream>
#include <string>
#include <utility>

namespace util {
    class Clock {
    public:
        Clock() : running(false) {}
        
        void start() {
            running = true;
            start_timepoint = std::chrono::system_clock::now();
        }

        int64_t peek() const {
            if(!running) return -1;
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_timepoint).count();
        }

        int64_t stop() {
            auto passed = peek();
            running = false;
            return passed;
        }
    private:
        bool running;
        std::chrono::system_clock::time_point start_timepoint;
    };

} // util

#endif // UTILS_H