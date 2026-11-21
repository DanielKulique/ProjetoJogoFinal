# Arquitetura e mecânicas integradas

## Responsabilidades e propriedade

A organização do professor André Koscianski permanece: `Dungeon2D`, `TileMap`, `DungeonVisualizer`, `Game`, `Entity`, `NPC`, matemática e física. Os nomes históricos `NCP.cpp` e `BeheviorTree.cpp` foram preservados. Componentes funcionais de geração, CSV, vetores, transformações, integração e A* continuam reutilizados.

| Componente | Responsabilidade |
| --- | --- |
| `Main` | Selecionar mapa e sementes e compor jogo/visualizador. |
| `DungeonVisualizer` | Possuir janela, câmera e fonte; eventos, input, único Game Loop e apresentação. |
| `Game` | Regras, relógio, spawns, danos, atribuição de mortes, remoções, estados e portal. |
| `GameConfig` | Agregar os parâmetros de balanceamento e validar configurações. |
| `Player : Entity` | Motores, direção, mira, invulnerabilidade, deriva e arma por composição. |
| `Weapon` / `Projectile : Entity` | Nível, cadência, espalhamento; movimento, dano, vida e posição anterior. |
| `Enemy : NPC` | Tipo normal/avançado, FSM, vida, perseguição, deriva e cooldown de contato. |
| `Asteroid : Entity` | Movimento, resistência, expiração e fragmentação com profundidade limitada. |
| `GravityField : Entity` | Região e fórmula de aceleração, sem integrar ou teleportar objetos. |
| `Pickup : Entity` | Tipo do item, caixa de coleta e duração. |
| `WaveManager` | Quantidade prevista, pendentes, cadência, limites e progressão. |
| `Portal : Entity` | Ativação e contato com exigência de saída/reentrada quando necessário. |
| `TileMap` | Matriz validada, CSV, pontos/áreas livres e conversões mundo/célula. |

`Game` possui jogador, portal e onda por valor; inimigos são `vector<unique_ptr<Enemy>>`, por serem NPCs não copiáveis. Projéteis, asteroides, itens e eventos visuais usam vetores por valor; existe no máximo um `optional<GravityField>`. O visualizador observa o estado a cada desenho, sem manter ponteiros persistentes para os inimigos removidos. O registro acadêmico de NPCs externos continua disponível com responsabilidade de desregistro do chamador.

`NPC` continua com textura de alocação tardia por `unique_ptr`, criada apenas ao carregar sprite. Isso permite construir aliens lógicos sem display/OpenGL. O núcleo vincula SFML pela herança de NPC, mas a simulação e seus testes não criam janela. `Game` não lê teclado/mouse nem desenha.

## Loop, coordenadas e entrada

`DungeonVisualizer::run()` é o único loop. Eventos são processados por quadro e `Game::update(1/60, GameInput)` avança a simulação por acumulador. O tempo acumulado por quadro é limitado a 0,25 s; sem foco, o acumulador é descartado e o tempo do jogo pausa. Em `GameOver` regras e temporizadores ficam congelados. O callback de extensão permanece posterior à atualização normal, sem integrar novamente a partida.

`GameInput` contém direção WASD, alvo em pixels de mundo e gatilho contínuo. Mouse usa `mapPixelToCoords(pixel, view)`, incluindo câmera, viewport, deslocamento e redimensionamento. O HUD é desenhado em coordenadas da janela, fora da viewport reservada ao mundo. R após derrota chama `restart()` sem repetição automática.

Todas as posições físicas são centros em **pixels**, x à direita e y para baixo. Velocidade usa pixels/s e aceleração pixels/s². Células usam `(coluna, linha)`, com `TILE_SIZE=64`. `cellCenter()` converte para `((col+0.5)*64,(row+0.5)*64)`; a conversão inversa usa `floor` após validar o ponto.

Mira usa `atan2(dy,dx)` em graus, com direita 0°, baixo 90°, cima −90°. Cursor sobre a nave preserva o ângulo; a renderização soma 90° porque o triângulo aponta originalmente para cima. Mira não altera posição.

## Motores, armas e controle

`PlayerConfig::engineSpeeds[0..4] = {0,130,170,210,250}` define velocidades por motores. Direção não nula é normalizada. Dano positivo remove motores sem passar de zero e inicia invulnerabilidade de 1,25 s. `restoreEngine()` recupera um até quatro; não ressuscita nave inativa. `health()`/`maxHealth()` são aliases de compatibilidade para motores/4, sem segunda barra de vida independente.

`WeaponConfig` mantém dano 25, velocidade 700 pixels/s e vida 2,5 s dos tiros. Nível 1 usa intervalo 0,18 s; nível 2 usa 0,12 s; nível 3 usa a cadência melhorada e direções central/±12°. Nível máximo padrão 3 impede crescimento ilimitado. `fireVolley()` compartilha um cooldown entre todos os tiros da rajada; `tryFire()` continua disponível para consumidores do tiro simples. Intervalos são quantizados pelo passo fixo.

O comando de movimento e a deriva gravitacional são separados. Para jogador e aliens:

```text
deriva = limitarComprimento((deriva + aceleracao * dt) * exp(-amortecimento * dt), derivaMaxima)
velocidade = comandoNormalizado * velocidadeConfigurada + deriva
```

A aceleração externa é limpa antes de chamar a integração existente, evitando aplicá-la duas vezes. Assim, WASD não apaga a força acumulada e a gravidade não elimina o controle. Os padrões de amortecimento são 2 s⁻¹ e deriva máxima 100 pixels/s.

## IA dos aliens

A decisão usa uma FSM separada de colisões, física e renderização. A árvore de comportamento existente fica preservada para futuras decisões mais complexas; não foi necessário substituir seu contrato.

| Tipo/estado | Decisão |
| --- | --- |
| Normal / `Wandering` | Escolher destino aleatório por RNG local; renovar a cada 2 s ou ao chegar. A posição distante do jogador não participa do destino. |
| Normal / `Chasing` | Entrar quando distância ≤220 pixels; perseguir com vetor até perder alvo além de 280 pixels. |
| Avançado / `Chasing` | Perseguir desde o nascimento, sem limiar de detecção. |

A histerese 220/280 evita alternância a cada quadro perto da fronteira. A navegação aleatória usa raio 300 e 65% da velocidade de perseguição. Normais têm meia extensão 18 pixels; avançados 24. O avançado é desenhado com antena e aparência distinta. Seu multiplicador de vida é 1,5 e de velocidade 1,1.

Na arena livre não se chama A*. Mapas opcionais com paredes reutilizam `findPathAStar(TileMap::rows(),...)`. Células do caminho viram centros em pixels e o corpo percorre os pontos, sem ultrapassá-los no comando de um passo. O caminho é replanejado quando o destino muda, com cooldown de 0,4 s. `Game` ainda verifica o corpo completo contra paredes e limites.

Cada inimigo possui sua semente local, fornecida pelo RNG da partida. Duas simulações com mesma configuração, semente e entradas reproduzem as decisões. Fora da percepção, mudar a posição do jogador não muda a trajetória de um normal com a mesma semente.

## Colisões, explosões e autoria das mortes

`Game` centraliza as interações. `AABB::intersects()` conserva o contrato de incluir contato de bordas. `TileMap::isAreaFree()` verifica a caixa completa, permitindo encostar em parede sem penetrar; usa `nextafter` no extremo máximo. Após integrar movimento, `constrainMovement()` reconstrói o deslocamento em trechos de até oito pixels e testa cada eixo, permitindo deslizar por paredes opcionais.

Projéteis guardam posição anterior. `Collision2D::firstIntersection()` expande a caixa do alvo pelas meias extensões do projétil e calcula o primeiro contato no segmento. A menor fração entre parede/contorno, alien e asteroide determina um único impacto. O projétil é consumido e não atinge vários corpos. Tocar parede no mesmo instante tem prioridade. O último trecho de vida do tiro ainda é considerado; tiros expirados são removidos depois.

Aliens normais causam dano por contato com intervalo de 0,8 s; invulnerabilidade do jogador também protege contra vários contatos simultâneos. Asteroides podem atingir jogador ou aliens; impactos destrutivos eliminam a rocha e podem gerar fragmentos. Inativos não recebem novos efeitos.

Um avançado ativo detona ao tocar jogador ou asteroide. A detonação o torna inativo antes de aplicar efeitos, garantindo evento único. Um círculo de raio configurável 110 pixels é testado contra a AABB do jogador e de asteroides. O dano ao jogador respeita invulnerabilidade; rochas atingidas são destruídas. Aliens não recebem dano dessa explosão, evitando reações em cadeia. O evento visual tem duração padrão 0,35 s e não é uma fonte contínua de dano.

Morte por tiro é identificada no impacto que muda o avançado de ativo para inativo. Só essa transição chama `tryDrop()`. Morte por explosão, contato ou ambiente não faz sorteio. Normais nunca fazem sorteio. Não se adiciona explosão à morte por tiro.

Remoções ocorrem após os percursos que usam referências; fragmentos são acumulados separadamente e inseridos ao final. Isso preserva iteradores e ponteiros temporários usados na resolução.

## Drops e itens

A sequência é:

1. Sorteio independente com chance 0,75 de haver item.
2. Somente em caso positivo, novo sorteio com chance 0,80 de upgrade; caso contrário, motor.
3. Escolha de posição válida aleatória entre pisos alcançáveis, com caixa inteira livre e sem sobrepor jogador, portal ativo, aliens, rochas ou outros itens.

As probabilidades incondicionais são **60% upgrade, 15% motor e 25% nenhum item**. Não se reutiliza o primeiro valor aleatório para escolher o tipo. Mesmo sem espaço/capacidade, a morte é processada uma única vez; não há repetição do sorteio em passos posteriores.

Itens têm meia extensão 12 pixels, duração 20 s e limite de 12 simultâneos. Coleta aplica restauração/upgrade uma vez e marca o item como inativo. Um item motor no limite não ultrapassa quatro; upgrades não ultrapassam o nível máximo. Itens sobreviventes continuam entre rounds até coleta ou expiração.

## Asteroides e fragmentação

`AsteroidConfig` reúne raio 28, velocidade 65 pixels/s, vida 50, dano de um motor, máximo 20 corpos, vida útil 45 s e tentativa de spawn a cada 5 s com chance 0,10. Surgimentos validam todo o corpo e ocupação. Movimento começa retilíneo; a física existente integra aceleração, há amortecimento linear e teto de velocidade 240 pixels/s. `Game` rebate a velocidade no eixo bloqueado por contorno/parede.

Destruição com vida zero pode produzir quatro fragmentos, configuráveis para três. Expiração não fragmenta. Para direção radial unitária `u`:

```text
raioFilho = raioPai * 0.45
vidaFilho = max(1, vidaMaximaPai / 2)
posicaoFilho = centroPai + u * (raioPai + raioFilho)
velocidadeFilho = 0.5 * velocidadePai + u * (velocidadeConfigurada * 1.25)
```

A velocidade é limitada pelo teto. Direções são distribuídas uniformemente ao redor da rocha; é uma aproximação de dispersão, sem pretensão de conservação exata de energia/momento. Filhos têm profundidade incrementada e proteção de contato por 0,3 s. Com profundidade máxima padrão 1, não se fragmentam novamente. `fragment()` tem trava de uso único.

O limite de corpos inclui fragmentos. Antes de inserir, o jogo busca espaço local válido sem sobrepor outros corpos ou fragmentos do mesmo lote. Próximo a paredes ou em região cheia, filhos sem espaço são descartados; eles não reaparecem longe da explosão. Proteção e inserção posterior impedem múltiplos contatos instantâneos pelo evento original.

## Gravidade e tempo discreto

`GravityFieldConfig` define tentativa a cada 60 s de tempo simulado, chance 0,50, máximo um campo ativo, duração 20 s e raio 260 pixels. Temporizadores continuam em `Playing` e `RoundCompleted`, pausam sem foco e congelam em `GameOver`. Não há sorteio por frame; são consumidos intervalos completos. Um campo ativo não é substituído por uma tentativa seguinte.

A anotação original “10 m/s” foi interpretada como **aceleração de 10 m/s²**. Com conversão configurável de 32 pixels/m, `a0 = 320 pixels/s²`. Para distância `d`, raio `R` e suavização central `s=16`:

```text
a(p) = normalizar(centro-p) * a0 * (1-d/R) * min(1,d/s),  se 0<d<R
       0,                                                caso contrario
```

O amortecimento e a deriva máxima de jogador e aliens são ajustados em `PlayerConfig` e `EnemyConfig`. Asteroides usam `AsteroidConfig::linearDamping` e `maxSpeed`. Para projéteis dentro do campo, `GravityFieldConfig::damping` aplica resistência `-damping * velocidade`, e `maxDrift` define o acréscimo máximo à velocidade nominal da arma. Fora do raio, o campo não freia os tiros.

A força aponta ao centro, zera na borda e aproxima-se de zero no centro, evitando singularidades. `apply()` apenas escreve aceleração em `RigidBody2D`; não altera posição. Jogador e aliens acumulam deriva amortecida; asteroides e projéteis integram a aceleração e possuem resistência/limites adequados. O círculo visual indica alcance, e o centro tem marcador distinto. Não é um obstáculo sólido.

A integração original é Euler semi-implícito:

```text
v = v + a * dt
p = p + v * dt
```

O passo fixo de 1/60 s mantém temporizadores e controles discretos reproduzíveis. O amortecimento usa exponencial em função de `dt`, em vez de um fator dependente do FPS de desenho.

## Rounds, portal e configuração

Para round `r`, com os padrões:

```text
quantidade = min(25, 5 + 2*(r-1))
intervalo = max(0.5, 2 - 0.1*(r-1)) segundos
vidaNormal = 50 + 10*(r-1)
vidaAvancado = floor(vidaNormal * 1.5)
velocidade = min(220, (90 + 5*(r-1)) * multiplicadorDoTipo)
```

Há no máximo oito aliens ativos simultaneamente; o restante fica pendente. No round 1 todos são normais; a partir do 2 cada surgimento escolhe 75/25 por sorteio. A onda só completa com vivos e pendentes zerados. Mortes ambientais também removem aliens dessa contagem; asteroides e campos não participam.

O portal preserva o comportamento existente: centro geométrico quando livre/conectado; senão, piso alcançável mais próximo. Ativação sob a nave exige sair e voltar. Entrar avança a onda sem regenerar mapa, reparar motores ou zerar upgrades. `restart()` é diferente: repõe motores/arma iniciais, limpa entidades/efeitos, volta ao round 1 e reinicia relógios e RNG.

`GameConfig` agrega `PlayerConfig`, `WeaponConfig`, `WaveConfig`/`EnemyConfig`, `AsteroidConfig`, `GravityFieldConfig`, `PickupConfig` e `DropConfig`. Probabilidades, intervalos, atributos, raios e limites ficam nessas estruturas. `--game-seed` fornece a semente de 32 bits; `--seed` continua exclusivo do mapa procedural. O RNG da partida é `mt19937`, com decisões em helpers locais, sem aleatoriedade global.

## Conceitos da disciplina

| Área | Aplicação |
| --- | --- |
| Matemática | Vetores, normalização, `atan2`, conversões entre células/pixels, teste círculo/caixa e varredura de segmento. |
| Física | Corpo rígido 2D, integração semi-implícita, aceleração radial, amortecimento, colisão e dispersão simplificada. |
| Controle discreto | Passo de 1/60 s, acumuladores, cooldowns, histerese perceptiva e limites de atuação. |
| Inteligência artificial | FSM `Wandering`/`Chasing`, percepção local, aleatoriedade controlada e A* somente com paredes. |
| Engenharia de Software | Separação de responsabilidades, configuração central, testes unitários/integrados, RNG reproduzível e documentação. |
| Orientação a objetos em C++ | `Entity`/`NPC` como bases, composição de arma, RAII, `unique_ptr`, valores e referências observadoras. |

## Limites

AABBs não giram com a nave; aliens podem se sobrepor durante perseguição. A física de fragmentos é aproximada e pode descartar filhos sem espaço. Mapas mínimos podem ser válidos para visualização e insuficientes para combate. O núcleo mantém dependência de SFML por NPC, embora funcione sem display. A fonte é localizada por caminho definido pelo CMake e exige reconfiguração ao mover o build. Áudio, sprites finais, instalação relocável, rede e salvamento ficam fora desta etapa.
