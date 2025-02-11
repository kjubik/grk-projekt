# Raport końcowy
## Opis użytych metod

- [Symulacja ptaków](https://github.com/kjubik/grk-projekt/blob/dev/cw%207/src/boids/Boid.h) – Algorytm Boids
- [Proceduralne generowanie terenu](https://github.com/kjubik/grk-projekt/blob/dev/cw%207/src/boids/Terrain.h) – Szum Perlina
- [Detekcja kolizji między ptakami, a terenem](https://github.com/kjubik/grk-projekt/blob/dev/cw%207/src/boids/Boid.h#L113) – Axis-Aligned Bounding Boxes
- [Shadow mapping](https://github.com/kjubik/grk-projekt/tree/dev/cw%207/shaders) - Percentage Closer Filtering 
- [Normal mapping](https://github.com/kjubik/grk-projekt/tree/dev/cw%207/shaders) - Tekstura z normalną 

## Dodatkowe funkcje
### Model Boid’a
Wykonany w Blenderze, eksportowany jako plik `.obj`.
### Proceduralnie generowany teren
Oparty na szumu Perlina, generowany jednorazowo dla obszaru symulacji.

## Sterowanie w symulacji
- WASD: podstawowy ruch wzdłuż dwóch poziomych osi
- Spacja: przesunięcie kamery w górę wzdłuż osi pionowej
- Lewy Control: przesunięcie kamery w dół wzdłuż osi pionowej
- Escape: Przełączenie widoczności kursora
- 1: Przełączenie shaderów terenu (wył/wł. normal mapping oraz shadow mapping)
- 2: Przełączenie shaderów boidów (wył/wł. światło oraz tekstury boidów)
- 3: Przełączenie widoczności bounding boxa boidów

## Podział pracy
### Maja Cytrycka
Odpowiedzialna za:
- normal mapping
- shadow mapping
- interaktywność
### Maciej Kmieć
Odpowiedzalny za:
- algorytm Boids
- detekcja kolizji
- interaktywność
### Wojciech Kubicki
Odpowiedzialny za:
- proceduralnie genereowany teren
- kompozycja wizualna aplikacji
- interaktywność

Członkowie zespołu wzajemnie przeprowadzali review pull requestów innych członków zespołu, sprawdzając czytelność kodu, sugerując poprawki wydajności i testując kompatybilność symulacji między systemami operacyjnymi (Windows-Linux).

## Napotkane wyzwania
Poniżej wymienione zostały natrudniejsze problemy z jakimi zmagaliśmy się podczas tworzenia projektu.
### Usunięcie shadow acne
Cały teren posiadał paski równoległych cieni. Dobranie odpowiednich wartości przy wyliczanu bias wewnątrz shadera rozwiązało problem, wyznaczjąc dolny limit, od którego cienie są nakładane.
### Całkowite zacieniowanie terenu położonego daleko od źródła światła
Teren położny w odległości większej od pewnej wartości początkowo zacieniowany był całości. Odpowiednie zwięszkenie parametrów rzutu ortogonalnego pomogło rozszerzyć obszar bazy ortogonalnej, co rozwiązało problem.
### Poprawna translacja siatki terenu wraz z atrybutami wierzchołków
Implementacja poprawnego umiejscowienia naszego proceduralnego terenu okazała się wymagająca ze względu na konieczność zachowania spójności atrybutów wierzchołków, takich jak normalne i współrzędne UV. Odpowiednie operacje na tych danych umożliwiły stworzenie metody translacji terenu o wektor z zachowaniem poprawnych wartości atrybutów wierzchołków.
## Wnioski końcowe
Podczas pracy nad projektem i w trakcie całego semestru zdobyliśmy cenne doświadczenie w implementacji animacji trójwymiarowych. Zrozumieliśmy, jak wymagające jest tworzenie nawet prostych scen – wymaga to zarówno solidnej wiedzy matematycznej, jak i umiejętności programowania w C++. Ten projekt pozwolił nam lepiej docenić wysoką jakość współczesnych metod grafiki komputerowej.

