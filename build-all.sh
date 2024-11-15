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

gccret=$(cd src; ret="0"
# shellcheck disable=SC2291
while IFS= read -r cmd; do
  eval "$cmd"
  ret=$(( ret + $? ))
done < <(cat << 'EOPART' 
gcc -shared -o ../libsecoclient.so -fPIC -lraylib -lGL -lm -lpthread -ldl -lrt     -DPLUG_DEV -g -Wall  "${cmdLineCompileFlags[@]}" -DPLUG_FILE=\""./libsecoclient.so\"" common/utils.c common/common.c client/client_seco.c client/scenes/connection.c client/scenes/game.c client/client_main.c client/client_network.c;
gcc common/network.c common/threads.c client.c client_plug.c -lraylib -lGL -lm -lpthread -ldl -lrt     -DPLUG_DEV -g -Wall  "${cmdLineCompileFlags[@]}" -DPLUG_FILE="\"./libsecoclient.so\"" -export-dynamic -o ../client;
gcc common/network.c common/threads.c server.c server/server_network.c -lraylib -lGL -lm -lpthread -ldl -lrt     -DPLUG_DEV -g -Wall  "${cmdLineCompileFlags[@]}" -export-dynamic -o ../server; 
EOPART
)
echo $ret)

[[ "$gccret" == 0 && -n "$runExe" ]] && eval "$runExe"

exit "$gccret"

