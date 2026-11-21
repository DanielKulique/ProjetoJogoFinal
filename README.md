# Space Arena

Jogo 2D em **C++17 e SFML 2.6.1**, desenvolvido para a disciplina de Desenvolvimento de Jogos da UTFPR, professor André Koscianski.

A nave enfrenta aliens, asteroides e campos gravitacionais, coleta motores e melhorias procedurais de arma e avança por portais. A arena padrão continua **livre de paredes**, com 31 × 23 células de 64 pixels. CSV e geração procedural de paredes permanecem opcionais.

## Compilar e executar

Requisitos: CMake ≥ 3.16, compilador C++17 e **SFML 2.6.1 exata**, módulos graphics/window/system. Nenhuma dependência é baixada automaticamente.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
./build/space_arena
```

Para uma instalação própria da SFML, acrescente `-DCMAKE_PREFIX_PATH=/caminho/da/instalacao`. CMake copia a fonte DejaVu Sans, sua licença e `assets/maps/maze.csv` para o build. O executável encontra a fonte e o mapa do professor mesmo quando iniciado de outro diretório; reconfigure o CMake depois de mover a pasta de build.

## Menu e persistência

A janela inicia com Novo jogo, Continuar jogo, Scores, Controles, Mecânicas e Sair, seguindo a referência visual espacial. Use mouse, setas e Enter. **Novo jogo** abre a escolha do cenário:

- **Arena espacial clássica:** 31 × 23 células inicialmente livres.
- **Masmorra procedural:** `Dungeon2D::generate()` com semente aleatória, carregada pelo `TileMap`.
- **Masmorra personalizada:** botão para usar o `maze.csv` do professor ou campo para outro arquivo. Clique no campo ou pressione Tab, digite o caminho e pressione Enter; caminhos com espaços e acentos são aceitos.

O mapa é validado antes de substituir a partida. Se houver save válido, a confirmação aparece depois da escolha; cancelar ou fornecer um CSV inválido preserva a partida anterior. Continuar só é habilitado quando o arquivo é válido e restaura o tipo, a matriz e os RNGs sem depender do CSV original. Combate, spawns, drops, asteroides, gravidade, portal e progressão funcionam nos três modos, com as mesmas regras de jogo.

Esc abre a pausa com Retomar, Salvar, Controles, Mecânicas, Menu principal e Sair. Toda a simulação congela, inclusive cooldowns e RNG. Há salvamento automático ao concluir rounds e manual pela pausa; sair sem salvar conserva apenas o último save. Game Over invalida a partida salva e permite registrar um nome no ranking local dos dez melhores scores.

Arquivos ficam em `$XDG_DATA_HOME/space-arena` ou `~/.local/share/space-arena`. Para outro local, execute `./build/space_arena --data-dir PASTA`. Formato versionado, estados restaurados, pontuação e limitações estão em [PERSISTENCE.md](doc/PERSISTENCE.md).

## Controles e partida

| Controle | Ação |
| --- | --- |
| W / A / S / D | Mover; diagonais têm a mesma velocidade |
| Mouse | Apontar a nave usando a câmera atual |
| Botão esquerdo mantido | Disparar durante combate **e durante `RoundCompleted`** |
| R após derrota | Abrir a escolha de cenário para outra partida |
| Setas | Navegar a câmera por pisos |
| F / Home | Enquadrar arena / restaurar escala 1:1 |
| Esc | Pausar, retomar ou voltar |
| Fechar janela / Sair no menu | Encerrar |

A nave começa com quatro motores. Cada dano aceito remove um, com invulnerabilidade temporária de 1,25 s. As velocidades com 4/3/2/1 motores são 250/210/170/130 pixels/s. Um motor coletado recupera uma vida, até quatro; zero motores causa Game Over. O HUD mostra motores, round, aliens vivos e pendentes, semente e a configuração atual da arma: padrão, melhorias adquiridas, dano médio, quantidade, frequência, velocidade e duração dos projéteis.

Normais navegam autonomamente e detectam a nave a 220 pixels, perdendo o alvo além de 280. Avançados têm antena e perseguem desde o nascimento. Explodem ao tocar jogador ou asteroide, com dano radial único, sem atingir outros aliens. Morte por tiro não provoca explosão. Somente avançados eliminados pelo jogador são elegíveis para drops; normais nunca geram itens.

A quantidade-base é 5/7/9… até 25 aliens por round, com no máximo oito ativos; o round introdutório contém somente normais. Vida, mistura de tipos e intervalo de surgimento continuam configuráveis e os candidatos experimentais variam esses parâmetros.

Quando não há aliens vivos **nem pendentes**, aparece o portal violeta. Entre para avançar na mesma arena, preservando motores e arma. Se ele surgir sob a nave, saia e entre novamente. Asteroides e gravidade continuam ativos nessa espera, por isso o disparo permanece disponível. Perder o foco pausa o tempo simulado.

## Armas procedurais e coleta

Cada round, incluindo o primeiro, desbloqueia uma definição aleatória de melhoria. **Desbloquear disponibiliza a oferta; coletar um item aplica a melhoria.** Não há aquisição automática ao passar pelo portal. Uma sacola sem reposição evita ordem fixa e repetições até liberar os tipos básicos; depois surgem variantes com níveis adicionais. Não existe teto global de três aquisições.

| Família | Comportamento e compensação |
| --- | --- |
| Padrão | Tiro concentrado; variantes refinam dano com pequeno custo de cadência |
| Rajada | Projéteis consecutivos separados por tempo; continua após soltar o gatilho |
| Pesado | Maior dano, menor frequência e velocidade |
| Circular | Projéteis radiais em 360°, com menor dano individual |
| Triangular / quadrada | Três/quatro projéteis em vértices geométricos, avançando na direção da mira |
| Espalhado | Cone de cobertura; mais tiros reduzem o dano individual |
| Paralelo | Linhas paralelas com deslocamento lateral e dano dividido |
| Cadência / velocidade / dano / duração | Modificadores compatíveis, com retornos decrescentes e custos entre atributos |

A arma possui um padrão ativo combinado aos modificadores adquiridos; pegar outro padrão troca a geometria sem apagar os modificadores. Isso evita multiplicar todos os padrões entre si. Há limites de projéteis por emissão, tamanho de rajada, cadência, velocidade, duração e entidades simultâneas. Variantes saturadas não são oferecidas como melhorias úteis. As fórmulas e os limites estão na [arquitetura](doc/ARCHITECTURE.md).

O item de melhoria leva uma definição desbloqueada. Se a oferta ficar desatualizada antes da coleta, o jogo procura uma alternativa útil. Motores têm ícone distinto e nunca ultrapassam quatro. As posições são aleatórias, alcançáveis e verificadas contra mapa e corpos incompatíveis. Os parâmetros históricos de drop — 75% de item e, condicionalmente, 80% upgrade/20% motor — são o baseline experimental. O mecanismo de evitar motores desnecessários com a nave completa é configurável e fica separado do sorteio nominal.

O jogo normal aplica `GameConfig::recommended()`, selecionada somente com as sementes de calibração. Python 3 (biblioteca padrão) é necessário apenas para analisar ou reproduzir o pipeline de experimentos, não para jogar ou executar o simulador C++. A recomendação offline, seus parâmetros e a comparação com o balanceamento anterior estão no [relatório de balanceamento](doc/BALANCING_REPORT.md). Todos os valores continuam ajustáveis em `GameConfig`; os cenários do simulador registram a configuração efetivamente usada.

## Reprodução e mapas opcionais

```sh
./build/space_arena --game-seed 42
./build/space_arena --procedural --seed 42 --game-seed 7
./build/space_arena --csv assets/maps/maze.csv --game-seed 7
./build/space_arena --help
```

Sem `--game-seed`, a configuração inicial usa semente variável. As opções CLI configuram o mapa inicial e permitem smoke tests reproduzíveis; a janela continua abrindo no menu. A escolha explícita em Novo jogo sorteia novas sementes de partida e, no modo procedural, de geração. R após derrota abre essa escolha. Continuar restaura as sementes e o estado completo dos RNGs. Reproduzir uma partida também exige as mesmas entradas e configuração. `--seed` continua independente e controla somente o mapa procedural inicial. Ambas aceitam inteiros de 0 a 4294967295; CSV não aceita `--seed` ou `--procedural` juntos.

CSV usa `0` para piso e `1` para parede, separados por vírgulas, sem cabeçalho e com largura constante. O exemplo do professor de 40 × 30 continua preservado. O limite é 256 × 256 células; mapas sem piso ou com o piso inicial isolado são rejeitados. Caminhos relativos são resolvidos pelo diretório de execução. Corpos e projéteis respeitam paredes; A* é usado somente nos mapas com paredes. Spawns usam pisos alcançáveis; se o centro estiver bloqueado/desconectado, o portal usa o piso alcançável mais próximo. Mapas muito pequenos podem limitar entidades simultâneas por falta de espaço; o jogo aguarda posições livres sem atravessar paredes.

Asteroides conservam movimento, destruição, colisões e fragmentação limitada. Fragmentos têm proteção inicial e não se subdividem infinitamente; filhos sem espaço local ou capacidade são descartados. Campos gravitacionais afetam gradualmente corpos dinâmicos, com raio visível, duração e força configuráveis. A hipótese é 10 m/s² e 32 pixels/m antes de atenuação, amortecimento e limites de velocidade. Os valores ambientais também participam dos experimentos.

## Simulação sem janela

`space_arena_sim` controla a **lógica real de `Game`**, no mesmo passo de 1/60 s, através de `GameInput`. Não há outra implementação de combate ou física. Três agentes parametrizados representam iniciantes, intermediários e avançados por erros de mira, reação, eficiência de movimento, defesa e coleta. Observam informações visíveis e estimam movimento entre observações; não recebem RNG, futuros spawns ou estado privado dos inimigos.

```sh
# Uma configuração, três perfis e seis sementes de calibração
./build/space_arena_sim --preset procedural_baseline --profile all \
  --seed-start 1001 --runs 6 --seconds 600 --max-rounds 20 \
  --output build/balancing/results.csv --configs-output build/balancing/configs.csv

# Lista completa de opções e ajustes experimentais
./build/space_arena_sim --help
```

Presets: `baseline`, `procedural_baseline`, `repair`, `balanced`, `gentle`, `pressure`, `variety` e `environment`. `baseline` reproduz a arma histórica de três níveis; os demais usam progressão procedural e combinações explícitas de parâmetros. A comparação `procedural_baseline` versus `baseline` avalia o pacote progressão procedural e prevenção de reparos inúteis, não um efeito causal isolado de progressão: as probabilidades nominais são iguais, mas `avoidFullEngineDrops` difere. As opções `--drop`, `--engine-chance`, `--advanced-chance`, `--health-growth`, `--spawn-interval`, `--progression-strength`, `--asteroid-chance` e `--gravity-strength` permitem outras comparações sem editar as regras.

CSV registra rounds, tempos, censura, eliminações, precisão, dano, motores perdidos/recuperados, upgrades, padrões usados, métricas por padrão, projéteis simultâneos e custo computacional. O [protocolo experimental](doc/EXPERIMENT_PROTOCOL.md) fixa critérios e separação de sementes antes da seleção; o [relatório](doc/BALANCING_REPORT.md) apresenta comandos, dados e conclusões efetivamente observados. Maior sobrevivência não é tratada sozinha como melhor balanceamento. Indicadores de agentes são aproximações e **não comprovam o estado psicológico de Flow**; validação com jogadores humanos continua necessária.

## Testes e organização

```sh
ctest --test-dir build --output-on-failure
# Linux: confirmar independência de display
env -u DISPLAY ctest --test-dir build --output-on-failure

# Display/OpenGL necessários
./build/space_arena --smoke-test
./build/space_arena --procedural --seed 42 --smoke-test
./build/space_arena --csv assets/maps/maze.csv --smoke-test
```

As suítes cobrem os contratos anteriores, novos padrões, rajadas, desbloqueios, compatibilidade, persistência, sementes, limites, agentes e reprodução dos experimentos. Resultados executados em Debug, Release e sanitizadores ficam em [VALIDATION.md](doc/VALIDATION.md).

`DungeonVisualizer` mantém apresentação e o único Game Loop interativo. `Game` centraliza regras; `Player` compõe `Weapon`; definições, desbloqueios, modificadores, geometria e simulação têm responsabilidades separadas. Veja [ARCHITECTURE.md](doc/ARCHITECTURE.md) e [DEVELOPMENT_PLAN.md](doc/DEVELOPMENT_PLAN.md).

Cada arquivo `.cpp` de `source/` possui uma explicação em `.txt` na pasta `doc/`, com responsabilidade, métodos principais e integração. Consulte o [índice das implementações](doc/IMPLEMENTACOES.txt).

O ajuste atual é offline, sem DDA durante a partida. A apresentação usa formas simples, sem áudio; colisões usam AABBs, aliens podem se sobrepor e não há instalação relocável ou rede nesta etapa. Esta tarefa não realiza commit ou push automaticamente.
