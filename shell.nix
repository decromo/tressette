{
  pkgs ? import <nixpkgs>,
  lib ? pkgs.lib,
  raylib ? pkgs.raylib.overrideAttrs {
    passthru.compileFlags = "-lraylib -lGL -lm -lpthread -ldl -lrt";
  },
  sourceFiles,
  extraCompilerArgs ? "",
}:

let

  inherit (sourceFiles)
    serverMainFilesString
    clientMainFilesString
    serverSecoFilesString
    clientSecoFilesString
    ;

  commonCompilerFlags = lib.concatStringsSep " " [
    raylib.compileFlags
    extraCompilerArgs
    "-DPLUG_DEV -g -Wall"
    ''''
    ''"''${cmdLineCompileFlags[@]}"''
  ];

  compilerCommands = {
    client = {
      main = ''gcc ${clientMainFilesString} ${commonCompilerFlags} -DPLUG_FILE="\"./libsecoclient.so\"" -export-dynamic -o ../client;'';
      seco = ''gcc -shared -o ../libsecoclient.so -fPIC ${commonCompilerFlags} -DPLUG_FILE=\""./libsecoclient.so\"" ${clientSecoFilesString};'';
    };
    server = {
      main = ''gcc ${lib.concatStringsSep " " [serverMainFilesString serverSecoFilesString]} ${commonCompilerFlags} -export-dynamic -o ../server;'';
      # seco = ''gcc -shared -o ../libsecoserver.so -fPIC ${commonCompilerFlags} -DPLUG_FILE="\"./libsecoserver.so\"" ${serverSecoFilesString};'';
    };
  };

  # No need to touch the rest of this file

  mainCompilerCommand = with lib;
    (pipe compilerCommands [
      attrValues
      zipAttrs
      (s: s.main or [])
      (concatStringsSep "\n")
    ]);
  secoCompilerCommand = with lib;
    (pipe compilerCommands [
      attrValues
      zipAttrs
      (s: s.seco or [])
      (concatStringsSep "\n")
    ]);

  serverCompilerCommand =
    lib.pipe compilerCommands.server [
      (lib.mapAttrsToList (n: v: v))
      (lib.concatStringsSep "\n")
    ];
  clientCompilerCommand =
    lib.pipe compilerCommands.client [
    (lib.mapAttrsToList (n: v: v))
    (lib.concatStringsSep "\n")
  ];

  compilingScript = exeName: compilerCommand: pkgs.writeShellApplication rec {
    name = "${exeName}";
    runtimeInputs = [ raylib ];
    derivationArgs.passthru.text = text;
    text = ''
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
      ${compilerCommand} 
      EOPART
      )
      echo $ret)

      [[ "$gccret" == 0 && -n "$runExe" ]] && eval "$runExe"

      exit "$gccret"
    '';
  };

  ccseco = compilingScript "ccseco" secoCompilerCommand;
  ccmain = compilingScript "ccmain" mainCompilerCommand;

  ccclient = compilingScript "ccclient" clientCompilerCommand;
  ccserver = compilingScript "ccserver" serverCompilerCommand;

  ccall = compilingScript "ccall" (lib.concatStringsSep "\n" [ secoCompilerCommand mainCompilerCommand ]);

  entr-plug-client = pkgs.writeShellApplication {
    name = "entr-plug-client";
    runtimeInputs = [
      pkgs.entr
      ccclient
      ccseco
    ];
    text = ''
      trap 'kill 0' INT
      while getopts r opt; do case $opt in
        r) run="1";;
        *) break;;
      esac; done;
      ccclient && if [[ -n ''${run:+yes} ]]; then
        echo yes
        ./client "$@" &
      fi
      sleep 1
      while sleep 0.1; do
        find ./src -name '*.c' -or -name '*.h' | entr -cd ccseco
      done'';
  };
in
pkgs.mkShell {
  inherit
    serverMainFilesString
    clientMainFilesString
    serverSecoFilesString
    clientSecoFilesString
    commonCompilerFlags
    mainCompilerCommand
    secoCompilerCommand
    clientCompilerCommand
    serverCompilerCommand
    ;

  buildInputs = [
    raylib
    ccseco
    ccmain
    ccclient
    ccserver
    ccall
    entr-plug-client
  ];

  shellHook = ''
    cat << 'EOHOOK' > build-all.sh
    ${ccall.text}
    EOHOOK
  '';
}
