# Etapa de progressão procedural e balanceamento offline

| Entrega | Implementação e evidência |
| --- | --- |
| Catálogo procedural | Doze tipos de melhoria, sacola com RNG explícito, desbloqueio por round sem aquisição automática e variantes relevantes depois dos tipos básicos. |
| Armas e modificadores | Oito padrões ativos, quatro especializações passivas, compensações entre atributos, geometria via `Transform2D`, rajadas temporizadas e limites de recursos. |
| Coleta | Só avançados mortos por tiro são elegíveis; itens válidos preservam ofertas desbloqueadas, evitam aquisições duplicadas e reparações sem efeito no modo novo. |
| Continuidade | Motores, velocidade por vida, aliens/FSM, explosões, asteroides, gravidade e portal preservados; é possível atirar em `RoundCompleted`. |
| Simulação | Três perfis com percepção discreta e `GameInput`, usando exclusivamente as regras reais de `Game`, sem janela. |
| Telemetria | Rounds, tempos, dano, motores, upgrades, precisão, padrões, projéteis e custo de execução observados em eventos reais. |
| Experimentos | Exploração, ampliação, refinamentos limitados, seleção congelada e avaliação reservada; 696 execuções principais, 588 condições distintas. |
| Recomendação | `growth18`, escolhida na calibração; `GameConfig::recommended()` é aplicada por `Main` e corresponde exatamente aos parâmetros avaliados. |
| Reprodução | Pipeline completo reexecutado: outras 696 partidas com métricas de gameplay idênticas, excluindo custo computacional. |
| Qualidade | 15/15 casos CTest em Debug, Release e ASan/UBSan; execução gráfica dos modos existentes e HUD inspecionado. |

O [protocolo](EXPERIMENT_PROTOCOL.md) define metas e partições; o [relatório](BALANCING_REPORT.md) apresenta números e compensações; a [validação](VALIDATION.md) registra comandos, inventário e limitações. Não houve commit ou push automático.

## Próximos trabalhos fundamentados pelos resultados

Validar com jogadores humanos a carga de desafio, a aprendizagem dos padrões e a coleta de motores, especialmente para iniciantes. O ganho de diversidade e desafio para agentes fortes não implica melhor experiência para todos. Só após essa avaliação deve ser considerado DDA: gradual, com limites, janelas e histerese, sem eliminar derrotas. A presente entrega faz ajuste offline e não modifica dificuldade em resposta ao desempenho durante a partida.

Áudio, sprites finais, instalação relocável, rede e salvamento continuam fora deste escopo. Não são usados para encobrir limites encontrados no balanceamento.

## Menu, interface e persistência

Implementados menu espacial com mouse/teclado, telas de scores/controles/enciclopédia, pausa completa, confirmação de substituição, novo jogo, continuar, autosave por round e save manual. Persistência versionada guarda corpos, configurações, arma, catálogo, entidades, relógios e todos os RNGs. A pontuação respeita autoria do jogador; o ranking local conserva dez resultados e evita registro duplicado da mesma derrota. O balanceamento da etapa experimental permanece inalterado.

## Três cenários no Novo jogo

Concluída a escolha entre arena livre, masmorra procedural com semente aleatória e CSV personalizado com atalho para `maze.csv`. `Scenario` reutiliza `Dungeon2D` e `TileMap`; A*, colisões e o loop do `DungeonVisualizer` foram preservados. Saves v2 conservam tipo, matriz e metadados, com leitura de v1. A integração exercita combate, ambiente, drops e dois portais em cada modo, além de restauração independente do CSV e substituição segura pelo menu. Nenhum parâmetro de balanceamento de produção foi alterado.

Esquema, durabilidade, limites e testes estão em [PERSISTENCE.md](PERSISTENCE.md). A apresentação segue a referência fornecida, usando a fonte e as formas existentes.
