#include "blather.h"

int KEEP_GOING = 1;

void handle_SIG(int sig_num){
    KEEP_GOING = 0;
}

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("usage: %s <server>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, handle_SIG);
    signal(SIGTERM, handle_SIG);

    server_t server = {};

    server_start(&server, argv[1], DEFAULT_PERMS);

    while(KEEP_GOING) {
        //check all sources
        server_check_sources(&server);
        
        //This is for breaking the loop. We tried to solve the issue called "143" by using this condition.
        //This one is working in the gradescope not SSH.
        if(KEEP_GOING == 0){
            break;
        }

        //handle a join request if one is ready
        if(server_join_ready(&server)) {
            server_handle_join(&server);
        }

        int numClients = server.n_clients;

        //for each client
        for(int i = 0; i < numClients; i++) {
            //if the client is ready handle data from it
            if(server_client_ready(&server, i)) {
                server_handle_client(&server, i);
            }
        }
    }
    server_shutdown(&server);
    return 0;
}