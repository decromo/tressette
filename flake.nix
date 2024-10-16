{
  description = "A raylib template flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
    lib = pkgs.lib;
    mkBin = name: pkgs.runCommandCC name { src = ./.; buildInputs = [ self.packages.${system}.raylib ]; }
    ''
      mkdir -p $out/bin
      pushd .
      cd $src
      DEST=/build $src/build.sh && popd; cp -t $out/bin ${name}
    '';

    commonMainFiles = lib.concatStringsSep " " [
    ];
    commonSecoFiles = lib.concatStringsSep " " [
      # "seco_server.c"
    ];
    serverMainFiles = lib.concatStringsSep " " [
      # "seco_client.c"
    ];
    serverSecoFiles = lib.concatStringsSep " " [
      "utils.c"
      "network.c"
      "common.c"
      "threads.c"
    ];
    clientMainFiles = lib.concatStringsSep " " [
      "server.c"
      "server_network.c"
    ];
    clientSecoFiles = lib.concatStringsSep " " [
      "client.c"
      "client_network.c"
    ];
  in {

    packages.${system} = rec {
      raylib-nixpkgs = pkgs.raylib;
      raylib-dynamic = pkgs.callPackage ./nix/raylib.nix { sharedLib = true; waylandSupport = true; externalGLFW = false; };
      raylib-rglfw = pkgs.callPackage ./nix/raylib.nix { sharedLib = false; waylandSupport = true; externalGLFW = false; };
      raylib-web = pkgs.callPackage ./nix/raylib.nix { webPlatform = true; };
      raylib-extglfw = (pkgs.callPackage ./nix/raylib.nix { sharedLib = false; waylandSupport = true; externalGLFW = true; } );

      raylib = raylib-nixpkgs;

      client = mkBin "client";

      server = mkBin "server";

      all = mkBin "client server";

      default = tressette;

      tressette = pkgs.stdenv.mkDerivation rec {
        pname = "tressette";
        version = "0.0.1";
        src = ./src;

        buildInputs = with pkgs; [
          self.packages.${system}.raylib
        ];

        buildPhase =
          let
            serverFiles = [
              commonMainFiles
              commonSecoFiles
              serverMainFiles
              serverSecoFiles
            ];
            clientFiles = [
              commonMainFiles
              commonSecoFiles
              clientMainFiles
              clientSecoFiles
            ];
          in ''
            gcc ${lib.concatStringsSep " " serverFiles}  -o s.out
            gcc ${lib.concatStringsSep " " clientFiles}  -o c.out
          '';

        installPhase = ''
          mkdir -p $out/bin
          cp s.out $out/bin/${pname}-server
          cp c.out $out/bin/${pname}-client
        '';
      };
    };

    devShells.${system}.default = 
    let
      raylib = self.packages.${system}.raylib;
      ccseco = pkgs.writeShellApplication {
        name = "ccseco";
        runtimeInputs = [ raylib ];
        text = ''
          cd src
          gcc -shared -o ../libsecoserver.so -fPIC -DPLUG_DEV_ON ${commonSecoFiles} ${serverSecoFiles} 
          gcc -shared -o ../libsecoclient.so -fPIC -DPLUG_DEV_ON ${commonSecoFiles} ${clientSecoFiles} 
        '';
      };
    in pkgs.mkShell {
      buildInputs = [ 
        raylib
        ccseco
        (pkgs.writeShellApplication {
          name = "ccmain";
          runtimeInputs = [ raylib ccseco ];
          text = ''
            (ccseco)
            cd src
            gcc -DPLUG_DEV_ON ${commonMainFiles} ${serverMainFiles}  -Wall -pthread -g -o ../server;
            gcc -DPLUG_DEV_ON ${commonMainFiles} ${clientMainFiles}  -Wall -pthread -g -o ../client;
          '';
        })
      ];
    };

  };
}
