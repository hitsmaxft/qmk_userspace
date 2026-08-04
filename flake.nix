{
  description = "qmk firmware";

  inputs = {
      nixpkgs.url = "flake:nixpkgs";
      flake-utils.url = "github:numtide/flake-utils";
      annepro2-tools = {
          # Hardened IAP transport: device-reported bases, strict reply/status
          # checks, bounded timeouts, and read-only probing.
          url = "git+https://github.com/hitsmaxft/nix-annepro2-tools.git?ref=refs/heads/codex/ble-iap-hardening";
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
          agentPython = pkgs.python312.withPackages (pythonPackages: [
            pythonPackages.unicorn
          ]);
          agentElfBinutils = pkgs.pkgsCross.gnu64.buildPackages.binutils;
          agentElfTools = pkgs.symlinkJoin {
            name = "annepro2-agent-elf-tools";
            paths = [
              (pkgs.writeShellScriptBin "elf-nm" ''
                exec ${agentElfBinutils}/bin/x86_64-unknown-linux-gnu-nm "$@"
              '')
              (pkgs.writeShellScriptBin "elf-objdump" ''
                exec ${agentElfBinutils}/bin/x86_64-unknown-linux-gnu-objdump "$@"
              '')
              (pkgs.writeShellScriptBin "elf-readelf" ''
                exec ${agentElfBinutils}/bin/x86_64-unknown-linux-gnu-readelf "$@"
              '')
            ];
          };
          # Agent-only reverse-engineering and packaging tools. Keep these in
          # the default shell so `direnv exec . ...` never falls back to
          # Homebrew or host-global Rust/Node installations.
          agentTools = with pkgs; [
            cargo
            rustc
            nodejs
            nodePackages.asar
            unzip
            zip
            p7zip
            jq
            file
            xxd
            ghidra
            agentElfBinutils
            agentElfTools
            agentPython
          ] ++ lib.optionals stdenv.isDarwin [
            lldb
          ];
          annepro2Tools = annepro2-tools.packages.${system}.default;
      in
        {
            devShells.default =  shell.overrideAttrs (old: {
                nativeBuildInputs = old.nativeBuildInputs ++ [
                  annepro2Tools
                  pkgs.addlicense
                  pkgs.license-cli
                  pkgs.just
                ] ++ agentTools;
            });
        }
      );
}
