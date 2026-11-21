# Menu, salvamento e pontuação

A aplicação inicia no menu principal inspirado na referência espacial: estrelas determinísticas, planeta com órbita, nave ciano, título e botões centralizados. A interface usa a fonte existente, viewport proporcional e tamanho mínimo de janela de 640 × 600. A navegação aceita mouse, setas/W/S e Enter/Espaço. Mecânicas tem três páginas, acessíveis por botões ou setas laterais.

## Estados e responsabilidades

`Menu` controla `MainMenu`, `ScenarioSelection`, `CustomScenario`, `Playing`, `Paused`, `Scores`, `Controls`, `Mechanics`, `GameOver` e confirmação de nova partida. Esses estados são diferentes de `GameState::Playing/RoundCompleted/GameOver`. `DungeonVisualizer` conserva a janela e o único loop; só chama `Menu::update()` durante a tela de partida, que por sua vez chama `Game::update()`.

Esc pausa/retoma a partida ou volta das telas auxiliares. A perda de foco também pausa. Enquanto os menus estão abertos, não se atualiza nenhuma regra, física, RNG, cooldown ou relógio. O acumulador e o delta são descartados quando há transição de telas, impedindo que tempo gasto na pausa ou na leitura de arquivo seja aplicado ao retomar.

Novo jogo oferece arena livre, masmorra procedural e CSV personalizado. `Scenario::create()` reutiliza `Dungeon2D::generate()` e os carregadores de `TileMap`. A partida candidata é construída e validada antes de pedir confirmação da substituição de um save válido. Cancelar ou fornecer um CSV inválido conserva o jogo e os bytes anteriores; confirmar invalida o save e substitui o jogo por essa candidata, com motores, arma, round, score, entidades e relógios reiniciados. Falha ao invalidar o arquivo impede a substituição. R na derrota abre a mesma escolha de cenário. `--game-seed` continua disponível para configuração inicial e testes; Novo jogo sorteia outra semente e mostra a semente efetiva no HUD. Para continuar fielmente uma sequência já iniciada, use o save, que inclui todos os RNGs.

## Destinos dos arquivos

- `$XDG_DATA_HOME/space-arena/`, quando definido.
- Caso contrário, `$HOME/.local/share/space-arena/`.
- Se nenhum existir no ambiente, pasta temporária `space-arena`.
- `--data-dir PASTA` substitui o destino, especialmente útil em testes isolados.

`session.sav` contém uma partida continuável. `scores.dat` guarda até dez resultados. As saídas não são gravadas no repositório por padrão. Um save ausente/inválido desabilita Continuar; falhas de carregamento/salvamento são exibidas na interface. Ranking inválido não é silenciosamente substituído por uma lista vazia ao registrar outro resultado.

## Formato de partida versão 2

Ambos os arquivos usam um cabeçalho ASCII:

```text
SPACE_ARENA_SAVE 2 <bytes_payload> <checksum_decimal>\n
<payload>
```

O ranking continua usando `SPACE_ARENA_SCORES` e **versão 1**, com o mesmo envelope. O checksum é FNV-1a de 64 bits sobre os bytes do payload, destinado a detectar corrupção acidental; não é assinatura ou proteção contra adulteração intencional. O limite total é 8 MiB. Versão desconhecida, assinatura errada, tamanho divergente, truncamento, bytes extras, checksum incorreto ou estado inválido rejeitam a carga sem substituir a partida atual.

A versão 2 acrescenta `ScenarioInfo` no início do payload: tipo (`Classic=0`, `Procedural=1`, `Custom=2`), semente de geração e caminho CSV informativo. A matriz completa continua sendo gravada e é a fonte usada na restauração: carregar não gera outra masmorra nem abre o CSV, mesmo que ele tenha sido movido ou removido. O mapa e seu tipo permanecem entre rounds. Saves de partida versão 1 continuam legíveis; como não continham o tipo, são classificados como Classic quando não têm paredes ou Custom quando têm. Sua matriz e regras são preservadas, mas a origem procedural histórica não pode ser recuperada. O próximo salvamento escreve versão 2.

O payload de partida é texto numérico em locale clássico. Valores float/double usam precisão suficiente para roundtrip exato; strings usam aspas e escapes de `std::quoted`; coleções incluem contagem e opcionais incluem presença. `StateCodec`, em `SaveManager.cpp`, é o esquema normativo e enumera os campos explicitamente. Não se gravam ponteiros, padding de structs, recursos SFML, texturas ou bytes brutos da memória.

São incluídos configuração completa, matriz do mapa, score, métricas, catálogo desbloqueado e sua sacola, upgrades adquiridos/tipos/tiers, estado fracionário de dano, cooldown e rajada pendente, motores e deriva do jogador, corpos/colisões, orientação, aliens com FSM/percepção/caminhos/cooldowns/RNG local, cronograma de onda, asteroides/fragmentos e proteção inicial, itens e sua oferta, gravidade/duração, portal/saída obrigatória, efeitos visuais, relógios, tipo de próximo alien já sorteado e RNG geral `mt19937`.

Listas de pisos livres, candidatos a spawn e indicação de paredes são derivadas novamente do mapa usando a mesma implementação; o cursor de seleção é restaurado. A câmera não faz parte das regras da partida e volta ao enquadramento disponível do visualizador. O formato valida mapa até 256 × 256, limite de coleção de 100 mil elementos, números finitos, vetores até dez milhões de unidades, índices de RNG, tipos e limites dos estados, configurações e contagens de entidades. Mudanças futuras no esquema exigem nova versão ou migração explícita; não há promessa de compatibilidade com versões futuras.

## Escrita e ciclo de vida

O arquivo é criado por `mkstemp` com nome exclusivo na mesma pasta do destino, gravado integralmente, sincronizado com `fsync`, fechado e promovido por `rename` atômico. A pasta também é sincronizada quando disponível. Se a escrita falhar antes da promoção, o save anterior permanece e o temporário é removido. Temporários abandonados não são tratados como save válido. A implementação de durabilidade usa APIs POSIX e foi validada no Linux; Windows precisa de um adaptador de escrita equivalente.

Ao concluir um round, a aplicação salva automaticamente o estado com portal ativo. O menu de pausa permite salvar manualmente. Não há salvamento implícito ao fechar a janela ou escolher Sair; use Salvar antes de sair se quiser preservar progresso posterior ao último round. Game Over invalida `session.sav` antes de apresentar a tela de resultados. Problemas de permissão são informados; o diretório precisa permitir escrita/remoção para persistir ou invalidar arquivos. Uso concorrente por várias instâncias não possui bloqueio de sessão.

## Pontuação

`GameConfig::scoring` define:

| Evento | Pontos iniciais |
| --- | ---: |
| Alien normal eliminado por projétil do jogador | 100 |
| Alien avançado eliminado por projétil do jogador | 250 |
| Asteroide grande destruído por projétil do jogador | 50 |
| Fragmento destruído por projétil do jogador | 10 |
| Round concluído | 500 |

O evento que torna a entidade inativa atribui pontos uma vez. Alvos inativos não recebem novos impactos. Explosões e colisões ambientais não rendem pontos de combate, mesmo quando eliminam um alien ou fragmentam uma rocha; o bônus de conclusão do round continua válido. O score não altera dano, drops, spawns ou balanceamento.

Game Over mostra score, round, tempo e aliens eliminados. Clique no campo de nome ou pressione Tab para editá-lo e pressione Enter para registrar (Backspace e até 32 caracteres ASCII pela entrada atual). Durante a edição, letras e espaços não acionam atalhos de menu; Esc encerra a edição. Registrar é uma ação explícita; após sucesso o botão fica desabilitado, impedindo registro duplicado da mesma derrota. O ranking conserva os dez maiores scores em ordem decrescente; empates mantêm a ordem de registro, e o mesmo nome pode ter partidas distintas.

## Testes executados

`persistence_contracts` verifica save/load exato e mais 300 passos idênticos depois da carga, incluindo RNG, estado de alien, gravidade, itens, asteroides e rajada. Também cobre arquivos ausentes/corrompidos, payload inválido com checksum válido, confirmação/cancelamento, continuar, pausa sem avanço, controles/mecânicas, salvamento manual e automático, score/portal restaurados, invalidação por derrota, reinício e top 10. Pontuação normal, avançada, rocha, fragmento e morte ambiental têm asserções próprias.

As suítes anteriores continuam ativas. Os testes gráficos usam o renderizador SFML e o tratador de eventos real com eventos SFML de teste para mouse/teclado, navegação, redimensionamento, pausa, save e retomada. Capturas locais ficam em `build/menu-main.png`, `menu-controls.png`, `menu-mechanics.png`, `menu-paused.png`, `menu-confirm.png`, `menu-scores.png` e `menu-resized.png`; não são testes de comparação de pixels.

`scenario_integration` complementa a suíte com combate real até o round 3 em cada modo, observação de drops/asteroides/gravidade/portal, colisões, igualdade da matriz entre rounds, restauração dos metadados e mais 120 passos idênticos após a carga. O CSV original é removido antes de restaurar a partida personalizada. Também cobre migração de payloads v1, seleção pelo menu, caminho vazio/inválido e cancelamento seguro.
