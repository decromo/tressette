{
  description = "A raylib template flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";

    raylib = self.packages.${system}.raylib-static-ext;
    raylib-dev = self.packages.${system}.raylib-dynamic-ext;

    commonMainFiles = [
    ];
    clientMainFiles = [
      "client.c"
      "client_plug.c"
    ];
    serverMainFiles = [
      "server.c"
      "server/server_network.c"
    ];
    commonSecoFiles = [
      "common/utils.c"
      "common/network.c"
      "common/common.c"
      "common/threads.c"
    ];
    serverSecoFiles = [
      # "seco_server.c"
    ];
    clientSecoFiles = [
      "client/client_seco.c"
      "client/scenes/connection.c"
      "client/scenes/game.c"

      "client/client_main.c"
      "client/client_network.c"
    ];
    
    # Not much needs to be changed down from here

    mkBin = { pname, files, extraCompilerArgs ? "", outName ? pname, raylibDrv ? raylib }:
      pkgs.stdenv.mkDerivation (finalAttrs: {
        inherit pname;
        version = "0.0.1";
        src = ./src;

        buildInputs = /* with pkgs; */ [ raylibDrv ];

        buildPhase = ''gcc ${files} ${raylibDrv.compileFlags} ${extraCompilerArgs}'';

        installPhase = ''
          mkdir -p $out/bin
          cp a.out $out/bin/${outName}
        '';
      });

    clientSecoFilesString = lib.concatStringsSep " " (commonSecoFiles ++ clientSecoFiles);
    serverSecoFilesString = lib.concatStringsSep " " (commonSecoFiles ++ serverSecoFiles);

    clientMainFilesString = lib.concatStringsSep " " (commonMainFiles ++ clientMainFiles);
    serverMainFilesString = lib.concatStringsSep " " (commonMainFiles ++ serverMainFiles);
    
    clientFilesString = "${clientMainFilesString} ${clientSecoFilesString}";
    serverFilesString = "${serverMainFilesString} ${serverSecoFilesString}";

    sourceFiles = { inherit clientSecoFilesString serverSecoFilesString clientMainFilesString serverMainFilesString; };

    callRaylib = pkgs.callPackage ./nix/raylib.nix;
    pkgs = import nixpkgs { inherit system; };
    lib = pkgs.lib;
  in {

    packages.${system} = rec {
      raylib-dynamic = callRaylib { sharedLib = true; externalGLFW = true; };
      raylib-dynamic-ext = callRaylib { sharedLib = true; externalGLFW = true; };
      raylib-static = callRaylib { sharedLib = false; externalGLFW = false; };
      raylib-static-ext = callRaylib { sharedLib = false; externalGLFW = true; };
      raylib-web = callRaylib { webPlatform = true; };
      raylib-X11 = callRaylib { sharedLib = true; externalGLFW = false; waylandSupport = false; };
      raylib-nixpkgs = pkgs.raylib.overrideAttrs { passthru.compileFlags = "-lraylib -lGL -lm -lpthread -ldl -lrt"; };

      client = mkBin { pname = "tressette-client"; files = clientFilesString; };
      server = mkBin { pname = "tressette-server"; files = serverFilesString; };

      tressette = pkgs.symlinkJoin { name = "tressette"; paths = [ client server ];};

      default = tressette;
    };

    devShells.${system}.default = import ./shell.nix { 
      raylib = raylib-dev;
      inherit pkgs sourceFiles;
    };

  };
}
