# Arquitetura e mecânicas integradas

## Responsabilidades e propriedade

A organização do professor André Koscianski permanece: `Dungeon2D`, `TileMap`, `DungeonVisualizer`, `Game`, `Entity`, `NPC`, matemática e física. Os nomes históricos `NCP.cpp` e `BeheviorTree.cpp` foram preservados. Componentes funcionais de geração, CSV, vetores, transformações, integração e A* continuam reutilizados.

| Componente | Responsabilidade |
| --- | --- |
| `Main` | Selecionar mapa e sementes e compor jogo/visualizador. |
| `Scenario` / `ScenarioInfo` | Criar os três cenários reutilizando os carregadores existentes; identificar tipo, semente de geração e origem CSV. |
| `DungeonVisualizer` | Possuir janela, câmera e fonte; eventos, input, único Game Loop e apresentação. |
| `Game` | Regras, relógio, spawns, danos, atribuição de mortes, remoções, estados e portal. |
| `GameConfig` | Agregar os parâmetros de balanceamento e validar configurações. |
| `Player : Entity` | Motores, direção, mira, invulnerabilidade, deriva e arma por composição. |
| `Weapon` / `Projectile : Entity` | Configuração adquirida, modificadores, cadência e fila de rajada; corpo, dano, vida e origem do padrão. |
| `WeaponUpgrade` / `UpgradeManager` | Definições parametrizadas e catálogo desbloqueado por sorteio, sem aquisição automática. |
| `ProjectilePattern` | Geometria pura de posições/direções por vetores e `Transform2D`. |
| `GameStats` | Contadores de eventos reais, somente observação, sem alterar regras. |
| `Simulation` / `SimulatedAgent` | Execução sem janela do `Game` e controle por `GameInput`, com perfis parametrizados. |
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

`GameInput` contém direção WASD, alvo em pixels de mundo e gatilho contínuo. Mouse usa `mapPixelToCoords(pixel, view)`, incluindo câmera, viewport, deslocamento e redimensionamento. O HUD é desenhado em coordenadas da janela, fora da viewport reservada ao mundo. R após derrota passa por `Menu::restartAfterDefeat()` e abre a escolha de cenário, sem repetição automática.

Todas as posições físicas são centros em **pixels**, x à direita e y para baixo. Velocidade usa pixels/s e aceleração pixels/s². Células usam `(coluna, linha)`, com `TILE_SIZE=64`. `cellCenter()` converte para `((col+0.5)*64,(row+0.5)*64)`; a conversão inversa usa `floor` após validar o ponto.

Mira usa `atan2(dy,dx)` em graus, com direita 0°, baixo 90°, cima −90°. Cursor sobre a nave preserva o ângulo; a renderização soma 90° porque o triângulo aponta originalmente para cima. Mira não altera posição.

## Motores, armas e controle

`PlayerConfig::engineSpeeds[0..4] = {0,130,170,210,250}` define velocidades por motores. Direção não nula é normalizada. Dano positivo remove motores sem passar de zero e inicia invulnerabilidade de 1,25 s. `restoreEngine()` recupera um até quatro; não ressuscita nave inativa. `health()`/`maxHealth()` são aliases de compatibilidade para motores/4, sem segunda barra de vida independente.

### Definição, desbloqueio e aquisição

`WeaponUpgrade` contém `UpgradeKind`, nível da variante e intensidade. Os doze tipos são padrão, rajada, pesado, circular, triangular, quadrado, espalhado, paralelo, cadência, velocidade, dano e duração. O padrão inicial é simples, mas sua melhoria de precisão também possui variantes relevantes.

`UpgradeManager` recebe explicitamente o `mt19937` da partida. Em cada round, incluindo o primeiro, uma definição nova entra no catálogo. Uma sacola evita repetir tipos antes de percorrer os básicos; sacolas seguintes geram níveis maiores, com escolha aleatória e sem repetir imediatamente um tipo quando há alternativa. Chamar o desbloqueio duas vezes no mesmo round não acrescenta outro item. Se os únicos tipos restantes já estiverem saturados, uma variante útil de outro tipo é escolhida. Não existe teto global de três aquisições.

Desbloqueio não altera `Weapon`. O drop carrega uma definição elegível do catálogo; a coleta chama `applyUpgrade()`. Uma definição cujo nível já foi adquirido não volta a aplicar o mesmo efeito. Se um item ficar obsoleto antes de ser coletado, a regra de coleta procura uma alternativa relevante. O portal preserva arma, níveis por tipo e catálogo. Reinício restaura a arma inicial e libera o catálogo do round 1 usando a semente escolhida.

A arma mantém um padrão ativo e modificadores passivos independentes. Adquirir outro padrão substitui a geometria ativa, sem apagar passivos. Isso permite combinações sem produto cartesiano de padrões, que multiplicaria a quantidade de tiros. `WeaponConfig::legacyProgression` existe somente para reproduzir o baseline histórico: nível 1 simples, nível 2 com maior cadência e nível 3 espalhado. Esse teto não se aplica à progressão procedural.

### Modificadores e compensações

Para cada tipo `k`, defina `p(k) = sqrt(nivelAdquirido(k)) * intensidadeDaVariante(k) * progressionStrength`. O cálculo reavalia a configuração a partir dos maiores níveis adquiridos; não multiplica um fator exponencial a cada coleta.

```text
dano = danoBase * (1 + 0.18*p(Dano)) / (1 + 0.025*p(Cadencia)) / (1 + 0.015*p(Velocidade))
intervalo = intervaloBase * (1 + 0.035*p(Dano)) / (1 + 0.18*p(Cadencia))
velocidade = velocidadeBase * (1 + 0.16*p(Velocidade)) / (1 + 0.01*p(Duracao))
duracao = duracaoBase * (1 + 0.12*p(Duracao))
dano *= 1 + 0.045*p(PadraoAtivo)
intervalo *= 1 + 0.015*p(PadraoAtivo)
```

Dano maior custa frequência; cadência e velocidade custam potência individual; duração custa velocidade. A intensidade permanece ajustável em `GameConfig::weapon`. O dano nominal é fracionário e `Projectile::damage()` continua inteiro: uma sobra determinística é acumulada entre emissões para realizar o dano médio sem sortear nem tornar níveis pequenos ineficazes por arredondamento.

| Padrão | Geometria e compensação inicial |
| --- | --- |
| Padrão | Um projétil alinhado à mira. |
| Pesado | Dano ×2,4; intervalo ×2,8; velocidade ×0,8. |
| Rajada | Primeiro tiro imediato e outros separados por temporizador; 3 a 5 tiros, dano individual ×0,85 e intervalo proporcional ao tamanho. |
| Circular | 8 a 16 direções radiais em 360°; dano ×0,55 e intervalo ×2,4. |
| Triangular | Três vértices equidistantes, direções paralelas à mira; dano ×0,55 e intervalo ×1,3. |
| Quadrado | Quatro vértices equidistantes, direções paralelas à mira; dano ×0,45 e intervalo ×1,4. |
| Espalhado | Cone de direções; dano individual ×(1,6/quantidade), intervalo ×1,2. |
| Paralelo | Deslocamentos laterais com a mesma direção; dano individual ×(1,6/quantidade), intervalo ×1,2. |

As receitas variam quantidade, abertura, raio da formação e atributos pelos níveis. Os limites iniciais são 16 projéteis por emissão, 5 tiros por rajada, intervalo mínimo 0,06 s, velocidade máxima 1600 pixels/s e duração máxima 8 s. O número global de projéteis ainda é limitado por `GameConfig::maxProjectiles`. Esses valores são configuráveis. Não há teto prático de três níveis nem crescimento exponencial por empilhamento; modificadores saturados que só acrescentariam custo não são escolhidos como ofertas úteis.

### Geometria e rajadas no tempo discreto

`projectilePatternRays()` recebe centro, direção e parâmetros, retornando posições/direções sem criar entidades. `Transform2D::rotation(angulo) * Transform2D::translation(x,y)` respeita o contrato de vetores-linha existente: aplica rotação e depois translação. Vértices usam `cos(2*pi*i/n)`/`sin(2*pi*i/n)`; o paralelo usa o vetor lateral `(-dy,dx)` e o espalhado gira a direção por ângulos em torno da mira. As direções são normalizadas.

`Weapon::update(dt)` é chamado uma vez por `Player::update()`. `fireVolley()` inicia a emissão quando há gatilho e cooldown disponível. `pollProjectiles()` é chamado em todo passo para drenar projéteis consecutivos já programados, inclusive após soltar o botão. Rajadas não são uma única emissão de vários tiros simultâneos. O estado programado conserva seus atributos e usa a posição/direção atual da nave em cada emissão. A próxima rajada aguarda o cooldown compartilhado e o término da anterior.

`Game` valida as posições geométricas contra o mapa e aplica o limite global antes de inserir os projéteis. Cada tiro recebe o identificador do padrão que o originou, inclusive os tiros atrasados de rajada. Durante `RoundCompleted`, gatilho e fila de rajada continuam funcionando para enfrentar os asteroides sobreviventes; a limpeza dos tiros do combate na conclusão não impede novas emissões nesse estado.

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

Somente a morte de um avançado por tiro abre a decisão de drop. O baseline nominal utiliza sorteio com probabilidade 0,75 de item e, se positivo, um segundo sorteio independente com 0,80 de upgrade/0,20 de motor. Isso corresponde a 60% upgrade, 15% motor e 25% nada antes das regras de utilidade e posicionamento. Os experimentos variam essas probabilidades explicitamente, sem alterar a elegibilidade dos tipos de alien.

Uma posição aleatória é escolhida entre pisos alcançáveis, com a caixa inteira livre e sem sobrepor jogador, portal ativo, aliens, rochas ou outros itens. A falta de capacidade/espaço não repete a morte ou o sorteio em passos futuros. Itens têm meia extensão 12 pixels, duração 20 s e limite inicial 12 simultâneos.

O upgrade guarda a `WeaponUpgrade` escolhida entre definições desbloqueadas e ainda aplicáveis. A aquisição só ocorre na coleta. `avoidFullEngineDrops` evita produzir reparações inúteis com quatro motores, redirecionando para uma oferta útil quando possível. Sem oferta útil e sem motor perdido, o jogo não concede uma melhoria fictícia. No baseline histórico essa política pode ser desligada para conservar suas probabilidades efetivas. O sorteio nominal e o item efetivamente criado são conceitos distintos na leitura dos experimentos.

Coleta aplica restauração ou melhoria uma vez e marca o item como inativo. Motor recupera uma vida sem ultrapassar quatro e não ressuscita a nave. Itens sobreviventes persistem entre rounds até coleta ou expiração; melhorias já adquiridas persistem indefinidamente na partida.

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

Para round `r`, os parâmetros-base históricos de `GameConfig{}` produzem:

```text
quantidade = min(25, 5 + 2*(r-1))
intervalo = max(0.5, 2 - 0.1*(r-1)) segundos
vidaNormal = 50 + 10*(r-1)
vidaAvancado = floor(vidaNormal * 1.5)
velocidade = min(220, (90 + 5*(r-1)) * multiplicadorDoTipo)
```

Há no máximo oito aliens ativos simultaneamente; o restante fica pendente. No round 1 todos são normais; a partir do 2 cada surgimento escolhe 75/25 por sorteio. A onda só completa com vivos e pendentes zerados. Mortes ambientais também removem aliens dessa contagem; asteroides e campos não participam.

O portal preserva o comportamento existente: centro geométrico quando livre/conectado; senão, piso alcançável mais próximo. Ativação sob a nave exige sair e voltar. Entrar avança a onda sem regenerar mapa, reparar motores ou zerar upgrades. `restart()` é diferente: repõe motores/arma iniciais, limpa entidades/efeitos, volta ao round 1 e reinicia relógios e catálogo. O reinício de baixo nível pode preservar uma semente fixa; o comando de interface Novo jogo chama `newGame()` e sorteia outra semente.

`GameConfig` agrega `PlayerConfig`, `WeaponConfig`, `WaveConfig`/`EnemyConfig`, `AsteroidConfig`, `GravityFieldConfig`, `PickupConfig` e `DropConfig`. Probabilidades, intervalos, atributos, raios e limites ficam nessas estruturas. Sem `--game-seed`, `Main` escolhe uma semente variável, mostra-a no terminal/HUD e configura reinício com nova semente. Com `--game-seed`, define-se a semente inicial do núcleo e dos testes; a interface Novo jogo inicia outra jornada, enquanto Continuar restaura o RNG salvo; `--seed` continua exclusivo do mapa procedural. O RNG da partida é `mt19937`, com decisões em helpers locais, sem aleatoriedade global.

## Telemetria, agentes e balanceamento offline

`GameStats` conta os eventos reais de emissão, impacto, morte, dano, restauração e coleta. Não existe cálculo alternativo de combate para produzir os dados. A origem de cada projétil permite agregar disparos/acertos e dano a aliens por padrão. Dano a asteroides possui contador separado. Eliminações por ambiente contam no progresso da onda, mas somente eliminações por tiro pertencem ao contador do jogador. Precisão é a razão de impactos em alien/asteroide por projéteis emitidos; uma formação circular pode legitimamente trocar precisão por cobertura.

`Simulation::runSimulation()` instancia a arena padrão e o `Game` real, cria um `SimulatedAgent` e chama `game.update(1/60, agent.input(...))`. O loop é um executor de experimentos, sem outra implementação de regras e sem janela; o único loop interativo permanece no visualizador. Não concede itens, cura, elimina adversários ou acelera cooldowns. Resultados no limite de tempo/rounds são marcados como censurados, em vez de tratados como derrota ou sobrevivência ilimitada.

`AgentConfig` parametriza reação, erro de mira/movimento, eficiência, evasão, coleta, distância preferida, antecipação estimada e gatilho. Os perfis iniciante/intermediário/avançado diferem nesses valores, não nas regras do jogo. O agente observa posições/tipos visíveis, HUD e geometria; retém decisões entre observações e estima velocidade por observações anteriores. Não consulta vida privada dos aliens, seu estado de decisão, RNG, futuros spawns ou telemetria para ganhar vantagem.

O CSV de configuração registra tanto parâmetros do jogo quanto do controlador. Cada partida registra semente, perfil, horizonte, rounds/tempos, precisão, mortes/dano, motores, melhorias, padrões efetivamente disparados, métricas por padrão, média/pico de projéteis e tempo computacional. O custo de CPU depende do ambiente e não participa da reprodução determinística dos eventos. Dados brutos e sínteses devem distinguir duração observada limitada pelo horizonte de expectativa completa de vida.

As configurações experimentais incluem o modo histórico de três níveis e combinações procedurais com alterações de drops, mistura de aliens, vida, spawn, intensidade da arma e ambiente. `procedural_baseline` conserva as probabilidades nominais do baseline, mas muda a progressão e ativa a prevenção de reparos inúteis; portanto a comparação é do pacote, sem atribuir todo efeito somente à arma. As comparações usam múltiplas sementes e os três perfis; calibração e avaliação reservada são separadas. O [protocolo](EXPERIMENT_PROTOCOL.md) define critérios antes da seleção e o [relatório](BALANCING_REPORT.md) documenta resultados, parâmetros recomendados e limitações. A recomendação permanece em configuração, sem atalhos nos controles de simulação.

Este trabalho implementa **balanceamento offline**, não DDA em tempo real. A teoria do fluxo motiva estudar equilíbrio entre capacidade e desafio, mas os proxies de agentes não comprovam concentração, prazer ou Flow psicológico. Validar esses aspectos exige jogadores humanos. Adaptação dinâmica só deve ser considerada após avaliar os parâmetros fixos; nenhum ajuste oculto de dificuldade muda durante uma partida nesta versão.

## Conceitos da disciplina

| Área | Aplicação |
| --- | --- |
| Matemática | Vetores, normalização, `atan2`, conversões entre células/pixels, teste círculo/caixa e varredura de segmento. |
| Física | Corpo rígido 2D, integração semi-implícita, aceleração radial, amortecimento, colisão e dispersão simplificada. |
| Controle discreto | Passo de 1/60 s, acumuladores, cooldowns, rajadas temporizadas, histerese perceptiva e limites de atuação. |
| Inteligência artificial | FSM `Wandering`/`Chasing`, percepção local, aleatoriedade controlada e A* somente com paredes. |
| Engenharia de Software | Definições separadas de aquisição e geometria, configuração central, testes unitários/integrados, experimentos reproduzíveis e documentação. |
| Orientação a objetos em C++ | `Entity`/`NPC` como bases, composição de arma, RAII, `unique_ptr`, valores e referências observadoras. |

## Limites

AABBs não giram com a nave; aliens podem se sobrepor durante perseguição. A física de fragmentos é aproximada e pode descartar filhos sem espaço. Mapas mínimos podem ser válidos para visualização e insuficientes para combate. O núcleo mantém dependência de SFML por NPC, embora funcione sem display. A fonte e o CSV do professor são localizados por caminhos definidos pelo CMake e exigem reconfiguração ao mover o build. Áudio, sprites finais, instalação relocável e rede ficam fora desta etapa.


## Interface e persistência local

`Menu` controla os estados de aplicação e chama regras somente em `Screen::Playing`. `Button` é reutilizado na apresentação. `DungeonVisualizer` mantém a única janela e loop, com viewport própria para menus, mouse convertido pelas coordenadas dessa viewport e descarte do acumulador ao trocar de tela. `SaveManager`/`StateCodec` gravam o estado privado explicitamente por uma amizade restrita ao codec; texturas e recursos gráficos não entram no arquivo. `ScoreManager` mantém o top 10. `LocalStorage` fornece envelope versionado, checksum e promoção atômica de arquivos. `Game` calcula pontos apenas nos eventos reais de eliminação por tiro e na conclusão de round.

O esquema completo, invariantes, autoria de pontos e limites de plataforma estão em [PERSISTENCE.md](PERSISTENCE.md). Nenhum atributo de dano, probabilidade, progressão ou física foi ajustado nesta etapa. Saves guardam a configuração da própria partida, e a enciclopédia lê seus parâmetros em tempo de apresentação.

## Seleção integrada de cenário

`Scenario::create()` produz a arena livre 31 × 23, chama o gerador `Dungeon2D::generate(seed)` com as mesmas dimensões ou delega o CSV a `TileMap::loadCSV()`. Não existe outro gerador nem outra interpretação do CSV. CMake copia o `maze.csv` original para o build e fornece seu caminho ao núcleo. O menu permite usá-lo diretamente ou editar um caminho UTF-8. São rejeitados mapas maiores que o limite do save, sem piso ou com piso inicial isolado.

`Menu` mantém uma candidata por `unique_ptr<Game>` enquanto aguarda confirmação. Apenas após validar o mapa e invalidar o save antigo move a candidata para o `Game` já observado pelo visualizador. Isso preserva a referência existente e evita descartar progresso por erro de CSV. Ao entrar na partida, a câmera é reenquadrada nas dimensões do mapa; retomar uma pausa conserva a câmera.

Todos os modos usam as mesmas regras de `Game`, incluindo busca de pisos alcançáveis, seleção de spawns, colisões, navegação A* em mapas com paredes e eventos ambientais. O `DungeonVisualizer` continua responsável pela apresentação e único loop. `ScenarioInfo` e a matriz entram no save v2; a restauração usa a matriz armazenada sem depender do gerador ou do arquivo CSV externo.
