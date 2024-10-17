{ 
  pkgs ? import <nixpkgs>
  , lib ? pkgs.lib
  , raylib ? pkgs.raylib.overrideAttrs { passthru.compileFlags = "-lraylib -lGL -lm -lpthread -ldl -lrt"; }
  , sourceFiles ? {
    serverFiles = "";
    clientFiles = "";
    serverPlugFiles = "";
    clientPlugFiles = "";
  }
}:

let

  inherit (sourceFiles) serverFiles clientFiles serverPlugFiles clientPlugFiles;

  flags = "${raylib.compileFlags} -DPLUG_DEV -g -Wall";

  ccseco = pkgs.writeShellApplication {
    name = "ccseco";
    runtimeInputs = [ raylib ];
    text = ''
      cd src
      gcc -shared -o ../libsecoserver.so -fPIC ${flags} ${serverPlugFiles} 
      gcc -shared -o ../libsecoclient.so -fPIC ${flags} ${clientPlugFiles} 
    '';
  };

  ccmain = pkgs.writeShellApplication {
    name = "ccmain";
    runtimeInputs = [ raylib ccseco ];
    text = ''
      (ccseco)
      cd src
      gcc ${serverFiles} ${flags} -o ../server;
      gcc ${clientFiles} ${flags} -o ../client;
    '';
  };
in 
pkgs.mkShell {
  CFLAGS = flags;
  buildInputs = [
    raylib
    ccseco
    ccmain
  ];
}