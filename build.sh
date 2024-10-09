gcc utils.c network.c common.c thread.c client.c client_network.c -pthreads -g -o client;
gcc utils.c network.c common.c thread.c server.c server_network.c -pthreads -g -o server;