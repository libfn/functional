{ lib
, stdenv
, cmake
, ninja
, functional
, disableCxx23 ? false
}:

stdenv.mkDerivation {
  name = "functional-test-consumer${lib.optionalString disableCxx23 "-no-cxx23"}";

  src = lib.sourceByRegex ./. [
    "CMakeLists.txt"
    "^src.*"
  ];

  nativeBuildInputs = [ cmake ninja ];
  buildInputs = [ functional ];

  cmakeFlags = lib.optional disableCxx23 "-DTEST_DISABLE_CXX23=ON";

  doCheck = true;
  checkPhase = ''
    runHook preCheck
    ./pfn_quine
    ${lib.optionalString (!disableCxx23) "./fn_quine"}
    runHook postCheck
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    cp pfn_quine $out/bin/
    ${lib.optionalString (!disableCxx23) "cp fn_quine $out/bin/"}
    runHook postInstall
  '';
}
