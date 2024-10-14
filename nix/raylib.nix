{ stdenv
, lib
, fetchFromGitHub
, mesa
, libGLU
, glfw
, libX11
, libXi
, libXcursor
, libXrandr
, libXinerama
, pkg-config
, alsa-lib
, autoPatchelfHook
, libpulseaudio
, sharedLib ? true
, includeEverything ? true
, webPlatform ? false
, waylandSupport ? true
, externalGLFW ? true
, libxkbcommon
, wayland
, wayland-protocols
, wayland-scanner
, egl-wayland
, glfw-wayland
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "raylib";
  version = "5.0";

  src = fetchFromGitHub {
    owner = "raysan5";
    repo = "raylib";
    rev = finalAttrs.version;
    hash = "sha256-gEstNs3huQ1uikVXOW4uoYnIDr5l8O9jgZRTX1mkRww=";
  };

  nativeBuildInputs = [ pkg-config autoPatchelfHook ];

  buildInputs = [ glfw mesa libXi libXcursor libXrandr libXinerama alsa-lib libpulseaudio ]
    ++ lib.optionals waylandSupport [ wayland-protocols wayland-scanner ];

  propagatedBuildInputs = [ libGLU libX11 ]
    ++ lib.optionals waylandSupport [ wayland libxkbcommon ]
    ++ lib.optionals externalGLFW [ glfw ]
  ;

  patches = [ ./no-root-install.patch ./no-ldconfig.patch ];

  raygui = fetchFromGitHub {
    owner = "raysan5";
    repo = "raygui";
    rev = "4.0";
    hash = "sha256-1qnChZYsb0e5LnPhvs6a/R5Ammgj2HWFNe9625sBRo8=";
  };


  makeFlags = 
    (if webPlatform 
    then ["PLATFORM=PLATFORM_WEB"]
    else [
      "PLATFORM=PLATFORM_DESKTOP"
      (if sharedLib then "RAYLIB_LIBTYPE=SHARED" else "RAYLIB_LIBTYPE=STATIC")
      # to use internal glfw, you must patch your binary with
      # postFixup = ''patchelf $out/bin/${pname} --add-needed {every library} --add-rpath ${lib.makeLibraryPath [wayland libxkbcommon]}''
      # or compile your binary with the compileFlags from passtrhu
      (if externalGLFW then "USE_EXTERNAL_GLFW=TRUE" else "USE_EXTERNAL_GLFW=FALSE")
      (if waylandSupport then "USE_WAYLAND_DISPLAY=TRUE" else "USE_WAYLAND_DISPLAY=FALSE")
    ])
    ++ lib.optionals includeEverything ["RAYLIB_MODULE_RAYGUI=TRUE" ''RAYLIB_MODULE_RAYGUI_PATH="${finalAttrs.raygui}/src"'']
  ;

  installFlags = [
    "RAYLIB_RELEASE_PATH=./."
    "DESTDIR=$$out"
  ];

  preBuild = ''
    pwd
    cd src
  '';

  preInstall = '' pwd '';

  passthru = {
    compileFlags = lib.concatStringsSep " " [
      "-lraylib" "-lGL" "-lm" "-lpthread" "-ldl" "-lrt"
      (lib.optionalString externalGLFW "-lglfw")
      (lib.optionalString waylandSupport "-lwayland-client -lwayland-cursor -lwayland-egl -lxkbcommon")
    ];
  };

  meta = with lib; {
    description = "A simple and easy-to-use library to enjoy videogames programming";
    homepage = "https://www.raylib.com/";
    license = licenses.zlib;
    platforms = platforms.all;
    changelog = "https://github.com/raysan5/raylib/blob/${finalAttrs.version}/CHANGELOG";
  };
})