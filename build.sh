if [[ -z $DEST ]]
    then DEST="$(pwd)"
fi

if [[ -z $SOURCE ]]
    then SOURCE="src"
fi

cd $SOURCE

gcc utils.c network.c common.c threads.c client.c client_network.c -Wall -pthread -g -o $DEST/client;
gcc utils.c network.c common.c threads.c server.c server_network.c -Wall -pthread -g -o $DEST/server;