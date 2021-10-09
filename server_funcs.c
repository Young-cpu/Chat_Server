#include "blather.h"

#define FIFO_EXTENSION 6

client_t *server_get_client(server_t *server, int idx){
    if (idx > server->n_clients){
        printf("the behavior of the function is unspecified and may cause a program crash.\n");
        exit(1);
    }
    else{
        return &server->client[idx];
    }
}

void server_start(server_t *server, char *server_name, int perms){
    log_printf("BEGIN: server_start()\n");
    char server_fifo[strlen(server_name)+FIFO_EXTENSION];

    sprintf(server->server_name, "%s", server_name);
    snprintf(server_fifo, (strlen(server_name)+FIFO_EXTENSION), "%s.fifo", server_name);

    server->n_clients = 0;

    //creates and opens FIFO correctly for joins, removes existing FIFO
    unlink(server_fifo);
    int create_fifo = mkfifo(server_fifo, perms);
    check_fail(create_fifo == -1, 1, "There is an error in mkfifo");

    server->join_fd = open(server_fifo, O_RDWR, perms);
    server->join_ready = 1;

    // Advanced
    char serverlog[strlen(server_name)+5];  
    
    snprintf(serverlog, strlen(server_name)+5, "%s.log", server_name);
    
    int open_log= open(serverlog, O_CREAT |O_WRONLY| O_APPEND, perms);
    check_fail(open_log == -1, 1, "There is an error in open_log");
    
    struct stat advanced_stat;
    stat(serverlog, &advanced_stat);

    server->log_fd = open_log;

    who_t current_clients = {};
    write(server->log_fd, &current_clients, sizeof(who_t));
    log_printf("END: server_start()\n");

}

void server_shutdown(server_t *server){
    log_printf("BEGIN: server_shutdown()\n");
    
    int arr_length = strlen(server->server_name) + FIFO_EXTENSION;
    char server_fifo[arr_length];                        //Server
    
    mesg_t shutdown ={};
    shutdown.kind = BL_SHUTDOWN;
        
    //closes and removes FIFO to prevent additional joins
    close(server->join_fd);
    close(server->log_fd);

    snprintf(server_fifo, arr_length, "%s.fifo", server->server_name);

    unlink(server_fifo);



    //broadcasts a shutdown message
    server_broadcast(server, &shutdown);

    for (int i = 0; i< server->n_clients; i++){
        server_remove_client(server, i);
    }

    log_printf("END: server_shutdown()\n");
}

int server_add_client(server_t *server, join_t *join){
    log_printf("BEGIN: server_add_client()\n");

    //does bounds checking to prevent overflow on add
    check_fail(server->n_clients == MAXCLIENTS, 0, "No spaces for more clients");

    //adds client to end of array and increments n_clients
    client_t *tmp_client = server_get_client(server, server->n_clients);
    tmp_client->data_ready = 0;

    // fills in fixed fields of client data based on join parameter
    // makes use of strncpy() to prevent buffer overruns
    strncpy(tmp_client->name, join->name, MAXNAME);
    strncpy(tmp_client->to_client_fname, join->to_client_fname, MAXPATH);
    strncpy(tmp_client->to_server_fname, join->to_server_fname, MAXPATH);

    //opens to-client and to-server FIFOs for reading/writing
    tmp_client->to_client_fd = open(tmp_client->to_client_fname, O_WRONLY, DEFAULT_PERMS);
    tmp_client->to_server_fd = open(tmp_client->to_server_fname, O_RDONLY, DEFAULT_PERMS);
    
    check_fail((tmp_client->to_client_fd) == -1, 1, "There is an issue with opening client file descriptor.");
    check_fail((tmp_client->to_server_fd) == -1, 1, "There is an issue with opening client file descriptor.");
    
    server->n_clients++;
    log_printf("END: server_add_client()\n");
    return 0;
}


int server_remove_client(server_t *server, int idx){
    //uses server_get_client() for readability
    client_t *tmp_client = server_get_client(server, idx);

    //closes to-client and from-client FIFOs
    close(tmp_client->to_client_fd);
    close(tmp_client->to_server_fd);

    unlink(tmp_client->to_client_fname);
    unlink(tmp_client->to_server_fname);

    //correctly shifts array of clients to maintain contiguous client array and order of joining
    for (int i =idx; i < server->n_clients; i++){
        server->client[i] = server->client[i+1];
    }

    server->n_clients--;

    return 0;
}

void server_broadcast(server_t *server, mesg_t *mesg){
  if (mesg->kind != BL_PING){
     server_log_message(server, mesg);
  }
  for(int i = 0; i < server->n_clients; i++){
    int broadcast = write(server_get_client(server, i)->to_client_fd, mesg, sizeof(mesg_t));
    check_fail(broadcast == -1 , 1, "Error writing to client %d fifo\n", i);
  }
}

/*
struct pollfd
{
    int fd          //File descriptor
    short events    //Event
    short revents   //Return event
};
*/
void server_check_sources(server_t *server){
    log_printf("BEGIN: server_check_sources()\n");
    //makes use of poll() system call to detect ready clients/joins
    int current_pfds = server->n_clients + 1;

    struct pollfd pfds[MAXCLIENTS];
    if (server->n_clients == MAXCLIENTS){
        return;
    }
    pfds[0].fd = server->join_fd;
    pfds[0].events = POLLIN;
    
    //Initialize all the fd as 0.
    for (int i = 1; i < current_pfds; i++){
        //Prevengting segmentation fault.
        if(i == MAXCLIENTS){
            return;
        }
        else{
            pfds[i].fd = server_get_client(server, i-1)->to_server_fd;
            pfds[i].events = POLLIN;
            server_get_client(server, i-1)->data_ready = 0;
        }
    }
    log_printf("poll()'ing to check %d input sources\n", current_pfds);
    //ret is the return value of poll
    int ret = poll(pfds, current_pfds, -1);

    log_printf("poll() completed with return value %d\n", ret); // after poll() call
    
    //checks the return value of poll() to determine if it completed or was signaled
    if (ret > 0){    
        
        //checks join FIFO and all clients in poll() call
        //does not read data but sets read flags for join and clients
        
        if(pfds[0].revents & POLLIN){
            server->join_ready = 1;
        }

        log_printf("join_ready = %d\n", server->join_ready);

        for(int i = 1; i < current_pfds; i++){
            //Prevengting segmentation fault.
            if (i == MAXCLIENTS){
                return;
            }
            if(pfds[i].revents & POLLIN){
                server_get_client(server,i-1)->data_ready = 1;
            }
            log_printf("client %d '%s' data_ready = %d\n",i-1, server_get_client(server,i-1)->name, server_get_client(server,i-1)->data_ready);
        }   
    }
    else if(ret == -1){
        log_printf("poll() interrupted by a signal\n");
    }
    log_printf("END: server_check_sources()\n");
}


// Return the join_ready flag from the server which indicates whether
// a call to server_handle_join() is safe.
int server_join_ready(server_t *server){
    return server->join_ready;
}

void server_handle_join(server_t *server){
    if(server_join_ready(server) == 0){
        return;
    }
    log_printf("BEGIN: server_handle_join()\n");
    
    //reads a join_t from join FIFO correctly
    join_t request_join;
    int read_request = read(server->join_fd, &request_join, sizeof(join_t));

    check_fail(read_request == -1, 1, "There is an error in read system call");

    log_printf("join request for new client '%s'\n", request_join.name);
    //adds client with server_add_client()
    server_add_client(server, &request_join);

    //broadcasts join
    mesg_t join_msg = {};
    join_msg.kind = BL_JOINED;
    
    sprintf(join_msg.name, "%s", request_join.name);
    server_broadcast(server, &join_msg);
    
    server->join_ready = 0;

    log_printf("END: server_handle_join()\n");
}

void server_log_message(server_t *server, mesg_t *mesg){
    write(server->log_fd, mesg, sizeof(mesg_t));
    
}

int server_client_ready(server_t *server, int idx) {
    return server_get_client(server, idx)->data_ready;
}

void server_handle_client(server_t *server, int idx){
    log_printf("BEGIN: server_handle_client()\n");
    mesg_t message;
    client_t *tmp = server_get_client(server, idx);

    int nread = read(server_get_client(server,idx)->to_server_fd, &message, sizeof(mesg_t));
    check_fail(nread == -1, 1, "There is error in read system call");

    if(message.kind == BL_MESG) {
        log_printf("client %d '%s' MESSAGE '%s'\n", idx, message.name, message.body);
        server_broadcast(server, &message);
        tmp->data_ready = 0;
    }
    else if(message.kind == BL_DEPARTED){
        log_printf("client %d '%s' DEPARTED\n", idx, message.name);
        server_remove_client(server,idx);
        server_broadcast(server, &message);
        
    }
    else {
        dbg_printf("%d: Other message types are not supported\n", message.kind);
        exit(1);
    }
    log_printf("END: server_handle_client()\n");
}