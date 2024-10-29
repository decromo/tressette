{ 
  pkgs ? import <nixpkgs>
  , lib ? pkgs.lib
  , raylib ? pkgs.raylib.overrideAttrs { passthru.compileFlags = "-lraylib -lGL -lm -lpthread -ldl -lrt"; }
  , sourceFiles
  , extraCompilerArgs ? ""
}:

let

  inherit (sourceFiles) serverMainFilesString clientMainFilesString serverSecoFilesString clientSecoFilesString;

  compilerFlags = lib.concatStringsSep " " [
    raylib.compileFlags
    extraCompilerArgs
    "-DPLUG_DEV -g -Wall"
    ''-DPLUG_FILE="\"./libsecoclient.so\""''
    ''"''${cmdLineCompileFlags[@]}"''
  ];

  mainCompilerCommand = ''
      gcc ${serverMainFilesString} ${compilerFlags} -export-dynamic -o ../server;
      gcc ${clientMainFilesString} ${compilerFlags} -export-dynamic -o ../client;
  '';

  secoCompilerCommand = ''
      # gcc -shared -o ../libsecoserver.so -fPIC ${compilerFlags} ${serverSecoFilesString} 
      gcc -shared -o ../libsecoclient.so -fPIC ${compilerFlags} ${clientSecoFilesString} 
    '';

  # No need to touch the rest of this file

  compilingScript = compilerCommand: ''
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
    ${compilerCommand}
    echo $?)

    [[ "$gccret" == 0 && -n "$runExe" ]] && eval "$runExe"

    exit "$gccret"
  '';

  ccseco = pkgs.writeShellApplication {
    name = "ccseco";
    runtimeInputs = [ raylib ];
    text = compilingScript secoCompilerCommand;
  };

  ccmain = pkgs.writeShellApplication {
    name = "ccmain";
    runtimeInputs = [ raylib ];
    text = compilingScript mainCompilerCommand;
  };

  ccall = pkgs.writeShellApplication {
    name = "ccmain";
    runtimeInputs = [ raylib ];
    text = ccall-text;
  };

  ccall-text = compilingScript ''
    ${secoCompilerCommand}
    ${mainCompilerCommand}
  '';

  echo-ccall = pkgs.writeShellScriptBin "echo-ccall" ''
    cat << 'EOSCRIPT'
    ${ccall-text}
    EOSCRIPT
  '';

  entr-plug-client = pkgs.writeShellApplication {
    name = "entr-plug-client";
    runtimeInputs = [ pkgs.entr ccmain ccseco ];
    text = ''trap 'kill 0' INT; ccmain -r "./client" & while sleep 0.1; do find ./src -name '*.c' -or -name '*.h' | entr -cd ccseco; done'';
  };
in 
pkgs.mkShell {
  inherit 
    compilerFlags
    mainCompilerCommand
    secoCompilerCommand
    serverMainFilesString
    clientMainFilesString
    serverSecoFilesString
    clientSecoFilesString;

  buildInputs = [
    raylib
    ccseco
    ccmain
    ccall
    echo-ccall
    entr-plug-client
  ];
}