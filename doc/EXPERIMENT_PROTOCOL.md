# Protocolo de balanceamento offline

Protocolo fixado antes de selecionar a configuração final. Ele orienta uma busca exploratória, seguida de calibração ampliada e avaliação reservada. Os números são critérios de engenharia para agentes simulados, não medidas validadas do estado psicológico de Flow.

## Pergunta e limites

A hipótese de engenharia é que progressão procedural com compensações e uma taxa de reparação ajustada reduz a discrepância entre resistência dos aliens e capacidade de armamento sem tornar todos os perfis invencíveis. Simulações podem medir progressão, erros, recursos e diversidade; não podem demonstrar prazer, concentração ou Flow humano. Depois desta análise offline, um teste com jogadores é necessário. DDA durante a partida não será acrescentado apenas por parecer desejável: primeiro será examinada a variação entre perfis sob parâmetros fixos.

A física e as regras são exclusivamente `Game::update(1/60, GameInput)`. O simulador não remove adversários, concede itens, cura a nave, altera relógios nem consulta o estado privado para facilitar uma partida. Os agentes usam observações do mundo visível; cada perfil tem percepção atualizada em intervalos, erro de mira e capacidade de movimento diferentes. Conhecer a posição renderizável de um alien é permitido; conhecer sua próxima escolha aleatória ou os próximos spawns não é.

## Desenho

- Mapa: arena livre padrão de 31 × 23 células; nenhuma alteração procedural de paredes entre execuções.
- Perfis: iniciante, intermediário e avançado; parâmetros e semente de controle registrados pelo executável.
- Unidade experimental: uma partida completa de configuração × perfil × semente.
- Limite inicial: 600 segundos de tempo simulado, ou 20 rounds. Sobreviventes no limite são observações censuradas, não vitórias nem tempo de vida conhecido.
- Exploração: sementes **1001–1006** em todos os perfis e configurações candidatas.
- Calibração ampliada: sementes **1001–1020**, nos três candidatos mais promissores; inclui as seis exploratórias, portanto não é validação independente.
- Avaliação final: sementes **9001–9020**, somente após congelar a recomendação e seu comparador baseline. Esses dados não são usados para escolher ou retocar a recomendação.
- Os mesmos identificadores de semente/perfil são usados em cada configuração para comparações pareadas. Isso não garante a mesma sequência de eventos, pois mudanças de regras/consumo de RNG podem divergir; reduz apenas parte da variabilidade.
- O baseline preserva o modelo anterior de arma com três níveis e as probabilidades 0,75 de item e 0,80 condicional de upgrade. Um comparador procedural preserva os mesmos parâmetros numéricos anteriores, mas inclui a progressão e a política de evitar reparações inúteis com motores cheios. Assim, ele compara o pacote procedural/anti-desperdício com o baseline; não identifica causalmente o efeito isolado da arma ou dessa política. Todas as configurações compartilham a correção que permite disparar durante `RoundCompleted`; portanto o baseline reproduz o balanceamento anterior, não o defeito de controle.

A busca inicial deve variar conjuntamente, em conjuntos explicitamente registrados: chance total de drop, mistura motor/upgrade, chance de alien avançado, incremento de vida por round, intervalo de spawn, força da progressão da arma, frequência de asteroides e intensidade gravitacional. A recomendação permanece editável em `GameConfig`.

## Critérios de seleção definidos antecipadamente

São preferências de projeto para esta arena e horizonte. Não derivam diretamente de um instrumento psicológico de Flow.

1. **Progresso com desafio (40%)**: rounds máximos médios próximos dos alvos iniciais 3/6/10 para iniciante/intermediário/avançado. Há penalidade tanto abaixo quanto acima dos alvos; sobrevivência ilimitada não maximiza este componente. Para alvo `t` e média `r`, qualidade `max(0, 1 - abs(r-t)/t)`.
2. **Risco de derrota (20%)**: proporção de derrotas próxima de 0,65/0,45/0,25 por perfil. Qualidade `max(0, 1 - abs(p-alvo))`. Observações censuradas são tratadas como não derrotadas até o horizonte; não se estima mortalidade posterior.
3. **Discriminação de habilidade (15%)**: diferença entre média de rounds de avançados e iniciantes próxima de 7; qualidade `max(0, 1-abs(diferenca-7)/7)`. Inversão de habilidade é penalizada adicionalmente por seleção interpretativa, nunca escondida no relatório.
4. **Recuperação (10%)**: quando houve motores perdidos, fração recuperada/perdida próxima de 0,40; qualidade `max(0, 1-abs(fracao-0.40)/0.60)`. Ausência de dano não é evidência de reparação adequada e recebe zero neste componente.
5. **Diversidade efetiva (10%)**: média de padrões distintos realmente disparados por partida próxima de pelo menos 3; qualidade `min(1, diversidade/3)`. Desbloqueios que nunca foram adquiridos/disparados não contam como diversidade de arma usada.
6. **Ritmo (5%)**: duração média de rounds concluídos próxima de 50 segundos, com qualidade `max(0, 1-abs(media-50)/100)`. Partidas sem completar round recebem zero, sem inventar duração para o round inconcluso.

O score agrega primeiro cada componente entre perfis com pesos iguais. A maior pontuação da calibração ampliada sugere a configuração recomendada; os valores brutos, efeitos por perfil e compensações devem continuar visíveis. Caso os resultados sejam incompatíveis com as preferências (por exemplo, todos os perfis triviais ou nenhuma configuração produz progressão), a conclusão deve declarar o problema e justificar uma nova rodada de exploração antes de tocar as sementes reservadas.

São barreiras de integridade: dados finitos, nenhum limite de projéteis excedido, zero motores causando derrota, recuperação máxima quatro, persistência correta pelo portal e nenhum acesso privilegiado dos agentes. Os testes verificam invariantes. Uma configuração que viole uma barreira é desclassificada, independentemente do score. A recomendação não será baseada apenas no tempo de sobrevivência.

## Medidas e análise

Registrar por partida: semente, perfil/configuração, rounds alcançados/concluídos, tempo de sobrevivência observado, censura/derrota, tempos de rounds concluídos, eliminações, disparos/acertos, dano causado/recebido, motores perdidos/recuperados, upgrades coletados, diversidade de padrões, métricas por padrão, pico de projéteis e tempo computacional real. Custos de CPU não são determinísticos e ficam fora das comparações de reprodução bit a bit.

Para cada configuração/perfil, reportar número de partidas, média, desvio padrão e intervalo de confiança de 95% da média com distribuição t (quando `n>1`), e intervalos de Wilson para taxas de derrota e chegada ao round 2. Os intervalos são descritivos, não correções por múltiplas comparações nem prova de generalização. Para resultados limitados pelo horizonte, não chamar a média observada de expectativa de vida completa.

Comparações de configurações usam diferenças pareadas por perfil/semente onde existirem ambas as partidas. Para precisão, reportar também razão total de acertos/disparos; padrões geométricos amplos podem legitimamente ter menor precisão e maior cobertura. Dano por projétil, eliminações, ritmo e sobrevivência devem ser lidos conjuntamente. Sementes e perfis são amostras artificiais; falta variabilidade humana.

## Artefatos e reprodução

O executável `space_arena_sim` gera CSV com uma linha por partida. O script `python3 tools/analyze_balancing.py --input resultados.csv --output-dir saida` produz resumos em CSV/JSON e diferenças pareadas. Ele rejeita misturar sementes de calibração e avaliação; as sementes 9001–9020 são marcadas como avaliação somente, sem elegibilidade para seleção da recomendação. Os comandos efetivamente executados, parâmetros selecionados e resultados observados serão registrados em `doc/BALANCING_REPORT.md`; este protocolo não contém resultados antecipados.

## Referências conceituais verificadas

- **John L. Sherry (2004), “Flow and Media Enjoyment”.** O artigo relaciona o aproveitamento de conteúdo de mídia ao equilíbrio entre desafios do conteúdo e capacidades cognitivas individuais, usando videogames como exemplo. Isso motiva observar diferentes perfis; não transforma qualquer métrica de sobrevivência em medida direta de Flow. [Resumo do periódico](https://academic.oup.com/ct/article-abstract/14/4/328/4110457?login=false), [DOI](https://doi.org/10.1111/j.1468-2885.2004.tb00318.x).
- **Penelope Sweetser e Peta Wyeth (2005), “GameFlow: a model for evaluating player enjoyment in games”.** O modelo reúne concentração, desafio, habilidades, controle, metas claras, feedback, imersão e interação social. Sua investigação inicial utiliza avaliações de especialistas em dois jogos de estratégia. Nosso simulador cobre apenas parte de desafio/habilidade/controle e deixa os demais aspectos para validação humana. [Publicação ACM](https://doi.org/10.1145/1077246.1077253).
- **Maurice Hendrix, Tyrone Bellamy-Wood, Sam McKay, Victoria Bloom e Ian Dunwell, “Implementing Adaptive Game Difficulty Balancing in Serious Games”.** O trabalho trata a adaptação como escolha de variáveis mecânicas baseada em um modelo de desempenho e propõe um processo de seis etapas, discutindo dois estudos de caso. Isso fundamenta separar observação, parâmetros controláveis e avaliação; não obriga acrescentar DDA antes de conhecer o balanceamento fixo. [Preprint institucional dos autores](https://pure.coventry.ac.uk/ws/portalfiles/portal/13374335/pre_print.pdf), [DOI](https://doi.org/10.1109/TG.2018.2791019).

As fontes primárias foram consultadas nesta implementação. São usadas como orientação conceitual, não como validação dos pesos/limiares de engenharia escolhidos acima.

## Refinamento após a exploração, antes das sementes reservadas

A exploração de 144 partidas mostrou boa diversidade, mas a calibração ainda precisava aproximar o desafio dos alvos: `pressure` teve médias 4,5/13,33/14,67 rounds, e iniciantes recuperaram poucos motores em todas as configurações. Por isso foram definidos quatro candidatos adicionais, mantendo agentes, regras e score congelados. Todos derivam de `pressure` (força de progressão 1,2; asteroides 0,15; gravidade 12 m/s²):

| Rótulo | Drop | Upgrade condicional | Avançado | Vida por round | Intervalo de spawn | Vida do item |
| --- | --- | --- | --- | --- | --- | --- |
| growth14 | 0,90 | 0,50 | 0,30 | 14 | 1,6 s | 35 s |
| growth18 | 0,90 | 0,40 | 0,30 | 18 | 1,6 s | 35 s |
| paced | 0,90 | 0,40 | 0,30 | 14 | 1,3 s | 35 s |
| recovery_long | 0,95 | 0,40 | 0,35 | 10 | 1,6 s | 45 s |

O refinamento usa somente 1001–1006, seguido pelos dois candidatos adicionais de maior score em 1001–1020. A escolha final compara esses resultados com a calibração ampliada dos candidatos iniciais. Não se mudam metas para favorecer o vencedor, não se altera o controlador após observar resultados e não se abrem as sementes 9001–9020 até gravar a seleção final. A vida maior dos itens amplia a janela de coleta sem mover itens ao jogador nem conceder reparos automaticamente.
