runExe=""
while getopts r: opt; do
  case $opt in
    r)
      runExe="$OPTARG"
      ;;
    *)
      break
      ;;
  esac
done
shift $((OPTIND - 1))

export cmdLineCompileFlags
cmdLineCompileFlags=("$@")

gccret=$(cd src;
gcc -shared -o ../libsecoclient.so -fPIC -lraylib -lGL -lm -lpthread -ldl -lrt     -DPLUG_DEV -g -Wall -DPLUG_FILE="\"./libsecoclient.so\"" "${cmdLineCompileFlags[@]}" common/utils.c common/network.c common/common.c common/threads.c client/client_seco.c client/scenes/connection.c client/scenes/game.c client/client_main.c client/client_network.c
# gcc -shared -o ../libsecoserver.so -fPIC -lraylib -lGL -lm -lpthread -ldl -lrt     -DPLUG_DEV -g -Wall -DPLUG_FILE="\"./libsecoclient.so\"" "${cmdLineCompileFlags[@]}" common/utils.c common/network.c common/common.c common/threads.c
gcc client.c client_plug.c -lraylib -lGL -lm -lpthread -ldl -lrt     -DPLUG_DEV -g -Wall -DPLUG_FILE="\"./libsecoclient.so\"" "${cmdLineCompileFlags[@]}" -export-dynamic -o ../client;
gcc server.c server/server_network.c -lraylib -lGL -lm -lpthread -ldl -lrt     -DPLUG_DEV -g -Wall -DPLUG_FILE="\"./libsecoclient.so\"" "${cmdLineCompileFlags[@]}" -export-dynamic -o ../server;

echo $?)

[[ "$gccret" == 0 && -n "$runExe" ]] && eval "$runExe"

exit "$gccret"

