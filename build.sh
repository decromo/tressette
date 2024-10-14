gcc utils.c network.c common.c threads.c client.c client_network.c -Wall -pthread -g -o client;
gcc utils.c network.c common.c threads.c server.c server_network.c -Wall -pthread -g -o server;