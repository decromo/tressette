export projectName
projectName="client"

export flags
flags="-DPLUG_DEV -lraylib -lGL -lm -lpthread -ldl -lrt"

export plugfiles
plugfiles="seco_client.c"
export mainfiles
mainfiles="utils.c network.c common.c threads. client.c client_network.c"


export workdir
workdir="$(pwd)"

dirs -c
pushd src > /dev/null
export srcdir
srcdir="$(pwd)"

export TEMPDIRECTORY
TEMPDIRECTORY="$(mktemp -dp . tmp-src.XXXX)"

find . -name '*.c' -or -name '*.h' \
| grep -v 'tmp/' \
| xargs -I {} sh -c \
" mkdir -p \"$TEMPDIRECTORY\"/\"\$(dirname '{}')\" \
&& echo '#define PLUG_FILE \"./libplug.so\"' \
| cat - '{}' \
> \"$TEMPDIRECTORY\"/'{}' \
" > /dev/null 2>&1

(cd "$TEMPDIRECTORY";
  gcc -shared -o "$workdir/libplug.so" -fPIC $flags $plugfiles
  gcc $mainfiles $flags -o "$workdir/$projectName"
)

rm -rf "$TEMPDIRECTORY"
pushd > /dev/null

if [[ -n $1 && "$1" == "run" ]]; then
  shift; exec "$workdir/$projectName" "$@"
fi