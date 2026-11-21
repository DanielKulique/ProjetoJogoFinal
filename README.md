# Space Arena

Jogo 2D em **C++17 e SFML 2.6.1**, desenvolvido para a disciplina de Desenvolvimento de Jogos da UTFPR, professor André Koscianski.

A nave enfrenta aliens normais e avançados, desvia de asteroides, sofre atração gravitacional, coleta motores e melhorias de tiro e avança pelo portal entre rounds. A arena padrão começa **totalmente livre de paredes**, com 31 × 23 células de 64 pixels. Os modos CSV e procedural continuam opcionais.

## Compilar e executar

Requisitos: CMake ≥ 3.16, compilador C++17 e **SFML 2.6.1 exata**, módulos graphics/window/system. O projeto não baixa dependências automaticamente.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
./build/space_arena
```

Para SFML instalada em outro caminho, acrescente `-DCMAKE_PREFIX_PATH=/caminho/da/instalacao`. A fonte DejaVu Sans e sua licença estão em `assets/fonts/` e são copiadas pelo CMake. O executável encontra a fonte independentemente do diretório atual; ao mover o build, reconfigure o CMake para atualizar o caminho.

## Controles e ciclo de jogo

| Controle | Ação |
| --- | --- |
| W / A / S / D | Mover; diagonais têm a mesma velocidade |
| Mouse | Apontar a nave usando a câmera atual |
| Botão esquerdo mantido | Disparar respeitando a cadência |
| R após derrota | Reiniciar motores, arma, rounds, relógio e semente |
| Setas | Navegar a câmera por pisos |
| F / Home | Enquadrar arena / restaurar escala 1:1 |
| Esc / fechar janela | Encerrar |

O HUD informa **motores, nível de tiro, round e aliens restantes**, incluindo os ainda pendentes. A nave começa com quatro motores. Cada dano aceito remove um motor e concede 1,25 s de invulnerabilidade. As velocidades com 4/3/2/1 motores são 250/210/170/130 pixels/s. Zero motores encerra a partida; o motor coletado recupera um, até quatro.

Aliens normais navegam autonomamente e começam a perseguir ao detectar a nave a 220 pixels; perdem o alvo além de 280 pixels. Aliens avançados têm antena, maior tamanho e resistência e perseguem desde o nascimento. Eles explodem ao tocar a nave ou um asteroide, causando dano dentro de 110 pixels e podendo fragmentar rochas. A explosão não atinge outros aliens. Eliminar um avançado por tiros não provoca explosão.

Somente avançados eliminados por tiros podem deixar itens: primeiro há **75%** de chance de drop; se houver, outro sorteio escolhe **80% melhoria de tiro / 20% motor**. Os itens surgem em piso válido aleatório e expiram após 20 s. O ícone de motor difere do upgrade. A arma vai até o nível 3: tiro simples → maior cadência → três tiros espalhados. O nível 3 é o limite.

O round 1 possui cinco normais. Os seguintes têm 7, 9, 11… até 25 aliens, com 75% normais e 25% avançados por surgimento. O intervalo inicial de 2 s cai 0,1 s por round até 0,5 s; no máximo oito aliens ficam ativos simultaneamente. O primeiro pode surgir imediatamente. Quantidade, intervalos, atributos, limites e probabilidades estão em [GameConfig.hpp](source/GameConfig.hpp) e nas configurações que ele agrega.

Quando não há aliens vivos **nem pendentes**, surge o portal violeta no centro. Entrar inicia o round seguinte na mesma arena, preservando motores e upgrades. Caso a nave já esteja sobre ele quando aparecer, saia e entre novamente. Asteroides e gravidade não impedem a conclusão, mas continuam ativos durante a espera pelo portal. Perder o foco pausa o tempo de jogo.

## Asteroides e gravidade

A cada 5 s há uma tentativa com 10% de chance de gerar asteroide. O limite padrão é 20 corpos, contando fragmentos. Rochas seguem trajetórias retilíneas, rebatem nos limites/paredes opcionais e podem ser destruídas por tiros, explosões ou colisões destrutivas. Uma rocha grande se divide em quatro fragmentos menores; são configuráveis três ou quatro. Fragmentos têm proteção inicial de 0,3 s e não se subdividem no nível máximo padrão 1. Asteroides expiram após 45 s sem fragmentar.

A cada minuto completo de tempo de jogo, se não houver campo ativo, há 50% de chance de criar uma região gravitacional. Ela dura 20 s, tem raio visível de 260 pixels e influencia nave, aliens, asteroides e projéteis. A hipótese física é **10 m/s²**, com **32 pixels/m**, resultando em até 320 pixels/s² antes da atenuação. A atração decai perto do limite e no centro; amortecimento e limites de deriva evitam crescimento ilimitado da velocidade. Fórmulas e decisões estão na [arquitetura](doc/ARCHITECTURE.md).

## Reprodução e mapas opcionais

```sh
./build/space_arena --game-seed 42
./build/space_arena --procedural --seed 42 --game-seed 7
./build/space_arena --csv assets/maps/maze.csv --game-seed 7
./build/space_arena --help
```

`--game-seed` controla aliens, drops e eventos; seu padrão é 1234. É independente de `--seed`, que controla o gerador procedural. Ambas aceitam inteiros de 0 a 4294967295. `--seed` sozinho seleciona o modo procedural; CSV não pode ser combinado com `--seed` ou `--procedural`. `--game-seed` funciona com qualquer mapa. Reiniciar repõe a semente inicial.

CSV usa `0` para piso e `1` para parede, separados por vírgulas, sem cabeçalho e com largura constante. Espaços externos e CRLF são aceitos. Caminhos são relativos ao diretório atual. O CSV de 40 × 30 do professor permanece preservado.

Corpos e projéteis respeitam paredes opcionais. O A* existente é usado apenas quando o mapa contém paredes; a arena livre usa vetores diretamente. Spawns e drops usam pisos alcançáveis, com a área do corpo inteira livre. Se o centro de um CSV estiver bloqueado ou desconectado, o portal usa o piso alcançável mais próximo. Mapas mínimos sem espaço para os corpos podem manter surgimentos pendentes; uma única célula não comporta o combate completo.

## Testes

```sh
ctest --test-dir build --output-on-failure
# Linux: verificar logica sem display
env -u DISPLAY ctest --test-dir build --output-on-failure

# Precisam de display e OpenGL
./build/space_arena --smoke-test
./build/space_arena --procedural --seed 42 --smoke-test
./build/space_arena --csv assets/maps/maze.csv --smoke-test
```

`--smoke-test` renderiza três quadros e encerra normalmente. A suíte de lógica verifica os mapas originais, mira/movimento/combate/portal, motores, FSM dos aliens, upgrades, explosões, fragmentação, gravidade, distribuição com semente, coleta, limites e ausência de danos/drops duplicados. Resultados efetivamente executados e comandos completos estão em [VALIDATION.md](doc/VALIDATION.md).

## Organização e limites

`DungeonVisualizer` mantém o único Game Loop e o passo fixo de 1/60 s. `Game` possui entidades e centraliza regras; `Player` compõe `Weapon`. `Entity`, `NPC`, matemática, física, colisões, `Dungeon2D`, `TileMap`, `BehaviorTree` e A* preservam a organização acadêmica. A IA inicial usa FSM explícita.

Veja [arquitetura e conceitos da disciplina](doc/ARCHITECTURE.md) e [etapas e pendências](doc/DEVELOPMENT_PLAN.md). A apresentação usa formas simples e não inclui áudio. Colisões de corpos usam AABBs; aliens podem compartilhar espaço durante a perseguição. O posicionamento local pode descartar fragmentos sem espaço ou acima do limite. Não há instalação relocável, salvamento ou rede nesta etapa. Não foram feitos commits, push ou deploy.
