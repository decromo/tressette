{
  description = "A raylib template flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";

    raylib = self.packages.${system}.raylib-static-ext;

    commonMainFiles = [
      "utils.c"
      "network.c"
      "common.c"
      "threads.c"
    ];
    clientMainFiles = [
      "client.c"
      "client_network.c"
    ];
    serverMainFiles = [
      "server.c"
      "server_network.c"
    ];
    commonSecoFiles = [
    ];
    serverSecoFiles = [
      # "seco_server.c"
    ];
    clientSecoFiles = [
      # "seco_client.c"
    ];
    
    # Not much needs to be changed from here down

    mkBin = { pname, files, extraCompilerArgs ? "", outName ? pname, raylibDrv ? raylib }:
      pkgs.stdenv.mkDerivation (finalAttrs: {
        inherit pname;
        version = "0.0.1";
        src = ./src;

        buildInputs = with pkgs; [ raylibDrv ];

        buildPhase = ''gcc ${files} ${raylibDrv.compileFlags} ${extraCompilerArgs}'';

        installPhase = ''
          mkdir -p $out/bin
          cp a.out $out/bin/${outName}
        '';
      });

    clientPlugFiles = lib.concatStringsSep " " (commonSecoFiles ++ clientSecoFiles);
    serverPlugFiles = lib.concatStringsSep " " (commonSecoFiles ++ serverSecoFiles);

    clientFiles = clientPlugFiles + (lib.concatStringsSep " " (commonMainFiles ++ clientMainFiles ));
    serverFiles = serverPlugFiles + (lib.concatStringsSep " " (commonMainFiles ++ serverMainFiles ));

    sourceFiles = { inherit clientPlugFiles serverPlugFiles clientFiles serverFiles; };

    callRaylib = pkgs.callPackage ./nix/raylib.nix;
    pkgs = import nixpkgs { inherit system; };
    lib = pkgs.lib;
  in {

    packages.${system} = rec {
      raylib-dyn = callRaylib { sharedLib = true; externalGLFW = true; };
      raylib-dyn-ext = callRaylib { sharedLib = true; externalGLFW = true; };
      raylib-static = callRaylib { sharedLib = false; externalGLFW = false; };
      raylib-static-ext = callRaylib { sharedLib = false; externalGLFW = true; };
      raylib-web = callRaylib { webPlatform = true; };
      raylib-X11 = callRaylib { sharedLib = true; externalGLFW = true; waylandSupport = false; };
      raylib-nixpkgs = pkgs.raylib.overrideAttrs { passthru.compileFlags = "-lraylib -lGL -lm -lpthread -ldl -lrt"; };

      client = mkBin { pname = "tressette-client"; files = clientFiles; };
      server = mkBin { pname = "tressette-server"; files = serverFiles; };

      tressette = pkgs.symlinkJoin { name = "tressette"; paths = [ client server ];};

      default = tressette;
    };

    devShells.${system}.default = import ./shell.nix { inherit pkgs raylib sourceFiles; };

  };
}
