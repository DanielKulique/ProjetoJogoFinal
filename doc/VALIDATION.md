# Validação da implementação integrada

Registro de 21/11/2026. A pasta de trabalho não possui `.git`; nenhum repositório foi inicializado e não houve commit, push ou deploy.

## Resultado e escopo

A implementação amplia o ciclo anterior com motores, dois aliens e FSM, explosões, asteroides/fragmentos, drops/itens/upgrades, gravidade, novos rounds e configuração central. O mapa padrão permanece livre de paredes e as classes acadêmicas são preservadas.

Os testes distinguem regras isoladas, integração de sistemas e execução gráfica. A prévia gráfica das mecânicas raras prepara um cenário por driver temporário dentro de `build/`, para não depender de esperar sorteios de minutos durante a inspeção; não altera o fluxo normal de produção.

## Execuções

Configuração, compilação, linkedição e execução das três variantes foram concluídas. Os alvos da aplicação foram compilados com `-Wall -Wextra -Wpedantic`, sem avisos; os sanitizadores não emitiram diagnósticos. A suíte foi executada com `DISPLAY` removido, confirmando que a lógica não exige janela.

| Verificação | Resultado registrado |
| --- | --- |
| Compilação/linkedição e CTest Debug sem display | **8/8 passaram**. |
| Compilação/linkedição e CTest Release sem display | **8/8 passaram**. |
| AddressSanitizer + UndefinedBehaviorSanitizer | **8/8 passaram**, sem diagnósticos. |
| Arena padrão `--smoke-test` | Três quadros e saída 0. |
| Procedural `--seed 42 --game-seed 9 --smoke-test` | Três quadros e saída 0. |
| CSV `assets/maps/maze.csv --game-seed 9 --smoke-test` | Três quadros e saída 0. |
| Entrada real X11 | WASD, mouse, botão de tiro e Esc exercitados na arena padrão. |
| Prévia integrada de mecânicas | Captura inspecionada com 3/4 motores, arma 3/3, ambos aliens/antena, raio gravitacional, itens distintos, tiros espalhados, asteroide, quatro fragmentos e explosão por colisão real entre avançado e rocha. |

A captura atual é `build/mechanics-window.png`; o driver temporário `build/MechanicsPreview.cpp` e scripts X11 ficam ignorados dentro de `build/`. A aplicação não depende de Python, Pillow ou Xlib Python. As inspeções visuais não constituem comparação automática de pixels.

## Cobertura automatizada

A configuração CTest contém oito casos: `mechanics_contracts`, `gameplay_contracts`, `map_contracts`, ajuda e quatro erros de CLI (semente procedural, semente de partida, modos conflitantes e CSV ausente).

### Mecânicas novas

`tests/MechanicsTests.cpp` organiza os cenários em grupos:

- Motores e arma: velocidades por cada quantidade, invulnerabilidade, reparação limitada, zero motores, valores configuráveis, cooldown/espalhamento e nível máximo.
- Percepção: normal navega sem conhecer alvo distante; detecção e perda mudam estado; avançado persegue desde o nascimento e tem maior caixa.
- Asteroides: trajetória, três/quatro filhos, tamanho/vida/profundidade, velocidade limitada, uso único, proteção inicial e expiração sem fragmentos.
- Gravidade: fórmula e conversão de unidades, centro/borda, aplicação gradual em corpo/jogador/alien/rocha/tiro, limites de deriva/velocidade e expiração.
- Drops: 20 mil decisões em dois jogos com a mesma semente, igualdade da sequência, frequência de 75% e seleção condicional 80/20, extremos 0/100%.
- Combate integrado: tiro em normal não gera item; tiro em avançado gera o único drop elegível sem explosão; contatos causam explosão uma vez, sem drops/dano entre aliens; rocha atingida produz filhos protegidos.
- Coleta: motor restaura um, upgrades param em três, itens são removidos uma vez e expiram.
- Rounds e distribuição: cinco/sete/nove até 25, intervalo mínimo, máximo ativo e quatro mil surgimentos posteriores reproduzíveis com distribuição 75/25.
- Ambiente: espera do intervalo completo, probabilidades extremas, máximo de rochas e uma área ativa, envelhecimento após round e ausência de geração com probabilidade zero.
- Regressões de integração: campo distante não freia projéteis, ambiente não impede portal, expiração não fragmenta, reinício restaura relógio/RNG, avançado nunca nasce com vida zero e caixa grande do avançado não bloqueia um normal válido no round introdutório.

O acesso de arranjo `GameTestAccess` é um amigo restrito aos testes, evitando acrescentar comandos de manipulação de entidades à API pública do jogo. Configura cenários e usa as regras reais de atualização/colisão para as asserções. Os testes não dependem de `assert`, mantendo suas verificações em Release.

### Regressões existentes

`GameplayTests.cpp` mantém mira, direção nula, integração fixa, diagonais, cadência, duração de projéteis, limites, AABBs, varredura contínua, consumo único de tiro rápido, parede antes do inimigo, spawn, portal, transição, derrota, reinício e navegação em mapas opcionais. As expectativas foram atualizadas para motores e novos valores de round; testes de perseguição universal usam explicitamente o alien avançado. O ciclo até o segundo round continua usando mira, tiro e movimento, sem reduzir vidas de inimigos artificialmente, e agora verifica a preservação de motores/upgrades.

`MapTests.cpp` foi preservado: CSV válido/inválido e transacional, dimensões, exterior, coordenadas, foco/fallback, A* e 601 arenas procedurais com conectividade, centro livre e reprodução por semente.

## Comandos reproduzíveis

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/space_arena --game-seed 42

# Linux, logica sem display
env -u DISPLAY ctest --test-dir build --output-on-failure

cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/release --parallel
env -u DISPLAY ctest --test-dir build/release --output-on-failure

# GCC/Clang, Linux
cmake -S . -B build/sanitize -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
cmake --build build/sanitize --parallel
env -u DISPLAY ctest --test-dir build/sanitize --output-on-failure

# Display/OpenGL necessarios
./build/space_arena --smoke-test
./build/space_arena --procedural --seed 42 --game-seed 9 --smoke-test
./build/space_arena --csv assets/maps/maze.csv --game-seed 9 --smoke-test
```

## Inventário desta etapa

| Arquivos criados | Finalidade |
| --- | --- |
| `source/Asteroid.hpp`, `source/Asteroid.cpp` | Corpo dinâmico, resistência, duração e fragmentos limitados. |
| `source/GravityField.hpp`, `source/GravityField.cpp` | Campo radial, unidades, suavização e duração. |
| `source/Pickup.hpp`, `source/Pickup.cpp` | Tipos de itens e tempo de coleta. |
| `source/GameConfig.hpp`, `source/GameConfig.cpp` | Agregação e validação central do balanceamento. |
| `source/Random.hpp` | Helpers de sorteio com RNG explícito e semente controlável. |
| `tests/MechanicsTests.cpp` | Testes novos de regras, probabilidades e integração. |

| Arquivos modificados | Mudanças |
| --- | --- |
| `source/Player.hpp/.cpp` | Motores configuráveis, reparação, dano e deriva. |
| `source/Weapon.hpp/.cpp` | Níveis limitados, maior cadência e rajada espalhada. |
| `source/Enemy.hpp/.cpp` | Tipos, FSM, percepção, destinos locais e deriva. |
| `source/WaveManager.hpp/.cpp` | Quantidade/cadência/atributos/limites e configuração por tipo. |
| `source/Game.hpp/.cpp` | Ambiente, aleatoriedade, explosões, autoria das mortes, drops, coleta e transições integradas. |
| `source/DungeonVisualizer.cpp` | Formas distintas/antena, motores e nível no HUD, itens, rochas, campo e explosão. |
| `source/Main.cpp` | `--game-seed` independente da semente do mapa e ajuda. |
| `CMakeLists.txt` | Novas implementações, suíte de mecânicas e teste de argumento da semente de partida. |
| `tests/GameplayTests.cpp` | Expectativas atualizadas e preservação de motores/upgrades ao atravessar portal. |
| `README.md`, `doc/ARCHITECTURE.md`, `doc/DEVELOPMENT_PLAN.md`, `doc/VALIDATION.md` | Mecânicas, fórmulas, conceitos acadêmicos, execução, cobertura e limites atuais. |

Matemática, integração de `RigidBody2D`, `Entity`, `NPC`, colisão contínua, mapas, árvore de comportamento, A*, o CSV do professor, fontes/licença e documentos de contratos não foram substituídos. `Projectile` continua reutilizado pelo fluxo de física e impacto, sem segundo sistema de integração.

## Correções e limites

As regressões novas protegem contra dano/drop/explosão repetidos, crescimento ilimitado de upgrades ou fragmentos, resistência gravitacional fora da região, perda de deriva ao aplicar WASD, descarte prematuro de pendentes e morte avançada com vida arredondada para zero. Uma caixa avançada maior não impede spawns normais válidos no round 1.

A apresentação é geométrica, sem áudio ou sprites finais. Corpos usam AABBs, aliens podem se sobrepor durante perseguição e fragmentos podem ser descartados quando não há capacidade ou espaço local seguro. A dispersão é física simplificada, sem conservação exata de momento/energia. CSVs muito pequenos ou isolados podem não comportar combate completo. Fonte e build não têm instalação relocável. A validação local usa Linux/GCC/SFML 2.6.1; outras plataformas precisam de execução própria.

Não há pendências de implementação para as mecânicas desta etapa na arena padrão. Os limites acima descrevem as aproximações e plataformas ainda não validadas.
