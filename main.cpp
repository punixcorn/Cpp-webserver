#include "headers.h"
#include <arpa/inet.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fstream>
#include <iterator>
#include <netinet/in.h>
#include <numeric>
#include <sstream>
#include <string>
#include <sys/socket.h>

template<typename T>
concept str = requires(T a) { sizeof(a[0]) == sizeof(char); };

// make sure T is a char * or string
template<typename T>
    requires str<T>
class webserver
{
  private:
    T host;
    int port;
    std::map<std::string, std::string> url_mappings;

  public:
    webserver(T Host, int Port)
      : host(Host)
      , port(Port){};

    void setPages(std::string url, std::string page)
    {
        url_mappings.insert({ url, page });
    }

    T getHost() { return host; }

    T getPort() { return port; }

  private:
    std::pair<std::string, std::string> parse_request(
      const std::string& request_path)
    {
        std::vector<std::string> tokens({ "", "" });
        std::istringstream stream(request_path);
        stream >> tokens[0] >> tokens[1];
        return { tokens[0], tokens[1] };
    };

    void get_request(int client_socket, const std::string& request_path)
    {
        std::string response{};
        if (url_mappings.find(request_path) != url_mappings.end()) {
            std::ifstream file(url_mappings[request_path]);

            if (file.is_open()) {
                response =
                  fmt::format("HTTP/1.1 200 OK\n"
                              "Content-type: text/html\r\n\r\n"
                              "{}",
                              std::string(std::istreambuf_iterator<char>(file),
                                          std::istreambuf_iterator<char>()));
            }
            file.close();
        } else {
            response = fmt::format("HTTP/1.1 404 ERROR\n"
                                   "Content-type: text/html\r\n\r\n"
                                   "{}",
                                   "<h1>ERR 404 resource not found</h1>");
        }

        send(client_socket, response.c_str(), response.size(), 0);
        close(client_socket);
    }

    void post_request(int client_socket,
                      int content_lenght,
                      const std::string& post_data)
    {

        std::stringstream stream{ post_data };
        std::string line;
        std::map<std::string, std::string> parsed_data{};

        while (std::getline(stream, line, '&')) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                parsed_data.insert(
                  { line.substr(0, pos), line.substr(pos + 1) });
            }
        }

        if (parsed_data.find("name") != parsed_data.end()) {
            std::string response = fmt::format("HTTP/1.1 200 OK\n"
                                               "Content-type: text/html\r\n\r\n"
                                               "<h1> Hello {} </h1>",
                                               parsed_data["name"]);

            std::ifstream file("static/index.html");
            if (file.is_open()) {
                response += std::string(std::istreambuf_iterator<char>(file),
                                        std::istreambuf_iterator<char>());
                file.close();
            }

            send(client_socket, response.c_str(), response.size(), 0);
            close(client_socket);
        }
    };

    int get_server_fd()
    {
        // create socket
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            fmt::print(stderr, "Failed to create server socket\n");
            return -1;
        }

        // create info for the socket
        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = inet_addr(host);
        server_address.sin_port = htons(port);

        // bind the info to the socket
        if (bind(server_socket,
                 (struct sockaddr*)&server_address,
                 sizeof(server_address)) == -1) {
            fmt::print(stderr, "Failed to bind socket\n");
            close(server_socket);
            return -1;
        }
        return server_socket;
    }

    void server_listen(int server_socket)
    {
        if (listen(server_socket, 1) == -1) {
            fmt::print(stderr, "server Failed to listen\n");
            close(server_socket);
        }

        fmt::print("Server listening on {}:{}\n", host, port);
    }

  public:
    void runServer()
    {
        int server_socket = get_server_fd();
        server_listen(server_socket);

        try {
            for (;;) {
                sockaddr_in client_address{};

                // &sizeof(sockaddr_in) directly in accept throws error
                socklen_t client_address_size = sizeof(client_address);

                int client_socket = accept(server_socket,
                                           (struct sockaddr*)&client_address,
                                           &client_address_size);

                if (client_socket == -1) {
                    fmt::print(stderr, "Err falied to accept connection\n");
                    continue;
                }

                fmt::print("accepted connection from {}\n",
                           inet_ntoa(client_address.sin_addr));

                char* buffer = new char[1024];
                int bytes = recv(client_socket, buffer, 1023, 0);

                if (bytes == 0 || bytes == -1) {
                    fmt::print(stderr, "Error recieving data\n");
                    close(client_socket);
                    continue;
                }

                // store read buffer here
                std::string request_data(buffer, bytes);

                // === === === //
                fmt::print("request_data : {} \n", request_data);
                // === === === //
                auto [method, path] = parse_request(request_data);

                fmt::print("recieved {} from {}\n", method, path);

                // handling request
                int content_lenght{ 0 };
                std::string post_data{};

                // grab content_lenght and post_data
                if (method == "POST") {
                    size_t pos = request_data.find("\r\n\r\n");
                    if (pos != std::string::npos) {
                        std::istringstream headers(request_data.substr(0, pos));
                        std::string header{};
                        while (getline(headers, header, '\r')) {
                            if (header.find("Content-Lenght: ") !=
                                std::string::npos) {
                                content_lenght = stoi(header.substr(16));
                                break;
                            }
                        }
                        post_data = request_data.substr(pos + 4);
                    }
                }

                if (method == "GET")
                    get_request(client_socket, path);
                else if (method == "POST")
                    post_request(client_socket, content_lenght, path);
                else
                    fmt::print("method not implemented yet\n");

            } // for

        } catch (const std::exception& e) {
            fmt::print(stderr, "{}\n", e.what());
        } catch (...) {
            fmt::print(stderr, "ERR: Something happened, but we dunno\n");
        }
        close(server_socket);
    }
};

int
main(int argc, char** argv)
{
    webserver Server("127.0.0.1", 8080);
    // serve these webpages
    Server.setPages("/", "static/index.html");
    Server.setPages("/about", "static/about.html");
    Server.runServer();
    return 0;
}
