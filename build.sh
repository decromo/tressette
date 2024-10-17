if [[ -z $DEST ]]
    then DEST="$(pwd)"
fi

if [[ -z $SOURCE ]]
    then SOURCE="src"
fi

if [[ -z $COMPFLAGS ]]
    then COMPFLAGS="-Wall -pthread -g -o"
fi

cd $SOURCE

echo "$COMPFLAGS"

gcc utils.c network.c common.c threads.c client.c client_network.c $COMPFLAGS $DEST/client;
gcc utils.c network.c common.c threads.c server.c server_network.c $COMPFLAGS $DEST/server;