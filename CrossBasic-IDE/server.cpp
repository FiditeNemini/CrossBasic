// ============================================================================
// CrossBasic Web Application Server
// Created by The Simulanics AI Team under direction of Matthew A. Combatti
// https://www.crossbasic.com
// DISCLAIMER: Simulanics Technologies and CrossBasic are not affiliated with Xojo, Inc.
// -----------------------------------------------------------------------------
// Business Source License 1.1 (BUSL-1.1)
// Copyright (c) 2025 Simulanics Technologies
//
// 1. License Grant
//    Subject to the conditions herein, Licensor grants you a non-exclusive,
//    worldwide, royalty-free license to:
//      • Use, copy, modify, and distribute this software, provided that your
//        use, modification, and distribution do not involve directly selling,
//        commercializing, or monetizing this software (CrossBasic) or its
//        direct derivatives.
//      • Use the software (CrossBasic) freely to develop and distribute your
//        own software applications, projects, and related products, which may
//        themselves be sold, licensed, or commercialized without restriction.
//
// 2. Rights Reserved
//    Licensor retains all rights to this software, including but not limited to
//    intellectual property rights, copyright, trademarks, branding, and patents.
//    You may not:
//      • Fork, copy, distribute, sublicense, or resell this software or its
//        components directly for profit.
//      • Offer commercial hosting or Software-as-a-Service (SaaS) products
//        based directly on this software without explicit permission.
//
// 3. Attribution
//    You must clearly include attribution to the original Licensor
//    (Simulanics Technologies, copyright notice, and license reference) in any
//    distributions, modifications, or other uses of the software (CrossBasic).
//    This does not apply to applications, projects, or related products which
//    you have created or developed using the software.
//
// 4. Additional Commercial Licensing
//    If you wish to use this software in a manner that directly commercializes,
//    sells, rebrands, forks, or profits directly from this project itself,
//    you must obtain an explicit commercial license from the Licensor.
//
// 5. Trademark and Branding
//    The Licensor’s trademarks, logos, and branding assets may not be used,
//    modified, or distributed without explicit written permission.
//
// 6. No Warranty
//    THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//    IMPLIED, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
//    FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//    LICENSOR BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY ARISING FROM,
//    OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR ITS USE OR OTHER DEALINGS.
//
// For inquiries about commercial licensing or permissions:
//    Contact: Matthew Combatti <mcombatti@crossbasic.com>
//    Website: https://www.crossbasic.com
//
// By using this software, you accept the terms and conditions of this license.

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <vector>
#include <cstdlib>   // std::system
#include <thread>
#include <memory>
#include <mutex>
#include <chrono>
#include <cctype>
#include <stdexcept>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <zlib.h>
#include <cstring>  // For memset



namespace fs = std::filesystem;
using boost::asio::ip::tcp;
using boost::multiprecision::cpp_dec_float_50;

// Global debug flag
bool DEBUG = true;
void debugLog(const std::string &msg) {
    if (DEBUG)
        std::cout << msg << std::endl;
}

// ----------------------------
// Utility Functions
// ----------------------------

// URL‑decode percent‑encoding
std::string urlDecode(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            std::string hex = s.substr(i + 1, 2);
            char c = static_cast<char>(std::stoi(hex, nullptr, 16));
            out.push_back(c);
            i += 2;
        }
        else if (s[i] == '+') {
            out.push_back(' ');
        }
        else {
            out.push_back(s[i]);
        }
    }
    return out;
}

// Compute a fingerprint based on file size and last‑modified time.
std::string getFingerprint(const fs::path &full) {
    auto ftime = fs::last_write_time(full);
    // Convert file_time_type to time_t in C++17:
    auto sctp = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now() + (ftime - fs::file_time_type::clock::now())
    );
    std::uintmax_t fsize = fs::file_size(full);
    return std::to_string(fsize) + "-" + std::to_string(sctp);
}


std::string gzipCompressString(const std::string &data) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED,
                     16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("deflateInit2 failed");
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    zs.avail_in = data.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = deflate(&zs, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&zs);
            throw std::runtime_error("deflate failed");
        }
        size_t have = sizeof(outbuffer) - zs.avail_out;
        outstring.append(outbuffer, have);
    } while (ret != Z_STREAM_END);

    deflateEnd(&zs);
    return outstring;
}


// ----------------------------
// Recursive‑Descent Parser
// ----------------------------
class ExprParser {
public:
    ExprParser(const std::string &s) : str_(s), pos_(0) {}
    cpp_dec_float_50 parse() {
        cpp_dec_float_50 result = parseExpression();
        skipWhitespace();
        if (pos_ != str_.size())
            throw std::runtime_error("Unexpected character at position " + std::to_string(pos_));
        return result;
    }
private:
    std::string str_;
    size_t pos_;
    
    void skipWhitespace() {
        while (pos_ < str_.size() && std::isspace(str_[pos_])) ++pos_;
    }
    
    cpp_dec_float_50 parseExpression() {
        cpp_dec_float_50 lhs = parseTerm();
        while (true) {
            skipWhitespace();
            if (pos_ < str_.size() && (str_[pos_] == '+' || str_[pos_] == '-')) {
                char op = str_[pos_++];
                cpp_dec_float_50 rhs = parseTerm();
                if (op == '+')
                    lhs = lhs + rhs;
                else
                    lhs = lhs - rhs;
            } else break;
        }
        return lhs;
    }
    
    cpp_dec_float_50 parseTerm() {
        cpp_dec_float_50 lhs = parseFactor();
        while (true) {
            skipWhitespace();
            if (pos_ < str_.size() && (str_[pos_] == '*' || str_[pos_] == '/')) {
                char op = str_[pos_++];
                cpp_dec_float_50 rhs = parseFactor();
                if (op == '*')
                    lhs = lhs * rhs;
                else
                    lhs = lhs / rhs;
            } else break;
        }
        return lhs;
    }
    
    cpp_dec_float_50 parseFactor() {
        skipWhitespace();
        if (pos_ < str_.size() && str_[pos_] == '(') {
            ++pos_;
            auto val = parseExpression();
            skipWhitespace();
            if (pos_ == str_.size() || str_[pos_] != ')')
                throw std::runtime_error("Missing closing parenthesis");
            ++pos_;
            return val;
        }
        return parseNumber();
    }
    
    cpp_dec_float_50 parseNumber() {
        skipWhitespace();
        size_t start = pos_;
        if (pos_ < str_.size() && (str_[pos_] == '+' || str_[pos_] == '-'))
            ++pos_;
        while (pos_ < str_.size() && (std::isdigit(str_[pos_]) || str_[pos_] == '.'))
            ++pos_;
        std::string token = str_.substr(start, pos_ - start);
        if (token.empty())
            throw std::runtime_error("Number expected");
        return cpp_dec_float_50(token);
    }
};

// ----------------------------
// HTTP Request/Response Structs
// ----------------------------
struct HttpRequest {
    std::string method, uri, version, body;
    std::map<std::string, std::string> headers;
};
struct HttpResponse {
    std::string status_code, status_msg, body;
    std::map<std::string, std::string> headers;
};

// ----------------------------
// Global Image Cache for Static Files
// ----------------------------
struct ImageCacheEntry {
    std::string data;           // Raw file data.
    std::string gzippedData;      // Compressed version.
    std::string fingerprint;
    std::chrono::steady_clock::time_point lastUpdate;
};

std::unordered_map<std::string, ImageCacheEntry> imageCache;
std::mutex imageCacheMutex;

// ----------------------------
// Function Registry
// ----------------------------
class FunctionRegistry {
public:
    using Fn = std::function<HttpResponse(const std::map<std::string, std::string>&)>;
    void registerFunction(const std::string &name, Fn fn) { fns_[name] = fn; }
    bool exists(const std::string &name) const { return fns_.count(name) > 0; }
    HttpResponse call(const std::string &name, const std::map<std::string, std::string>& p) {
        return fns_.at(name)(p);
    }
private:
    std::map<std::string, Fn> fns_;
};

// ----------------------------
// Global File I/O Thread Pool
// ----------------------------
boost::asio::thread_pool file_io_pool(4);

// ----------------------------
// Session Handling
// ----------------------------
class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket sock, FunctionRegistry &reg)
     : sock_(std::move(sock)), registry_(reg) {}
    void start() { readRequest(); }
    
private:
    tcp::socket sock_;
    boost::asio::streambuf requestBuffer_; // For complete request reading
    FunctionRegistry &registry_;
    
    // Read HTTP request using async_read_until to ensure full header is captured.
    void readRequest() {
        auto self = shared_from_this();
        boost::asio::async_read_until(sock_, requestBuffer_, "\r\n\r\n",
            [this, self](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
                if (!ec) {
                    std::istream reqStream(&requestBuffer_);
                    std::string raw;
                    std::getline(reqStream, raw, '\0');
                    HttpRequest req = parseRequest(raw);
                    //debugLog("REQ:\n" + raw);
                    // Distinguish between AJAX calls and static file requests.
                    if (req.uri.rfind("/call?", 0) == 0) {
                        HttpResponse res = handleAjaxCall(req);
                        writeResponse(res);
                    } else {
                        handleStaticFile(req);
                    }
                } else {
                    debugLog("Error reading request: " + ec.message());
                }
            });
    }
    
    // Parse HTTP Request from raw string
    HttpRequest parseRequest(const std::string &raw) {
        HttpRequest req;
        std::istringstream ss(raw);
        ss >> req.method >> req.uri >> req.version;
        std::string line;
        std::getline(ss, line);
        while (std::getline(ss, line) && !line.empty() && line != "\r") {
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                std::string name = line.substr(0, pos);
                std::string val  = line.substr(pos + 1);
                boost::algorithm::trim(name);
                boost::algorithm::trim(val);
                req.headers[name] = val;
            }
        }
        std::string body;
        std::getline(ss, body, '\0');
        req.body = body;
        return req;
    }
    
    // Parse URL Query String into a map
    std::map<std::string, std::string> parseQuery(const std::string &q) {
        std::map<std::string, std::string> m;
        std::istringstream ss(q);
        std::string tok;
        while (std::getline(ss, tok, '&')) {
            auto p = tok.find('=');
            if (p != std::string::npos) {
                std::string key = urlDecode(tok.substr(0, p));
                std::string val = urlDecode(tok.substr(p + 1));
                m[key] = val;
            }
        }
        return m;
    }
    
    // Handle AJAX calls (e.g., /call?function=...)
    HttpResponse handleAjaxCall(const HttpRequest &req) {
        HttpResponse res;
        auto q = req.uri.substr(req.uri.find('?') + 1);
        auto params = parseQuery(q);
        std::string fn = params["function"];
        if (registry_.exists(fn)) {
            res = registry_.call(fn, params);
        } else {
            res.status_code = "404"; res.status_msg = "Not Found";
            res.headers["Content-Type"] = "application/json";
            res.body = R"({"error":"Function not found"})";
        }
        res.headers["Content-Length"] = std::to_string(res.body.size());
        if (req.headers.find("Connection") != req.headers.end() &&
            boost::iequals(req.headers.at("Connection"), "keep-alive"))
            res.headers["Connection"] = "keep-alive";
        return res;
    }
    
    // Handle Static File Requests with asynchronous file I/O and caching for images.
    void handleStaticFile(const HttpRequest &req) {
        std::string path = req.uri;
        if (path == "/")
            path = "/index.html";
        fs::path full = fs::current_path() / "www" / path.substr(1);
        if (!fs::exists(full) || fs::is_directory(full)) {
            HttpResponse res;
            res.status_code = "404";
            res.status_msg = "Not Found";
            res.headers["Content-Type"] = "text/plain";
            res.body = "404 Not Found";
            res.headers["Content-Length"] = std::to_string(res.body.size());
            writeResponse(res);
            return;
        }
        
        std::string ext = full.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        bool isCached = (ext == ".png" || ext == ".gif" || ext == ".jpg" || ext == ".jpeg");
        
        auto self = shared_from_this();
        // Lambda to set the content type header based on file extension.
        auto setContentType = [ext](HttpResponse &res) {
            if (ext == ".html" || ext == ".htm" || ext == ".xs")
                res.headers["Content-Type"] = "text/html; charset=UTF-8";
            else if (ext == ".css")
                res.headers["Content-Type"] = "text/css";
            else if (ext == ".js")
                res.headers["Content-Type"] = "application/javascript";
            else if (ext == ".png")
                res.headers["Content-Type"] = "image/png";
            else if (ext == ".gif")
                res.headers["Content-Type"] = "image/gif";
            else if (ext == ".jpg" || ext == ".jpeg")
                res.headers["Content-Type"] = "image/jpeg";
            else if (ext == ".zip")
                res.headers["Content-Type"] = "application/zip";
            else if (ext == ".txt")
                res.headers["Content-Type"] = "text/plain";
            else
                res.headers["Content-Type"] = "application/octet-stream";
        };
        
        // Check if client accepts gzip.
        bool clientAcceptsGzip = (req.headers.find("Accept-Encoding") != req.headers.end() &&
                                  req.headers.at("Accept-Encoding").find("gzip") != std::string::npos);
        
        if (isCached) {
            // ----------------------------
            // Image file – Use caching and asynchronous I/O.
            // ----------------------------
            {
                std::lock_guard<std::mutex> lock(imageCacheMutex);
                auto it = imageCache.find(full.string());
                if (it != imageCache.end()) {
                    auto now = std::chrono::steady_clock::now();
                    auto recacheInterval = std::chrono::minutes(5);
                    if (now - it->second.lastUpdate < recacheInterval) {
                        HttpResponse res;
                        res.status_code = "200";
                        res.status_msg = "OK";
                        setContentType(res);
                        if (clientAcceptsGzip && !it->second.gzippedData.empty()) {
                            res.body = it->second.gzippedData;
                            res.headers["Content-Encoding"] = "gzip";
                        } else {
                            res.body = it->second.data;
                        }
                        res.headers["Content-Length"] = std::to_string(res.body.size());
                        if (req.headers.find("Connection") != req.headers.end() &&
                            boost::iequals(req.headers.at("Connection"), "keep-alive"))
                            res.headers["Connection"] = "keep-alive";
                        writeResponse(res);
                        
                        // Schedule a background update if necessary.
                        if (now - it->second.lastUpdate >= recacheInterval) {
                            boost::asio::post(file_io_pool, [full, req, self]() {
                                try {
                                    std::ifstream ifs(full, std::ios::binary);
                                    std::ostringstream buf;
                                    buf << ifs.rdbuf();
                                    std::string fileData = buf.str();
                                    std::string newFingerprint = getFingerprint(full);
                                    std::lock_guard<std::mutex> lock(imageCacheMutex);
                                    auto &entry = imageCache[full.string()];
                                    if (entry.fingerprint == newFingerprint) {
                                        entry.lastUpdate = std::chrono::steady_clock::now();
                                    } else {
                                        entry.data = fileData;
                                        entry.fingerprint = newFingerprint;
                                        entry.gzippedData = gzipCompressString(fileData);
                                        entry.lastUpdate = std::chrono::steady_clock::now();
                                    }
                                } catch (...) {
                                    debugLog("Background cache update failed for " + full.string());
                                }
                            });
                        }
                        return;
                    }
                }
            }
            // If not cached or stale, read file asynchronously and update cache.
            boost::asio::post(file_io_pool, [full, req, self, setContentType, clientAcceptsGzip, ext]() {
                try {
                    std::ifstream ifs(full, std::ios::binary);
                    std::ostringstream buf;
                    buf << ifs.rdbuf();
                    std::string fileData = buf.str();
                    std::string newFingerprint = getFingerprint(full);
                    std::string gzippedData = gzipCompressString(fileData);
                    
                    {
                        std::lock_guard<std::mutex> lock(imageCacheMutex);
                        imageCache[full.string()] = ImageCacheEntry{
                            fileData,
                            gzippedData,
                            newFingerprint,
                            std::chrono::steady_clock::now()
                        };
                    }
                    
                    HttpResponse res;
                    res.status_code = "200";
                    res.status_msg = "OK";
                    if (ext == ".png")
                        res.headers["Content-Type"] = "image/png";
                    else if (ext == ".gif")
                        res.headers["Content-Type"] = "image/gif";
                    else if (ext == ".jpg" || ext == ".jpeg")
                        res.headers["Content-Type"] = "image/jpeg";
                    
                    if (clientAcceptsGzip) {
                        res.body = gzippedData;
                        res.headers["Content-Encoding"] = "gzip";
                    } else {
                        res.body = fileData;
                    }
                    res.headers["Content-Length"] = std::to_string(res.body.size());
                    if (req.headers.find("Connection") != req.headers.end() &&
                        boost::iequals(req.headers.at("Connection"), "keep-alive"))
                        res.headers["Connection"] = "keep-alive";
                    
                    boost::asio::post(self->sock_.get_executor(), [self, res]() {
                        self->writeResponse(res);
                    });
                } catch (std::exception &e) {
                    HttpResponse res;
                    res.status_code = "500";
                    res.status_msg = "Internal Server Error";
                    res.headers["Content-Type"] = "text/plain";
                    res.body = std::string("Error reading file: ") + e.what();
                    res.headers["Content-Length"] = std::to_string(res.body.size());
                    boost::asio::post(self->sock_.get_executor(), [self, res]() {
                        self->writeResponse(res);
                    });
                }
            });
            return;
        }
        
        // ----------------------------
        // Non-image static files: offload file I/O asynchronously.
        // ----------------------------
        boost::asio::post(file_io_pool, [full, req, self, setContentType, clientAcceptsGzip]() {
            try {
                std::ifstream ifs(full, std::ios::binary);
                std::ostringstream buf;
                buf << ifs.rdbuf();
                std::string fileData = buf.str();
                std::string compressedData;
                if (clientAcceptsGzip)
                    compressedData = gzipCompressString(fileData);
                
                HttpResponse res;
                res.status_code = "200";
                res.status_msg = "OK";
                setContentType(res);
                if (clientAcceptsGzip) {
                    res.body = compressedData;
                    res.headers["Content-Encoding"] = "gzip";
                } else {
                    res.body = fileData;
                }
                res.headers["Content-Length"] = std::to_string(res.body.size());
                if (req.headers.find("Connection") != req.headers.end() &&
                    boost::iequals(req.headers.at("Connection"), "keep-alive"))
                    res.headers["Connection"] = "keep-alive";
                
                boost::asio::post(self->sock_.get_executor(), [self, res]() {
                    self->writeResponse(res);
                });
            } catch (std::exception &e) {
                HttpResponse res;
                res.status_code = "500";
                res.status_msg = "Internal Server Error";
                res.headers["Content-Type"] = "text/plain";
                res.body = std::string("Error reading file: ") + e.what();
                res.headers["Content-Length"] = std::to_string(res.body.size());
                boost::asio::post(self->sock_.get_executor(), [self, res]() {
                    self->writeResponse(res);
                });
            }
        });
    }

    void writeResponse(const HttpResponse &res) {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << res.status_code << " " << res.status_msg << "\r\n";
        for (auto &h : res.headers)
            ss << h.first << ": " << h.second << "\r\n";
        ss << "\r\n" << res.body;
        // Allocate the response string on the heap to extend its lifetime.
        auto out = std::make_shared<std::string>(ss.str());
        bool keepAlive = (res.headers.find("Connection") != res.headers.end() &&
                          boost::iequals(res.headers.at("Connection"), "keep-alive"));
        auto self = shared_from_this();
        //debugLog("RESP:\n" + *out);
        boost::asio::async_write(sock_, boost::asio::buffer(*out),
            [self, out, keepAlive](boost::system::error_code ec, std::size_t /*length*/) {
                // The shared pointer "out" ensures that the response stays valid until the write completes.
                if (!ec && keepAlive) {
                    self->readRequest();
                }
            });
    }
    
};

// ----------------------------
// Server Acceptor
// ----------------------------
class HttpServer {
public:
    HttpServer(boost::asio::io_context &ctx, short port, FunctionRegistry &reg)
     : acceptor_(ctx, tcp::endpoint(tcp::v4(), port)), registry_(reg) {
        doAccept();
    }
private:
    tcp::acceptor acceptor_;
    FunctionRegistry &registry_;
    void doAccept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket sock) {
                if (!ec)
                    std::make_shared<HttpSession>(std::move(sock), registry_)->start();
                doAccept();
            });
    }
};

// ----------------------------
// Main Function
// ----------------------------
int main(){
    try {
        boost::asio::io_context ctx;
        auto work = boost::asio::make_work_guard(ctx);

        FunctionRegistry registry;

        // "hello" function
        registry.registerFunction("hello", [&](auto&) {
            debugLog("Function Invoked: hello");
            HttpResponse r;
            r.status_code = "200";
            r.status_msg = "OK";
            r.headers["Content-Type"] = "application/json";
            r.body = R"({"result":"Hello, World!"})";
            return r;
        });

        // "redirect" function
        registry.registerFunction("redirect", [&](auto&) {
            debugLog("Function Invoked: redirect");
            HttpResponse r;
            r.status_code = "200";
            r.status_msg = "OK";
            r.headers["Content-Type"] = "application/json";
            r.body = R"({"redirect":"/newpage.html"})";
            return r;
        });

        // "calculate" function
        registry.registerFunction("calculate", [&](auto &p) {
            debugLog("Function Invoked: calculate");
            HttpResponse r;
            r.status_code = "200";
            r.status_msg = "OK";
            r.headers["Content-Type"] = "application/json";
            try {
                ExprParser parser(p.at("expr"));
                auto result = parser.parse();
                std::ostringstream os;
                os << std::fixed << std::setprecision(50) << result;
                r.body = std::string("{\"result\":\"") + os.str() + "\"}";
            } catch (std::exception &e) {
                r.body = std::string("{\"error\":\"") + e.what() + "\"}";
            }
            return r;
        });

        /*
        *  COMMAND  “build”  RPC
        *  ─────────────────
        *  Expects  ?function=build&code=<url-encoded CrossBasic source>
        *  ▸ Saves the code to temp.xs
        *  ▸ Launches  crossbasic --s temp.xs  in a detached process
        *  ▸ Returns JSON  {"result":"Build started"}
        */
        registry.registerFunction("build",
            [&](const std::map<std::string,std::string>& p) -> HttpResponse
        {
            debugLog("Function Invoked: build");

            HttpResponse r;
            r.status_code = "200";
            r.status_msg  = "OK";
            r.headers["Content-Type"] = "application/json";

            try {
                /* 1️⃣ get the code text */
                auto it = p.find("code");
                if (it == p.end())
                    throw std::runtime_error("Missing 'code' parameter");
                const std::string& code = it->second;

                /* 2️⃣ write it to   temp.xs  in the server’s cwd */
                const std::string filename = "temp.xs";
                {
                    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
                    if (!ofs) throw std::runtime_error("Unable to create temp.xs");
                    ofs << code;
                }

                /* 3️⃣ spawn CrossBasic in its own OS process (non-blocking) */
            #ifdef _WIN32
                std::string cmd = "start \"\" \"crossbasic\" --s " + filename;
            #else
                std::string cmd = "crossbasic --s " + filename + " &";
            #endif

                std::thread([cmd]{
                    std::system(cmd.c_str());   // child runs; this thread just waits
                }).detach();

                /* 4️⃣ tell the browser we’re good */
                r.body = R"({"result":"Build started"})";
            }
            catch (const std::exception& ex) {
                r.body = std::string("{\"error\":\"") + ex.what() + "\"}";
            }
            r.headers["Content-Length"] = std::to_string(r.body.size());
            return r;
        });


        HttpServer server(ctx, 8080, registry);
        std::cout << "CrossBasic Application Server is listening for connections on port 8080." << std::endl;

        const std::size_t threadCount = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> threads;
        for (std::size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back([&ctx]() {
                ctx.run();
            });
        }
        for (auto &t : threads)
            t.join();
    } catch (std::exception &e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
    }
    return 0;
}
