# Normal Mapping

W tej części będziemy dalej modyfikować shadery **shader_5_tex** poprzez implementację normal mappingu.

Obliczenia dla map normalnych należy wykonywać w przestrzeni stycznych. Przestrzeń styczna jest wyliczana dla każdego punktu w obiekcie. Jej celem jest takie przekształcenie przestrzeni, żeby wektor normalny był wektorem jednostkowym (0,1,0). 

Do wyliczenia przestrzeni stycznej potrzebujemy dla każdego wierzchołka oprócz wektora normalnego wektor styczny i bistyczny (*tangent* i *bitangent*). Są one wyliczane przez bibliotekę `Assimp`. 

### Wykonaj kopię shaderów shader_4_tex.vert shader_4_tex.frag 

### Przenieś obliczenia światła do przestrzeni stycznej.

1) Oblicz macierz **TBN**.

   Macierz **TBN** to macierz 3x3 wyznaczana przez wektory *tangent*, *bitangent* i *normal*, służy do przenoszenia wektorów z przestrzeni świata do przestrzeni stycznej.

   1. W **vertex shaderze** przekrztałć wektory `vertexNormal`, `vertexTangent` i `vertexBitangent` do przestrzeni świata (przemnóż macierz modelu przez te wektory, tak jak to robiliśmy wcześniej z wektorem normalnym, z uwzględnieniem zmiennej w=0) i zapisz wyniki odpowiednio w zmiennych `normal`, `tangent` i `bitangent`.
    	1. Stwórz macierz 3x3 TBN jako transpose(mat3(tangent, bitangent, normal)). Macierz transponujemy, aby szybko uzyskać jej odwrotność (możemy tak zrobić przy założeniu, ze jest ortogonalna).

2. Przenieś wektor światła i wektor widoku do przestrzeni stycznych
    1. Musimy przekształcić wektor światła (L) i wektor widoku (V) do przestrzeni stycznych. Zrobimy to w vertex shaderze. W tym celu przenieś potrzebne dane dotyczące światła i kamery (uniformy `lightPos` i `cameraPos`) z **fragment shadera** do **vertex shadera.**

    2. Oblicz wektor `viewDir` jako znormalizowana różnice `cameraPos` i `fragPos` (tu jeszcze działamy w przestrzeni świata). Analogicznie oblicz `lightDir` jako różnicę `lightPos` i `fragPos`

    3. Przekształć wektory `viewDir` i `lightDir`  do przestrzeni stycznej mnożąc je przez macierz **TBN**. Wynik zapisz w zmiennych`viewDirTS` i `lightDirTS`  odpowiednio.

    4. Przekaż `viewDirTS` i `lightDirTS` do fragment shadera.  (zadeklaruj je jako zmienne `out`)

       > (Sufiks TS oznacza tangent space. Ważne jest, aby oznaczać (np. dopisując coś do nazwy zmiennej) w jakiej przestrzeni znajdują się używane wektory, tak aby poprawnie wykonywać obliczenia. Trzeba zawsze zwracać uwagę na to, w jakiej przestrzeni działamy.)

3. Przekształć **fragment shader**, by obsługiwał **tangent space**
    1. Nie potrzebujemy już we **fragment shaderze** informacji na temat pozycji fragmentu i wektora normalnego geometrii, skasuj wiec zmienne przekazujące te wektory pomiędzy shaderami.
    2. wektora `lightDir` powinniśmy użyć wektora `lightDirTS` (należy go dodatkowo znormalizować), a jako wektor widoku V powinniśmy użyć wektora `viewDirTS` (również należy go znormalizować). Jako wektora N użyj na razie wektora vec3(0,0,1).

Efekt finalny powinien wyglądać tak samo, jak przed jakąkolwiek zmianą. Następnym krokiem będzie wykorzystanie map normalnych.

### Wykorzystaj normalmapy

1. Chcemy wczytywać normalne z tekstury, w tym celu dodaj we **fragment shaderze** dodatkowy sampler do czytania map normalnych, nazwij go `normalSampler`. Pobierz z niego wektor normalny analogicznie, jak czytamy kolor zwykłej tekstury z samplera `textureSampler` i zapisz go w zmiennej `N`.
 	2. Ponieważ w teksturze wartości są w przedziale $[0,1]$, musimy jeszcze przekształcić je do przedziału $[-1,1]$. W tym celu przemnóż wektor N przez 2 i odejmij 1.
     Na koniec warto jeszcze znormalizować wektor normalny, aby uniknąć błędów związanych z precyzja lub kompresja tekstury.
 	3. Wczytaj pliki zawierające mapy normalnych w kodzie C++ W tym celu załaduj przy użyciu funkcji `Core::LoadTexture` mapy normalnych dla wszystkich modeli. Maja one taką samą nazwę jak zwykle tekstury, tyle że z suffiksem "_normals".
 	4. Zmodyfikuj na koniec funkcje `drawObjectTexture`. Dodaj do niej nowy argument `GLuint normalmapId`, który będzie służył do przekazywania id tekstury zawierającej mapę normalnych. Przy użyciu funkcji `Core::SetActiveTexture` załaduj `normalmapId` jako `normalSampler` i ustaw jednostkę teksturowania nr 1.
     Argument odpowiadający za normalne w miejscach wywołania funkcji `drawObjectTexture`.
  

### Zadanie*
Ustaw mapy normalne do statku planety i księżyca (lub przynajmniej 3 obiektów, jeżeli rysujesz swoją scenę). Wykorzystaj multitexturing na statku, musisz w takim wypadku mieszać zarówno tekstury koloru i normalanych.

## SkyBox
Cubemapy są specjalnym rodzajem tekstur. Zawieją one 6 tekstur, każda z niej odpowiada za inną ścianę sześcianu. Nie służy ona do teksturowania zwykłego sześcianu, pozwala ona bowiem próbkować po wektorze kierunku. To znaczy, możemy o tym myśleć jak o kostce, w której środku się znaleźliśmy, co obrazuje poniższy rysunek. W przeciwieństwie do zwykłych tekstur samplujemy ją nie za pomocą dwuwymiarowych współrzędnych UV, ale za pomocą wektora trójwymiarowego, który odpowiada kierunkowi promienia. 
![](./img/cubemaps_sampling.png)
Jednym z zastosowań Cubemapy jest wyświetlanie skyboxa, czyli dalekiego tła dla sceny. Może to być na przykład rozgwieżdżone niebo z górami na horyzoncie albo obraz dalekiej galaktyki. 

### Ładowanie cubemapy
Cubemapę generujemy podobnie jak inne tekstury, ale przy bindowaniu należy podać `GL_TEXTURE_CUBE_MAP`.
```C++
glGenTextures(1, &textureID);
glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
```
Skoro składa się ona z 6 tekstur, to należy każdą z nich załadować za pomocą `void glTexImage2D( 	GLenum target, ...)` `taget` wskazuje którą z tekstur ładujemy. Możliwe wartości rozpisane są w tabeli poniżej
|Layer number|Texture        target | Orientation|
|----|--------------------:|-----------:|
|0|`GL_TEXTURE_CUBE_MAP_POSITIVE_X` | Right | 
|1|`GL_TEXTURE_CUBE_MAP_NEGATIVE_X`|Left|
|2|`GL_TEXTURE_CUBE_MAP_POSITIVE_Y`|Top|
|3|`GL_TEXTURE_CUBE_MAP_NEGATIVE_Y`|Bottom|
|4|`GL_TEXTURE_CUBE_MAP_POSITIVE_Z`|Back|
|5|`GL_TEXTURE_CUBE_MAP_NEGATIVE_Z`|Front|

Możemy je ładować w pętli biorąc za kolejne targety `GL_TEXTURE_CUBE_MAP_POSITIVE_X+i`, ale należy pamiętać o powyższej kolejności. Poniższy kod ładuje do wszystkich 6 ścian tę samą teksturę, która znajduje się pod `filepath`. 

```C++

int w, h;
unsigned char *data;  
for(unsigned int i = 0; i < 6; i++)
{
	unsigned char* image = SOIL_load_image(filepath, &w, &h, 0, SOIL_LOAD_RGBA);
    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
        0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
    );
}
```
Na koniec pozostaje ustawić parametry opisujące zachowanie tekstury:
```C++
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  
```

### Zadanie
Napisz funkcję, która będzie ładować cubmapę bazującą na tablicy 6 stringów i załaduj do niej tekstury z foldera `skybox`.

### Rysowanie skyboxa
Skybox jest sześcianem, wewnątrz którego zamieszamy naszą scenę, przedstawia on dalekie tło, dzięki temu dostajemy iluzję głębi i przestrzeni. Do tego potrzebujemy narysować sześcian i narysować go odpowiednim shaderem. Rysowanie jest bardzo proste, polega wyłącznie na wyświetleniu koloru tekstury. Aby próbkować, teksturę potrzebujemy przesłać pozycję w przestrzeni modelu do shadera fragmentów. 

```C++
#version 430 core

layout(location = 0) in vec3 vertexPosition;

uniform mat4 transformation;

out vec3 texCoord;

void main()
{
	texCoord = vertexPosition;
	gl_Position = transformation * vec4(vertexPosition, 1.0);
}

```
> shader_skybox.vert 

W shaderze fragmentów wystarczy odebrać pozycję i próbkować za jej pomocą teksturę.

```C++
#version 430 core

uniform samplerCube skybox;

in vec3 texCoord;

out vec4 out_color;

void main()
{
	out_color = texture(skybox,texCoord);
}

```
> shader_skybox.frag

### Zadanie
W modelach znajduje się `cube.obj`, załaduj go i narysuj shaderami  `shader_skybox.vert` i `shader_skybox.vert`. Pamiętaj o przesłaniu macierzy transformacji i tekstury skyboxa. Aktywujemy ją za pomocą instrukcji:
```C++
glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
```

Skybox potencjalnie zasłania pewne obiekty, które są umieszczone trochę dalej. Wynika to z tego, że renderowanie go nadpisuje bufor głębokości. Dlatego narysuj cubemapę z wyłączonym testem głębokości na samym początku. Następnie włącz test głębokości dla reszty sceny. 
Dezaktywację wykonasz za pomocą instrukcji `glDisable(GL_DEPTH_TEST);`, natomiast aktywację za pomocą`glEnable(GL_DEPTH_TEST);`. 

Skybox reprezentuje obiekty, które są bardzo daleko. Tę iluzję możemy utracić, gdy kamera przysunie się zbyt blisko do skyboxa. Aby tego uniknąć, musimy tak umieścić skybox, by kamera zawsze była w jego środku. Przesuń skybox do pozycji kamery z użyciem macierzy translacji. 

