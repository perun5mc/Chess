Szachy zrobione jako projekt na zajęcia z Podstaw Informatyki.

Komenda do kompilacji:
>g++ -Wall -Wextra -g3 -std=c++17 *.cpp -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows -o output.exe

Lub wariant, który nie ukrywa konsoli:
>g++ -Wall -Wextra -g3 -std=c++17 *.cpp -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm -o output.exe
