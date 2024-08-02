let
  # Import the older Nixpkgs snapshot
  oldPkgs = import (fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/5b22285b53d8c84e537d27fd7ddc3c9303584193.tar.gz";
  }) {};

in
oldPkgs.stdenv.mkDerivation {
  name = "development-environment";
  buildInputs = [ oldPkgs.gcc46 oldPkgs.glibc ];

    shellHook = ''
    export PATH=${oldPkgs.gcc46}/bin:$PATH
    export C_INCLUDE_PATH=${oldPkgs.glibc}/include
    export LIBRARY_PATH=${oldPkgs.glibc}/lib
  '';
}
