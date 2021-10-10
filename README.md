# Chat Server/Client

This is the second project from CSCI 4061 in University of Minnesota - Twin Cities. This project focuses on developing a simple chat server and client called blather. The basic usage is in two parts.

### Server
---
Some user starts bl_server which manages the chat "room". The server is non-interactive and will likely only print debugging output as it runs.

### Client
---
Any user who wishes to chat runs bl_client which takes input typed on the keyboard and sends it to the server. The server broadcasts the input to all other clients who can respond by typing their own input.

### Blather
---
It utilizes a combination of many different system tools:
* Multiple communicating processes: clients to servers
* Communication through FIFOs
* Signal handling for graceful server shutdown
* Alarm signals for periodic behavior
* Input multiplexing with poll()
* Multiple threads in the client to handle typed input versus info from the server

### Preview
![Preview](https://user-images.githubusercontent.com/77591817/136682718-e1a611f7-ee7d-4728-8f7e-80a37bb55af7.png)
