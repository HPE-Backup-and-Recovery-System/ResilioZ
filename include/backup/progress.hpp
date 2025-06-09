#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

class ProgressBar {
public:
    ProgressBar(size_t total_size, size_t total_chunks, const std::string& operation_name);
    
    // Update progress with new bytes and chunks processed
    void update(size_t processed_bytes, size_t processed_chunks);
    
    // Mark operation as complete
    void complete();

private:
    void print_progress(float progress, size_t processed_bytes, size_t processed_chunks);
    
    size_t total_size_;
    size_t total_chunks_ = 0;
    std::string operation_name_;
    const int bar_width_ = 50;
    bool first_update_ = true;
    size_t last_line_length_ = 0;
    size_t current_chunk_ = 0;
    size_t processed_bytes_ = 0;
    std::chrono::steady_clock::time_point start_time_;
};

 