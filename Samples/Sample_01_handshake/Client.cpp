/**
 * Author:    Andrea Casalino
 * Created:   16.05.2019
*
* report any bug to andrecasa91@gmail.com.
 **/

#include <TcpClient.h>
#include <StringClient.h>
#include <iostream>
using namespace std;

int main(int argc, char **argv){

    cout << "-----------------------  Client  -----------------------" << endl;

    string server_address;
    if(argc > 1) {
       server_address = argv[1];
    }

    //build and initialize a connection to the server on port 2000
    sck::StringClient client( std::make_unique<sck::TcpClient>(sck::Address::Localhost(2000)) );
    client.open();

    string message = "Hi there!";

    //hit the server with a message
    cout << "sending to the server: " << message << endl;
    client.send(message);
    
    //get the server response
    message = *client.receive(500).get();
    cout << "got from the server:   " << message << endl;

    //just to keep the prompt active 
    int k;
    cout << "press any key to close" << endl;
    cin >> k;
    return 0;
}