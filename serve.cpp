// c++ serve.cpp -o serve.out && ./serve.out a 1234

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <unistd.h>

using namespace std;

map<string, string> create_types () {
  map<string, string> types;
  types["css"] = "text/css";
  types["html"] = "text/html";
  types["ico"] = "image/x-icon";
  types["js"] = "application/javascript";
  return types;
}

map<string, string> types = create_types();

void serve (string folder, int port) {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int i = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, & i, sizeof(i));
    sockaddr_in sock = {};
    sock.sin_port = htons(port);
    socklen_t len = sizeof(sock);
    bind(server, (struct sockaddr *) & sock, len);
    listen(server, 5);
    printf("localhost:%d\n", port);
    while (true) {
        int client = accept(server, (struct sockaddr *) & sock, & len);
        char buffer[4096];
        read(client, buffer, 4096);
        stringstream t(buffer);
        string file;
        t >> buffer >> file;
        while ((i = file.find("%20")) != -1) {
            file.replace(i, 3, " ");
        }
        string type;
        if (file[0] != '/' || file == "/") {
            file = "/x.html";
            type = "text/html";
        } else {
            type = types[file.substr(file.find_last_of('.') + 1)];
        }
        file = folder + file;
        ifstream f(file.c_str());
        if (f.is_open()) {
            stringstream t;
            t << f.rdbuf();
            string content = t.str();
            f.close();
            string response = "HTTP/1.\ncontent-type:" + type + "\n\n" + content;
            send(client, response.c_str(), response.size(), 0);
        }
        close(client);
    }
}

int main (int argc, char * argv[]) {
    serve(argc > 1 ? argv[1] : "a", argc > 2 ? atoi(argv[2]) : 1234);
    return 0;
}