# Validação da progressão procedural e do balanceamento

Registro de 21/11/2026, Linux/GCC 13.3.0, CMake 3.28.3 e SFML 2.6.1. Nenhum commit ou push foi feito nesta etapa.

## Verificações executadas

| Verificação | Evidência |
| --- | --- |
| Configuração, compilação e linkedição Debug | Aplicação, núcleo, simulador, bibliotecas e testes concluídos com avisos habilitados. |
| CTest Debug, `DISPLAY` removido | **15/15 passaram**. |
| CTest Release, `DISPLAY` removido | **15/15 passaram**. |
| ASan + UBSan em Debug, `DISPLAY` removido | **15/15 passaram**, sem diagnósticos dos sanitizadores. |
| Estatística Python | Oito casos de `AnalysisTests.py`, incluídos em `balancing_analysis_contracts`, passaram. |
| CLI do simulador | Rótulos, TTL, filtros inválidos e proteção de resultados contra aliases `..`, symlink e hardlink passaram. |
| Mapa padrão, procedural e CSV com `--game-seed 42 --smoke-test` | Cada modo desenhou três quadros e encerrou com código 0. |
| Semente variável no jogo normal | Duas inicializações sem `--game-seed` exibiram sementes distintas e encerraram normalmente. |
| HUD e arma após progressão real | Agente avançado levou o `Game` até round 6 e cinco aquisições; desenho SFML capturado e inspecionado em `build/procedural-window.png`. |
| Experimentos principais | **696 execuções**, 588 condições distintas; 108 repetições de calibração ao ampliar sementes. |
| Reprodução integral independente | Outras **696 execuções** em `build/balancing-reproduction`; todas as linhas de gameplay dos seis CSVs canônicos corresponderam exatamente ao arquivo entregue, excluindo somente `wall_seconds`. |

A captura visual é produzida por um driver temporário em `build/`: ele usa `SimulatedAgent`/`GameInput`, regras reais de combate, coleta e portal, depois o mesmo `DungeonVisualizer`. Não injeta vida, inimigos, upgrades ou rounds. A captura do framebuffer SFML evita depender da janela estar em primeiro plano. Não é comparação automatizada de pixels. A aplicação de produção mantém seu único Game Loop.

## Cobertura

- `procedural_weapon_contracts`: catálogo aleatório, sementes, ausência de aquisição no desbloqueio, primeiros doze tipos sem repetição, variantes relevantes em 600 rounds, modificadores, compensações, geometria circular/triangular/quadrada, espalhamento, paralelos, rajadas temporizadas, coleta, persistência e tiros contra asteroides em `RoundCompleted`.
- `simulation_contracts`: diferenças de habilidade, comandos retidos entre reações, replay determinístico usando `Game`, contadores reais, limites, horizonte por rounds concluídos e equivalência exata entre `GameConfig::recommended()` e o candidato experimental selecionado.
- `balancing_analysis_contracts`: média, desvio padrão, IC t, Wilson, pontuação, limite de projéteis, rejeição de duplicatas, horizontes diferentes e mistura indevida entre calibração e avaliação reservada.
- `simulation_cli_contracts`: exportação, rótulos e parâmetros válidos, rejeições e preservação de resultados já existentes.
- `mechanics_contracts`: motores, percepção, explosões, asteroides, fragmentos, gravidade, drops, coleta, ambiente, reinício, distribuição e limites. Expectativas específicas da arma antiga usam o modo histórico explícito.
- `gameplay_contracts`: mira, movimento, tiros, AABBs, primeiro impacto, paredes, spawns, derrota, reinício, mapas opcionais e portal. As fixtures de tiro a 12.000 px/s configuram explicitamente o teto de velocidade da arma nova.
- `map_contracts`: suíte preservada de CSV, coordenadas, A*, conectividade e 601 arenas procedurais.
- Oito casos CTest restantes: ajuda e validação dos argumentos da aplicação e do simulador, conforme listagem `ctest -N`.

A suíte de 15 casos é composta de cinco executáveis de contratos C++, duas suítes Python e oito casos de CLI. Os testes Python são registrados quando o interpretador é encontrado; C++ e o simulador não precisam de dependências Python em runtime.

## Reproduzir testes e experimentos

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
env -u DISPLAY ctest --test-dir build --output-on-failure

cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/release --parallel
env -u DISPLAY ctest --test-dir build/release --output-on-failure

cmake -S . -B build/sanitize -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
cmake --build build/sanitize --parallel
env -u DISPLAY ctest --test-dir build/sanitize --output-on-failure

env -u DISPLAY python3 tools/reproduce_balancing.py \
  --simulator build/release/space_arena_sim \
  --output-dir build/balancing-reproduction \
  --verify-against experiments/balancing
```

A reprodução executa novamente a seleção com as sementes de calibração antes de abrir a avaliação. Não inclui tempo computacional no teste de igualdade, pois esse custo depende da máquina e da carga do sistema. Os resultados originais e as cópias de verificação permanecem separados.

## Artefatos e inventário desta etapa

Criados: `WeaponUpgrade.hpp/.cpp`, `UpgradeManager.hpp/.cpp`, `ProjectilePattern.hpp/.cpp`, `GameStats.hpp`, `Simulation.hpp/.cpp`, `SimulationMain.cpp`, `ProceduralWeaponTests.cpp`, `SimulationTests.cpp`, `AnalysisTests.py`, `SimulationCLITests.py`, os scripts em `tools/`, `EXPERIMENT_PROTOCOL.md`, `BALANCING_REPORT.md` e os resultados/configurações em `experiments/balancing/`.

Ampliados: `Weapon`, `Game`, `GameConfig`, `Pickup`, metadado de origem em `Projectile`, `Main`, HUD no visualizador, CMake, `.gitignore`, testes existentes, README, arquitetura e plano de desenvolvimento. `Player`, `WaveManager`, aliens, asteroides, gravidade, portal, matemática, física e geração de mapas continuam reutilizados em seus próprios componentes.

O manifesto `experiment_source.sha256` preserva o estado dos fontes no início dos experimentos. O manifesto final registra os fontes entregues. Após o início dos experimentos houve adição da factory recomendada, HUD, metadados de CSV e validação da CLI; a reprodução integral verifica que esses ajustes não mudaram os resultados de gameplay arquivados.

## Limites observados

A recomendação é baseada em proxies e agentes artificiais. Iniciantes simulados continuam coletando poucos motores; a sobrevivência desse perfil não melhorou. A avaliação intermediária tem risco de derrota alto no horizonte de dez minutos. Não há prova de Flow humano nem mecanismo DDA online. Resultados, intervalos, alternativas e necessidade de avaliação com jogadores são detalhados no [relatório de balanceamento](BALANCING_REPORT.md).

Permanece a apresentação geométrica, colisão AABB, eventual sobreposição entre aliens e descarte de fragmentos sem espaço/capacidade. Mapas arbitrários muito pequenos podem não suportar combate. A fonte exige reconfigurar CMake se o build for movido. Outras plataformas ainda precisam de validação própria.

## Etapa posterior: menu, save e ranking (21/11/2026)

Foi acrescentado `persistence_contracts`, mantendo todos os casos anteriores. A suíte atual tem **16 testes CTest**, executados sem display em Debug, Release e ASan/UBSan. O novo caso verifica estados de tela, nova partida, pausa, restauração exata e continuidade por 300 passos, RNG/rajada/entidades, arquivos inválidos, autosave/manual, pontuação sem duplicação e top 10. Os números experimentais anteriores permanecem históricos e o balanceamento não foi modificado.

A validação gráfica abriu uma janela SFML e exercitou o mesmo tratador de eventos com mouse e teclado de teste: menu, Scores, Controles, páginas de Mecânicas, Novo jogo, pausa, salvar, retorno ao menu, Continuar fiel, confirmação de substituição e resize para 800 × 600. Capturas de framebuffer foram inspecionadas. O driver local está em `build/InterfaceGraphics.cpp`; imagens em `build/menu-*.png`. Não é uma comparação automatizada de pixels nem uma segunda implementação das regras.

Arquivos novos desta etapa: `Menu.hpp/.cpp`, `Button.hpp/.cpp`, `SaveManager.hpp/.cpp`, `ScoreManager.hpp/.cpp`, `LocalStorage.hpp/.cpp`, `tests/PersistenceTests.cpp` e `doc/PERSISTENCE.md`. Alterados: `Game` e `GameConfig` para score/nova partida; amigos `StateCodec` nos componentes com estado privado; visualizador e `Main` para menus/diretório de dados; CMake e documentação. Os recursos gráficos e fontes existentes foram reutilizados.

## Etapa posterior: cenários integrados ao Novo jogo (21/11/2026)

A suíte passou a **17 testes CTest**, todos aprovados em Debug, Release e ASan/UBSan, sem `DISPLAY`. O novo `scenario_integration` usa o `Game` real e somente entradas públicas `GameInput`. Para cada modo — arena livre, procedural com seed 77 e CSV do professor — um controlador de teste navega por A*, atira, elimina pelo menos cinco aliens e cruza dois portais até o round 3. Verifica drops, asteroides, gravidade, portal, corpos do jogador/inimigos em pisos livres e matriz preservada entre rounds.

Esse teste acelera spawns, força probabilidades ambientais/drops e usa arma mais forte para tornar a integração curta e determinística; não avalia dificuldade nem altera o balanceamento de produção. A matriz procedural é comparada diretamente à saída do `Dungeon2D` existente, e outra semente produz outro mapa. O CSV é comparado à carga direta de `TileMap`.

Cada modo é salvo em v2 e restaurado com tipo, semente, caminho e matriz idênticos. A partida personalizada é carregada depois de remover seu arquivo CSV de origem. Mais 120 passos após a carga produzem snapshots idênticos ao original. Payloads no esquema v1 também são restaurados. O menu cobre seleção procedural, confirmação/cancelamento de substituição, caminho vazio, arquivo ausente e piso inicial isolado, preservando a partida anterior nos erros.

A validação gráfica SFML exercitou seleção, início procedural, pausa/save, campo UTF-8 com espaços/acentos, erro de CSV sem perda do save, confirmação do mapa do professor e Continuar restaurando o tipo Custom. O driver temporário `build/ScenarioGraphics.cpp` terminou com código 0. Capturas inspecionadas: `build/scenario-menu.png`, `scenario-procedural.png`, `scenario-csv-menu.png` e `scenario-custom.png`. O teste usa eventos SFML sintéticos e não faz comparação automática de pixels.

Arquivos específicos novos: `source/Scenario.hpp/.cpp` e `tests/ScenarioTests.cpp`. Foram integrados `Menu`, `DungeonVisualizer`, `Main`, `Game`, `SaveManager`, `LocalStorage` e CMake. `Dungeon2D`, `TileMap`, A* e o `maze.csv` fornecido não foram alterados. O novo formato e a migração estão em [PERSISTENCE.md](PERSISTENCE.md).

A escrita atômica foi validada no Linux. Adaptador Windows, importação de versões futuras, múltiplas instâncias concorrentes e entrada de nomes Unicode completa continuam fora da validação atual. O formato e as regras estão em [PERSISTENCE.md](PERSISTENCE.md).
