# Projeto Jogo Final

Base em **C++17 e SFML 2.x** para desenvolver um novo jogo a partir dos contratos e implementações do projeto da disciplina de Desenvolvimento de Jogos.

O projeto está na etapa de estruturação: reúne os componentes reutilizáveis, sem regras de partida, interface, mapa ou recursos do jogo anterior. Ainda não há `main()` nem executável jogável.

## Estrutura

```text
.
├── assets/     # Recursos do novo jogo; inicialmente apenas .gitkeep
├── build/      # Saída de compilação; inicialmente apenas .gitkeep
├── doc/        # Documentação das classes matemáticas, física e colisões
├── source/     # Cabeçalhos de contrato (.hpp) e implementações (.cpp)
└── README.md
```

Os arquivos `.gitkeep` mantêm as pastas `assets/` e `build/` no Git enquanto não há conteúdo. Nenhum recurso gráfico ou binário do jogo anterior faz parte desta base.

## Componentes disponíveis

| Componente | Responsabilidade |
| --- | --- |
| `Global.hpp` | Identificador de entidade e constantes `EPSILON` e `TILE_SIZE` |
| `Vector2D` | Posições, direções e operações vetoriais |
| `Transform2D` | Transformações afins: translação, rotação, escala e inversa |
| `RigidBody2D` | Posição, velocidade, aceleração e integração por Euler semi-implícito |
| `Collision2D` / `AABB` | Caixas de colisão e teste de interseção |
| `Entity` | Corpo físico, forma de colisão, identificador e estado ativo |
| `NPC` | Entidade com textura própria e sprite SFML |
| `BehaviorTree` | Condições, ações, sequências e seletores para tomada de decisão |
| `AStar` | Busca de caminho mínimo em uma grade com quatro vizinhos |

Os nomes originais `NCP.cpp` (implementação de `NPC`) e `BeheviorTree.cpp` (implementação de `BehaviorTree`) foram preservados.

## Contratos principais

- Entidades usam posições pelo **centro, em pixels**. A forma de colisão guarda metade da largura e da altura.
- `TILE_SIZE` vale 64 pixels. Bordas em contato contam como interseção entre AABBs.
- `RigidBody2D::integrate(dt)` recebe um intervalo positivo em segundos; o futuro loop do jogo deverá controlar o passo de simulação.
- `Transform2D` usa vetores-linha; `A * B` aplica primeiro `A`, depois `B`.
- `BehaviorTree` reavalia a raiz a cada `tick`, sem memória entre chamadas, e retorna um comando textual a partir de um contexto de fatos inteiros.
- `findPathAStar()` recebe índices inteiros de células em uma grade retangular: `.` indica espaço livre e `#`, parede. O caminho inclui início e destino; um resultado vazio indica entrada inválida ou ausência de caminho.
- `NPC` não permite cópia, para preservar a relação entre o sprite e sua textura.

Os cabeçalhos em `source/` detalham os contratos. A pasta `doc/` contém explicações de `Vector2D`, `Transform2D`, `RigidBody2D` e `Collision2D`.

## Dependências e verificação

É necessário um compilador com suporte a C++17. O componente `NPC` depende dos cabeçalhos de **SFML 2.x**; a base foi verificada com SFML 2.6.1.

Na raiz do projeto, verifique a sintaxe de todas as implementações:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Isource -fsyntax-only source/*.cpp
```

Esse comando não gera binários nem abre janelas. Em instalações não padrão, acrescente os caminhos de inclusão da SFML. Ao criar o executável do novo jogo, será necessário adicionar o ponto de entrada e vincular os módulos SFML usados, como `sfml-graphics`, `sfml-window` e `sfml-system`.

## Próxima etapa

Definir a proposta do novo jogo e implementar seu ponto de entrada, regras e apresentação sobre esta base. Os recursos próprios ficarão em `assets/`, e os arquivos gerados pela compilação, em `build/`.
