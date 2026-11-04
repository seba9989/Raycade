{ pkgs, ... }:

{
  # Narzędzia dostępne w powłoce projektu
  packages = with pkgs; [
    raylib          # biblioteka graficzna (z pliku raylib-config.cmake korzysta CMake)
    cmake
    ninja
    pkg-config
    gdb             # debugowanie
    clang-tools     # clangd, clang-format, clang-tidy (linter)
    doxygen         # dokumentacja kodu (lab 6)
    graphviz        # diagramy hierarchii klas w Doxygenie

    # Edytor i serwery językowe, z których korzystają rozszerzenia Zeda
    zed-editor             # polecenie `zeditor`
    nixd                   # LSP dla plików .nix
    cmake-language-server  # LSP dla CMakeLists.txt
  ];

  languages.cplusplus.enable = true;

  # Skróty: wpisz w terminalu `build`, `run`, `lint`, `fmt`, `docs`, `clean`
  scripts.build.exec = ''
    cmake -S "$DEVENV_ROOT" -B "$DEVENV_ROOT/build" -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cmake --build "$DEVENV_ROOT/build"
    # clangd szuka compile_commands.json w katalogu głównym projektu
    ln -sf build/compile_commands.json "$DEVENV_ROOT/compile_commands.json"
  '';

  scripts.run.exec = ''
    build && "$DEVENV_ROOT/build/silnik" "$@"
  '';

  # Formatowanie wszystkich plików według .clang-format
  scripts.fmt.exec = ''
    find "$DEVENV_ROOT/src" -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \
      | xargs clang-format -i
  '';

  # Hook pre-commit: clang-format na plikach C/C++ dodanych do commita (.clang-format).
  # Jeśli coś przeformatuje, commit zostaje przerwany - wystarczy `git add` i commit ponownie.
  # Instaluje się sam przy wejściu do powłoki devenv; ręcznie: `pre-commit run -a`.
  git-hooks.hooks.clang-format = {
    enable = true;
    types_or = [ "c" "c++" ]; # domyślnie także JSON itp. - nie formatujemy .zed/*.json
  };

  # Dokumentacja HTML wg doxygen/Doxyfile -> doxygen/html/index.html
  scripts.docs.exec = ''
    cd "$DEVENV_ROOT"
    doxygen doxygen/Doxyfile
  '';

  scripts.edit.exec = ''
    zeditor "$DEVENV_ROOT"
  '';

  scripts.clean.exec = ''
    rm -rf "$DEVENV_ROOT/build" "$DEVENV_ROOT/compile_commands.json"
  '';

  enterShell = ''
    echo "Silnik 2D (raylib ${pkgs.raylib.version})"
    echo "  build  - konfiguracja i kompilacja"
    echo "  run    - kompilacja i uruchomienie"
    echo "  fmt    - formatowanie kodu (.clang-format, także hook pre-commit)"
    echo "  docs   - dokumentacja Doxygen"
    echo "  edit   - otwarcie projektu w Zedzie"
    echo "  clean  - usunięcie katalogu build"
  '';
}