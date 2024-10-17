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
, libpulseaudio
, libxkbcommon
, wayland
, wayland-protocols
, wayland-scanner
, autoPatchelfHook
, includeEverything ? true
, sharedLib ? true
, waylandSupport ? true
, externalGLFW ? true
, webPlatform ? false
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "raylib";
  version = "5.0";

  src = fetchFromGitHub {
    owner = "raysan5";
    repo = "raylib";
    rev = "c9c830cb971d7aa744fe3c7444b768ccd5176c4c";
    hash = "sha256-PHYdAEhittShd1UhASdhmq0nGHEEVZEUGcjCUUJZl9g=";
  };
  src5 = fetchFromGitHub {
    owner = "raysan5";
    repo = "raylib";
    rev = finalAttrs.version;
    hash = "sha256-gEstNs3huQ1uikVXOW4uoYnIDr5l8O9jgZRTX1mkRww=";
  };
  raygui = fetchFromGitHub {
    owner = "raysan5";
    repo = "raygui";
    rev = "38ababbc6285f90e46ed16db074aa64fdaf55ee7";
    hash = "sha256-q3Jb+45096XTxODeOlCg7TTQOV0F20jW8TYJbvz8ot8=";
  };
  raygui4 = fetchFromGitHub {
    owner = "raysan5";
    repo = "raygui";
    rev = "4.0";
    hash = "sha256-1qnChZYsb0e5LnPhvs6a/R5Ammgj2HWFNe9625sBRo8==";
  };

  nativeBuildInputs = [ pkg-config autoPatchelfHook ];

  buildInputs = [ glfw mesa libXi libXcursor libXrandr libXinerama alsa-lib libpulseaudio ]
    ++ lib.optionals waylandSupport [ wayland-protocols wayland-scanner ]
  ;

  propagatedBuildInputs = [ libGLU libX11 ]
    ++ lib.optionals waylandSupport [ wayland libxkbcommon ]
    ++ lib.optionals externalGLFW [ glfw ]
  ;

  patches = [ ./no-root-install.patch ./no-ldconfig.patch ];

  makeFlags =
    if webPlatform then [ "PLATFORM=PLATFORM_WEB" ]
    else [
      "PLATFORM=PLATFORM_DESKTOP"

      (if sharedLib
        then "RAYLIB_LIBTYPE=SHARED"
        else "RAYLIB_LIBTYPE=STATIC")

      # to use internal glfw, you must patch your binary with
      # postFixup = ''patchelf $out/bin/${pname} --add-needed {every library} --add-rpath ${lib.makeLibraryPath [wayland libxkbcommon]}''
      # or compile your binary with the compileFlags from passtrhu
      (if externalGLFW
        then "USE_EXTERNAL_GLFW=TRUE"
        else "USE_EXTERNAL_GLFW=FALSE")

      (if waylandSupport
        then "USE_WAYLAND_DISPLAY=TRUE GLFW_LINUX_ENABLE_WAYLAND=TRUE"
        else "USE_WAYLAND_DISPLAY=FALSE GLFW_LINUX_ENABLE_WAYLAND=FALSE")

      (lib.optionalString includeEverything 
        ''RAYLIB_MODULE_RAYGUI=TRUE RAYLIB_MODULE_RAYGUI_PATH="${finalAttrs.raygui}/src"'')
    ]
  ;

  installFlags = [
    "RAYLIB_RELEASE_PATH=./."
    "DESTDIR=$$out"
  ];

  preBuild = ''
    cd src
  '';

  passthru = {
    # With a certain configuration, we need to link against other libraries, insert ${raylib.compileFlags} inside your compilation command to get them automatically
    compileFlags = lib.concatStringsSep " " [

      "-lraylib" "-lGL" "-lm" "-lpthread" "-ldl" "-lrt"

      (lib.optionalString (sharedLib && waylandSupport && !externalGLFW)
        "-lwayland-cursor -lwayland-egl -lxkbcommon")

      (lib.optionalString (!sharedLib && externalGLFW)
        "-lglfw")

      (lib.optionalString (!sharedLib && !externalGLFW)
        (if waylandSupport 
          then "-lwayland-client -lxkbcommon"
          else "-lX11"))

      # leaner and meaner variant
      # (if waylandSupport 
      #   then "-lwayland-cursor -lwayland-egl -lxkbcommon"
      #   else "-lX11")
      # (lib.optionalString externalGLFW "-lglfw")
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


# Notes:
# - I'm not sure, but it seems that most '-l' compile flags are needed only with a static library, since the dynamic library probably already links to them by itself
# always needed: -lraylib
# static: -lm -lGL
  # external: -lglfw
  # internal wayland: -lwayland-client -lxkbcommon
  # internal X11: -lX11
# dynamic and internal and wayland: -lwayland-cursor -lwayland-egl -lxkbcommon

# it looks like linking with wayland-cursor also links automatically with wayland-client

