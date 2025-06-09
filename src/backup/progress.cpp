#include "backup/progress.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>


ProgressBar::ProgressBar(size_t total_size, size_t total_chunks, const std::string& operation_name)
    : total_size_(total_size)
    , total_chunks_(total_chunks)
    , operation_name_(operation_name)
    , first_update_(true)
    , last_line_length_(0)
    , current_chunk_(0)
    , processed_bytes_(0)
    , start_time_(std::chrono::steady_clock::now()) {
    std::cout << "\nStarting " << operation_name_ << " (" << total_size_ << " bytes)" << std::endl;
}

void ProgressBar::update(size_t processed_bytes, size_t processed_chunks) {
    processed_bytes_ = std::max(processed_bytes,processed_bytes_);
    current_chunk_ = std::max(processed_chunks,current_chunk_);
    float progress = static_cast<float>(processed_bytes_) / total_size_;
    print_progress(progress, processed_bytes_, current_chunk_);
}

void ProgressBar::complete() {
    // Print 100% progress
    total_chunks_ = std::max(current_chunk_,total_chunks_);
    print_progress(1.0f, total_size_, total_chunks_);
    std::cout << "\nCompleted " << operation_name_ << std::endl;
}

void ProgressBar::print_progress(float progress, size_t processed_bytes, size_t processed_chunks) {
    // Build the entire progress line in a string buffer first
    std::ostringstream oss;
    
    int filled_width = static_cast<int>(bar_width_ * progress);
    
    oss << "[";
    for (int i = 0; i < bar_width_; ++i) {
        if (i < filled_width) {
            oss << "=";
        } else if (i == filled_width && progress < 1.0f) {
            oss << ">";
        } else {
            oss << " ";
        }
    }
    oss << "] " << std::fixed << std::setprecision(1)
        << (progress * 100.0) << "% "
        << processed_bytes << "/" << total_size_
        << " bytes (" << processed_chunks;
    
    if (total_chunks_ > 0) {
        oss << "/" << total_chunks_;
    }
    oss << " chunks)";

    // Add elapsed time
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    auto hours = std::chrono::duration_cast<std::chrono::hours>(elapsed);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsed - hours);
    auto seconds = elapsed - hours - minutes;
    
    oss << " [Time: ";
    if (hours.count() > 0) {
        oss << hours.count() << "h ";
    }
    if (minutes.count() > 0 || hours.count() > 0) {
        oss << minutes.count() << "m ";
    }
    oss << seconds.count() << "s]";
    
    std::string progress_line = oss.str();
    
    // Clear the line and print the new progress
    if (!first_update_) {
        // Move cursor to beginning of line and clear it
        std::cout << "\r" << std::string(last_line_length_, ' ') << "\r";
    } else {
        first_update_ = false;
    }
    
    std::cout << progress_line << std::flush;
    last_line_length_ = progress_line.length();
}
