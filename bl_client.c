
#include "blather.h"

simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

pthread_t user_thread;
pthread_t server_thread;

join_t join;

void *server_worker(void *arg){
    //open to-client FIFO
    int to_client_fd = open(join.to_client_fname, O_RDWR, DEFAULT_PERMS);
    while(1){
        //read a mesg_t from to-client FIFO
        mesg_t message = {};
        int read_message = read(to_client_fd, &message, sizeof(mesg_t));
        check_fail(read_message == -1, 1, "There is an error in read system call");
        
        //print appropriate response to terminal with simpio
        if (message.kind == BL_MESG) {
            iprintf(simpio, "[%s] : %s\n", message.name, message.body);
        }
        else if (message.kind == BL_JOINED) {
            iprintf(simpio, "-- %s JOINED --\n", message.name);
        }
        else if (message.kind == BL_DEPARTED) {
            iprintf(simpio, "-- %s DEPARTED --\n", message.name);
        }
        else if (message.kind == BL_SHUTDOWN) {
            iprintf(simpio, "!!! server is shutting down !!!\n");
            break;
        }
        else {
            dbg_printf("Message Kind Not Supported\n");
        }
    }
    //cancel the user thread
    pthread_cancel(user_thread);
    close(to_client_fd);
    return NULL;
}

void *user_worker(void *arg){
    //open to-server FIFO
    int to_server_fd = open(join.to_server_fname, O_RDWR, DEFAULT_PERMS);
    while(!simpio->end_of_input) {
        simpio_reset(simpio);
        iprintf(simpio, "");
        //read input using simpiio
        while(!simpio->line_ready && !simpio->end_of_input) {
            simpio_get_char(simpio);
        }
        //when a line is ready
        if(simpio->line_ready){
            //Create a mesg_t with the line
            mesg_t message = {};
            message.kind = BL_MESG;
            strcpy(message.name, join.name);
            strcpy(message.body, simpio->buf);
            //write it to the to-server FIFO
            int nwrite = write(to_server_fd, &message, sizeof(mesg_t));
            check_fail(nwrite == -1, 1, "Fail to write message to to-server FIFO");
        }
    }
    //print "End of Input, Departing"
    iprintf(simpio, "End of Input, Departing\n");

    //write a DEPARTED mesg_t into to-server
    mesg_t depart_msg = {};
    depart_msg.kind = BL_DEPARTED;
    strcpy(depart_msg.name, join.name);
    int nwrite = write(to_server_fd, &depart_msg, sizeof(mesg_t));
    check_fail(nwrite == -1, 1, "Fail to write message to to-server FIFO");
    //cancel the server thread
    pthread_cancel(server_thread);
    close(to_server_fd);
    return NULL;
}


int main(int argc, char *argv[]){
    if (argc < 3){
        printf("usage: %s <server> <name>\n", argv[0]);
        exit(1);
    }
    

    char server_name[MAXPATH];
    char to_client[MAXPATH];
    char to_server[MAXPATH];
    char prompt[MAXNAME];

    snprintf(to_client, MAXPATH, "%d.client.fifo", getpid());
    snprintf(to_server, MAXPATH, "%d.server.fifo", getpid());
    snprintf(server_name, MAXPATH, "%s.fifo", argv[1]);

    strcpy(join.name, argv[2]);
    strcpy(join.to_client_fname, to_client);
    strcpy(join.to_server_fname, to_server);
            
    mkfifo(to_client, DEFAULT_PERMS);
    mkfifo(to_server, DEFAULT_PERMS);

    int server_fd = open(server_name, O_WRONLY, DEFAULT_PERMS);
    int nwrite = write(server_fd, &join, sizeof(join_t));
    check_fail(nwrite == -1, 1, "There is an error in system call write");
    check_fail(server_fd == -1, 1, "There is an error in system call open");
    
    sprintf(prompt, "%s%s", argv[2], PROMPT);

    simpio_set_prompt(simpio, prompt);         // set the prompt
    simpio_reset(simpio);                      // initialize io
    simpio_noncanonical_terminal_mode();       // set the terminal into a compatible mode

    pthread_create(&user_thread, NULL, user_worker, NULL);     // start user thread to read input
    pthread_create(&server_thread, NULL, server_worker, NULL);

    pthread_join(user_thread, NULL);
    pthread_join(server_thread, NULL);
    
    simpio_reset_terminal_mode();
    printf("\n");                 // newline just to make returning to the terminal prettier
    return 0;
}