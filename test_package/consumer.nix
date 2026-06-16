{ lib
, stdenv
, cmake
, ninja
, libfn
, disableCxx23 ? false
, cxxStd ? null
}:

stdenv.mkDerivation {
  name = "libfn-test-consumer${lib.optionalString disableCxx23 "-no-cxx23"}";

  src = lib.sourceByRegex ./. [
    "CMakeLists.txt"
    "^src.*"
  ];

  nativeBuildInputs = [ cmake ninja ];
  buildInputs = [ libfn ];

  cmakeFlags = lib.optional (cxxStd != null) "-DCMAKE_CXX_STANDARD=${toString cxxStd}";

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
