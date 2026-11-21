# Implementação final das mecânicas

O ciclo anterior de movimento/mira/tiros/rounds/portal foi ampliado sem substituir os componentes funcionais de matemática, física, mapas e colisão contínua. O objetivo desta etapa é integrar as mecânicas especificadas e tornar seus valores ajustáveis.

| Prioridade | Implementação | Evidência automatizada |
| --- | --- | --- |
| Integridade e estados | Arquitetura existente, único loop, `Playing`/`RoundCompleted`/`GameOver`. | Suítes de mapas e regressões do ciclo anterior. |
| Aliens e IA | Normal com `Wandering`/`Chasing` e percepção; avançado perseguindo desde nascimento. | Mesma semente fora da percepção produz mesma trajetória apesar de alvos distintos; entrada/perda do alvo e perseguição avançada. |
| Combate e explosões | Avançado detona em contato com jogador/rocha; dano radial único, sem dano entre aliens. | Explosão por ambos os contatos, proteção de dano, nenhum drop por contato e nenhuma explosão por tiro. |
| Asteroides | Spawn periódico probabilístico, trajetória, colisões, vida útil e dispersão. | Movimento, três/quatro fragmentos, uso único, tamanho, profundidade, proteção inicial e limite ativo. |
| Drops e upgrades | Dois sorteios independentes somente por morte de avançado por tiro; posição válida; tiro até nível 3. | Reprodução por semente, amostra de 20 mil decisões, coleta, expiração e nenhuma duplicação. |
| Motores | Quatro vidas com velocidades 250/210/170/130, reparação e invulnerabilidade. | Cada velocidade, perda limitada, recuperação até quatro e derrota com zero. |
| Gravidade | Região limitada, temporizador de minuto, aceleração amortecida em corpos dinâmicos. | Centro sem singularidade, limite de alcance, conversão de unidades, deriva limitada, expiração e ausência de resistência fora do campo. |
| Rounds e portal | 5/7/9… até 25, oito ativos, cadência decrescente, avançados a partir do segundo. | Progressão, distribuição de quatro mil spawns, pendentes, abertura, transição e preservação de motores/upgrades. |
| Integração e balanceamento | `GameConfig`, semente de partida, limites e reinício reproduzível. | Contagem após mortes ambientais, limites, RNG restaurado e regressões em mapas opcionais. |

Os resultados efetivamente executados estão em [VALIDATION.md](VALIDATION.md). As regras, fórmulas e aplicações acadêmicas estão em [ARCHITECTURE.md](ARCHITECTURE.md).

## Decisões preservadas

- Arena inicial sem paredes; CSV e procedural opcionais, sem regeneração entre rounds.
- Coordenadas físicas em pixels e integração existente em passo de 1/60 s.
- `Game` centraliza colisões, autoria da morte, drops e remoções; o visualizador apresenta o estado.
- O normal não usa a posição distante do jogador para escolher destinos de navegação.
- A* somente em mapas com paredes, conservando os vetores diretos na arena livre.
- Corpos inativos não recebem efeitos; fragmentos entram em lote depois das colisões.
- Um único campo gravitacional e limites configuráveis para entidades evitam crescimento descontrolado.
- Probabilidades e sementes pertencem à simulação, sem sorteio por FPS de desenho.

## Limitações e próximas etapas

A apresentação é geométrica e não inclui áudio. A nave usa AABB mesmo quando gira, aliens podem compartilhar espaço durante perseguição e a fragmentação é uma aproximação física. O número de filhos inseridos pode ser menor que três/quatro quando não houver espaço local ou capacidade; não são reposicionados em outra região. Mapas mínimos podem não suportar a partida completa. O caminho da fonte exige reconfigurar CMake após mover o build.

Texturas finais, áudio, outras armas/inimigos, instalação relocável, salvamento e redes ficam para etapas futuras. Esses recursos não fazem parte dos critérios das mecânicas integradas atuais. Não houve commit, push ou deploy automático.
