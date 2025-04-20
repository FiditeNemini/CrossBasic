#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <future>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <limits>
#include <atomic>

using boost::asio::ip::tcp;
using namespace std::chrono;

// Structure to store results for a single HTTP request.
struct RequestResult {
    double duration_ms;
    std::size_t response_length;
    bool success;
};

// Mutex for synchronized console output.
std::mutex cout_mutex;

// This function performs a synchronous HTTP GET request using Boost.Asio.
RequestResult perform_request(const std::string& host, const std::string &port, const std::string &target) {
    RequestResult result = {0.0, 0, false};
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, port);
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);
        
        // Build the HTTP GET request.
        std::ostringstream request_stream;
        request_stream << "GET " << target << " HTTP/1.1\r\n";
        request_stream << "Host: " << host << "\r\n";
        request_stream << "User-Agent: ServerEval\r\n";
        request_stream << "Connection: close\r\n\r\n";
        std::string request = request_stream.str();
        
        auto start = high_resolution_clock::now();
        boost::asio::write(socket, boost::asio::buffer(request));
        
        std::string response;
        char buf[1024];
        boost::system::error_code ec;
        while (true) {
            std::size_t len = socket.read_some(boost::asio::buffer(buf), ec);
            if (len > 0) {
                response.append(buf, len);
            }
            if (ec == boost::asio::error::eof) {
                break;
            } else if (ec) {
                throw boost::system::system_error(ec);
            }
        }
        auto end = high_resolution_clock::now();
        double duration = duration_cast<microseconds>(end - start).count() / 1000.0; // in milliseconds
        
        result.duration_ms = duration;
        result.response_length = response.size();
        result.success = true;
    } catch (std::exception &e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Request error: " << e.what() << std::endl;
        result.success = false;
    }
    return result;
}

int main() {
    // Configuration.
    const int requests_per_url = 500;  // Number of requests per evaluation file.
    std::vector<std::string> urls = {"/eval1.html", "/eval2.html"}; // Targets for evaluation.
    const std::string host = "localhost";
    const std::string port = "8080";

    // Prepare futures to hold asynchronous results.
    std::vector<std::future<std::vector<RequestResult>>> futures;
    
    // Launch asynchronous tasks for each evaluation URL.
    for (const auto &target : urls) {
        futures.push_back(
            std::async(std::launch::async, [=]() -> std::vector<RequestResult> {
                std::vector<RequestResult> results;
                results.reserve(requests_per_url);
                
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Starting tests for URL: " << target << std::endl;
                }
                
                // For progress update calculation.
                int progressInterval = std::max(1, requests_per_url / 10);
                
                for (int i = 0; i < requests_per_url; i++) {
                    // Append a dummy query parameter to bypass any caching.
                    std::string target_with_dummy = target + "?dummy=" + std::to_string(i);
                    RequestResult res = perform_request(host, port, target_with_dummy);
                    results.push_back(res);
                    
                    if ((i + 1) % progressInterval == 0) {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "URL " << target << ": " << (i + 1) << " / " 
                                  << requests_per_url << " requests completed." << std::endl;
                    }
                }
                
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Finished tests for URL: " << target << std::endl;
                }
                
                return results;
            })
        );
    }
    
    // Process results and print metrics for each URL.
    for (size_t i = 0; i < futures.size(); i++) {
        std::vector<RequestResult> results = futures[i].get();
        int successCount = 0;
        double totalTime = 0.0;
        double minTime = std::numeric_limits<double>::max();
        double maxTime = 0.0;
        std::size_t totalBytes = 0;
        for (const auto &r : results) {
            if (r.success) {
                successCount++;
                totalTime += r.duration_ms;
                if (r.duration_ms < minTime) minTime = r.duration_ms;
                if (r.duration_ms > maxTime) maxTime = r.duration_ms;
                totalBytes += r.response_length;
            }
        }
        double avgTime = (successCount > 0) ? totalTime / successCount : 0.0;
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Metrics for URL " << urls[i] << ":\n";
            std::cout << "  Total Requests: " << results.size() << "\n";
            std::cout << "  Successful Requests: " << successCount << "\n";
            std::cout << "  Failed Requests: " << (results.size() - successCount) << "\n";
            std::cout << "  Average Response Time: " << std::fixed << std::setprecision(2)
                      << avgTime << " ms\n";
            std::cout << "  Minimum Response Time: " << minTime << " ms\n";
            std::cout << "  Maximum Response Time: " << maxTime << " ms\n";
            std::cout << "  Total Data Received: " << totalBytes << " bytes\n";
            std::cout << "---------------------------------------------------\n";
        }
    }
    
    return 0;
}
