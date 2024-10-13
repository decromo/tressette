{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs;
  in {
    packages.${system} = {
      raylib-shared = 
      raylib-static = 
      raylib-web = 
      default = pkgs.hello;
    };

    devShells.${system}.default = pkgs.mkShell {
      nativeBuildInputs = with pkgs; [ 
        raylib
      ] ++ (builtins.attrValues self.packages.${system});
    };

  };
}
