#include <cstdio>
#include <libconfig.h++>

#include "HttpServer.h"

namespace tzhttpd {

using namespace http_proto;

int get_test_handler(const HttpParser& http_parser, std::string& response, std::string& status_line) {
    response = "test uri called...";
    status_line = generate_response_status_line(http_parser.get_version(), StatusCode::success_ok);
    return 0;
}

}

int main(int argc, char* argv[]) {

    std::string cfgfile = "../tzhttpd.conf";
    libconfig::Config cfg;
    std::shared_ptr<tzhttpd::HttpServer> http_server_ptr;

    try {
        cfg.readFile(cfgfile.c_str());
    } catch(libconfig::FileIOException &fioex) {
        fprintf(stderr, "I/O error while reading file: %s.", cfgfile.c_str());
        return false;
    } catch(libconfig::ParseException &pex) {
        fprintf(stderr, "Parse error at %d - %s", pex.getLine(), pex.getError());
        return false;
    }

    std::string bind_addr;
    int listen_port = 0;
    if (!cfg.lookupValue("http.bind_addr", bind_addr) || !cfg.lookupValue("http.listen_port", listen_port) ){
        fprintf(stderr, "Error, get value error");
        return false;
    }

    fprintf(stderr, "listen at: %s:%d", bind_addr.c_str(), listen_port);
    http_server_ptr.reset(new tzhttpd::HttpServer(bind_addr, static_cast<unsigned short>(listen_port)));
    if (!http_server_ptr || !http_server_ptr->init(cfg)) {
        fprintf(stderr, "Init HttpServer failed!");
        return false;
    }

    http_server_ptr->register_http_get_handler("^/test$", tzhttpd::get_test_handler);
    std::string demo_path = "../cgi-bin/libdemo.so";
    tzhttpd::http_handler::CgiGetWrapper demo_get(demo_path);
    if (!demo_get.init()) {
        fprintf(stderr, "init demo_get failed!");
        return false;
    }
    http_server_ptr->register_http_get_handler("^/demo$", demo_get);

    http_server_ptr->io_service_threads_.start_threads();
    http_server_ptr->service();

    http_server_ptr->io_service_threads_.join_threads();

    return 0;
}

namespace boost {

void assertion_failed(char const * expr, char const * function, char const * file, long line) {
    fprintf(stderr, "BAD!!! expr `%s` assert failed at %s(%ld): %s", expr, file, line, function);
}

} // end boost