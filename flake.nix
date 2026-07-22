{
  description = "qmk firmware";

  inputs = {
      nixpkgs.url = "flake:nixpkgs";
      flake-utils.url = "github:numtide/flake-utils";
      annepro2-tools = {
          url = "github:hitsmaxft/nix-annepro2-tools/master";
          inputs = {
              nixpkgs.follows = "nixpkgs";
          };
      };
  };

  outputs = { self, nixpkgs, flake-utils, annepro2-tools }:
    flake-utils.lib.eachDefaultSystem
      (system:
      let

          pkgs = nixpkgs.legacyPackages.${system};
          shell = import ./shell.nix { inherit pkgs; };
          annepro2Tools = (pkgs.callPackage annepro2-tools {
            pkgs = pkgs // { pkgconfig = pkgs.pkg-config; };
          }).overrideAttrs (_: {
            CARGO_HOME = "cargo-home";
          });
          annepro2ToolsQmk = pkgs.writeShellScriptBin "annepro2_tools" ''
            exec ${annepro2Tools}/bin/annepro2-tools "$@"
          '';
      in
        {
            devShells.default =  shell.overrideAttrs (old: {
                nativeBuildInputs = old.nativeBuildInputs ++ [
                  annepro2Tools
                  annepro2ToolsQmk
                    pkgs.addlicense
                    pkgs.license-cli
                    pkgs.just
                ];
            });
        }
      );
}
