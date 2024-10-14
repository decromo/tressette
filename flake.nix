{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
    mkBin = name: pkgs.runCommandCC name { src = ./.; } ''
      mkdir -p $out/bin
      pushd .
      cd $src
      DEST=/build $src/build.sh && popd; cp -t $out/bin ${name}
    '';
  in {
    packages.${system} = rec {
      raylib-shared = pkgs.callPackage ./nix/raylib.nix { sharedLib = true; };
      raylib-static = pkgs.callPackage ./nix/raylib.nix { sharedLib = false; };
      raylib-web = pkgs.callPackage ./nix/raylib.nix { webPlatform = true; };
      raylib-nixpkgs = pkgs.raylib;

      client = mkBin "client";

      server = mkBin "server";

      all = mkBin "client server";

      default = client;
    };

    devShells.${system}.default = pkgs.mkShell {
      nativeBuildInputs = with self.packages.${system}; [ 
        raylib-static
        client
      ];
    };

  };
}
