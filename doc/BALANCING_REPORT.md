# Progressão procedural e investigação de balanceamento

Registro de 21/11/2026. Todos os resultados numéricos abaixo vêm dos CSVs/JSONs em `experiments/balancing/`. Não são estimativas inventadas nem medições com jogadores humanos. Não houve commit ou push nesta etapa.

## Resultado principal

O jogo agora possui definições de upgrades, desbloqueio aleatório por round, aquisição por itens, composição de modificadores e oito padrões de disparo, incluindo rajadas temporizadas e formações geométricas. Não existe teto global de três aquisições no modo procedural. Há limites físicos de cadência, velocidade, vida e quantidade de projéteis, compensações entre atributos e variantes com retornos decrescentes. O modo antigo de três níveis continua disponível apenas como comparador experimental.

A busca selecionou **`growth18`** em dados de calibração, com score de engenharia **61,396/100**, antes de abrir as sementes reservadas. O jogo interativo usa os mesmos parâmetros em `GameConfig::recommended()`. A avaliação reservada teve score descritivo 65,504/100; esse segundo score não foi usado para escolher nem retocar a recomendação.

A suspeita inicial de resistência excessiva frente à arma antiga não se manifestou nos agentes fortes durante este horizonte: o baseline teve alta progressão sem derrotas desses perfis. Isso não invalida dificuldades humanas ou em partidas mais longas, mas impede tratar a suspeita como conclusão empírica destas simulações.

O resultado é um compromisso: os agentes intermediários e avançados encontraram mais desafio e usaram mais padrões, mas **os iniciantes não tiveram melhora de sobrevivência nem de progressão**. A recuperação absoluta dos iniciantes aumentou pouco e com incerteza. Não afirmamos que a nova arma é universalmente mais potente, que a dificuldade é ideal ou que Flow psicológico foi demonstrado.

## Fundamento e decisão sobre DDA

A relação entre desafio e habilidade orienta a hipótese, sem fornecer automaticamente os limiares numéricos do experimento. [Sherry, “Flow and Media Enjoyment”](https://academic.oup.com/ct/article-abstract/14/4/328/4110457?login=false), DOI 10.1111/j.1468-2885.2004.tb00318.x, discute o equilíbrio entre capacidades individuais e desafios de conteúdo. [Sweetser e Wyeth, “GameFlow”](https://doi.org/10.1145/1077246.1077253), DOI 10.1145/1077246.1077253, trata também de concentração, controle, metas, feedback, imersão e interação social: uma curva de sobrevivência não mede todos esses elementos.

[Hendrix et al., “Implementing Adaptive Game Difficulty Balancing in Serious Games”](https://pure.coventry.ac.uk/ws/portalfiles/portal/13374335/pre_print.pdf), DOI 10.1109/TG.2018.2791019, relaciona variáveis mecânicas ajustáveis a um modelo observado de desempenho. Aqui efetuamos **balanceamento offline**, com parâmetros constantes dentro da partida. **Não foi implementado DDA online.** A grande diferença entre perfis sugere investigar adaptação posteriormente, mas o modelo de agentes ainda não justifica intervir em jogadores humanos. Seriam necessários observação humana, medidas de experiência e calibração de janelas, limites e histerese antes de mudar dificuldade em tempo real.

## Método, congelamento e partições

O protocolo e os pesos foram definidos em [EXPERIMENT_PROTOCOL.md](EXPERIMENT_PROTOCOL.md) antes da seleção. Cada execução usa a arena livre de 31 × 23 células e a lógica real de `Game::update(1/60, GameInput)`. O simulador não tem uma segunda implementação de combate e não concede cura, eliminações ou itens privilegiados aos controladores.

Cada perfil observa apenas informações renderizáveis, atualizadas no momento da reação: posições, tipos, movimento estimável e configuração atual da arma. Não consulta próximos sorteios, destinos privados, spawns pendentes para decidir ações ou futuras melhorias. A velocidade é estimada de observações anteriores, não lida de uma previsão. Os três perfis são aproximações artificiais, não populações de habilidade humana.

| Parâmetro dos controladores | Iniciante | Intermediário | Avançado |
| --- | ---: | ---: | ---: |
| Reação, segundos | 0,50 | 0,22 | 0,09 |
| Erro máximo de mira, graus | 23 | 7 | 1,5 |
| Erro máximo de movimento, graus | 25 | 8 | 1,5 |
| Probabilidade de executar movimento por decisão | 0,58 | 0,85 | 0,99 |
| Peso de evasão | 0,3 | 1,2 | 2,2 |
| Raio de evasão, pixels | 65 | 140 | 210 |
| Alcance de busca de itens, pixels | 150 | 450 | 1800 |

Os parâmetros restantes estão nos arquivos `*_configs.csv`. Os controladores ficaram congelados durante exploração, calibração, refinamentos e avaliação. O limite de cada partida foi **600 s simulados ou 20 rounds concluídos**. `round_reached` é o round mais alto entrado; não deve ser confundido com `rounds_completed`. Sobreviventes no limite são censurados à direita: 600 s observados não significam vida infinita nem expectativa completa de vida.

| Fase | Configurações × perfis × sementes | Execuções |
| --- | --- | ---: |
| Exploração inicial | 8 × 3 × 1001–1006 | 144 |
| Refinamento exploratório | 4 × 3 × 1001–1006 | 72 |
| Calibração ampliada inicial | 4 × 3 × 1001–1020 | 240 |
| Calibração ampliada dos refinamentos | 2 × 3 × 1001–1020 | 120 |
| Avaliação reservada | baseline + selecionada × 3 × 9001–9020 | 120 |
| **Total executado** | | **696** |

São **588 combinações distintas de configuração/perfil/semente**, porque expandir a calibração reexecuta 108 condições já exploradas. Não tratamos as 696 execuções como observações independentes. A calibração final contém 360 linhas únicas dentro dessa fase. CSVs consolidados e arquivos individuais equivalentes são aliases de dados, não novas partidas. O teste de reprodução integral é verificação adicional, não uma nova amostra científica.

O arquivo [selection.json](../experiments/balancing/selection.json) registra a escolha, parâmetros e hashes dos dois CSVs de calibração antes da avaliação reservada. Os resumos de heldout registram `selection_allowed=false`; o analisador rejeita misturar calibração e avaliação. Usamos sementes pareadas entre configurações, mas o consumo de RNG pode divergir quando as regras diferem; não se presume que todos os eventos ocorreram nas mesmas posições/instantes.

## Critérios prévios e espaço de busca

O score não mede Flow; é uma preferência de engenharia declarada: 40% proximidade dos rounds médios aos alvos 3/6/10 por perfil, 20% proximidade das derrotas a 65/45/25%, 15% diferença de sete rounds entre avançado e iniciante, 10% reparação/perda próxima de 40%, 10% diversidade de pelo menos três padrões usados e 5% duração de round próxima de 50 s. O protocolo contém as equações. Perfis têm peso igual. Violações dos limites de recursos desclassificam uma configuração.

A seleção não maximizou apenas sobrevivência: um agente avançado que sempre sobrevive pode indicar desafio insuficiente. Entretanto, o score não substitui o exame das taxas por perfil; um vencedor agregado pode continuar inadequado aos iniciantes.

A tabela mostra os valores relevantes efetivamente usados. `Motor` é a chance condicional quando um drop é produzido, antes da política de evitar item inútil com motores cheios. A chance condicional de upgrade é `1−Motor`. As três formações e os demais padrões não multiplicam simultaneamente seus números de projéteis; o padrão ativo é único e os modificadores compatíveis coexistem.

| Configuração | Drop | Motor | Avançado | +vida/round | Spawn, s | Força upgrades | Vida item, s | Asteroide / gravidade |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `baseline` | 0.75 | 0.20 | 0.25 | 10 | 2.0 | 1.00 | 20 | 0.10 / 10 m/s² |
| `procedural_baseline` | 0.75 | 0.20 | 0.25 | 10 | 2.0 | 1.00 | 20 | 0.10 / 10 m/s² |
| `repair` | 0.85 | 0.40 | 0.30 | 8 | 2.0 | 1.00 | 20 | 0.10 / 10 m/s² |
| `balanced` | 0.85 | 0.35 | 0.30 | 6 | 1.9 | 1.15 | 20 | 0.10 / 10 m/s² |
| `gentle` | 0.90 | 0.40 | 0.35 | 4 | 2.2 | 1.15 | 20 | 0.07 / 8 m/s² |
| `pressure` | 0.80 | 0.30 | 0.25 | 10 | 1.6 | 1.20 | 20 | 0.15 / 12 m/s² |
| `variety` | 0.80 | 0.28 | 0.40 | 8 | 1.8 | 1.10 | 20 | 0.10 / 10 m/s² |
| `environment` | 0.85 | 0.35 | 0.30 | 6 | 1.9 | 1.15 | 20 | 0.20 / 15 m/s² |
| `growth14` | 0.90 | 0.50 | 0.30 | 14 | 1.6 | 1.20 | 35 | 0.15 / 12 m/s² |
| `growth18` | 0.90 | 0.60 | 0.30 | 18 | 1.6 | 1.20 | 35 | 0.15 / 12 m/s² |
| `paced` | 0.90 | 0.60 | 0.30 | 14 | 1.3 | 1.20 | 35 | 0.15 / 12 m/s² |
| `recovery_long` | 0.95 | 0.60 | 0.35 | 10 | 1.6 | 1.20 | 45 | 0.15 / 12 m/s² |

Asteroides tentam surgir a cada 5 s e gravidade a cada 60 s nos candidatos; a última coluna varia chance de asteroide e intensidade da gravidade. Todos conservam no máximo oito aliens ativos, 25 aliens por round e 256 projéteis. As probabilidades são valores float registrados com precisão nos CSVs; a tabela arredonda para leitura.

O `baseline` usa os três níveis antigos e permite reparações potencialmente inúteis com motores cheios. `procedural_baseline` conserva os escalares anteriores, mas já reúne a nova progressão **e** a política anti-desperdício. Portanto essa comparação não isola causalmente o efeito da arma. Todas as configurações compartilham a correção que permite disparar em `RoundCompleted`.

## Exploração e seleção na calibração

| Configuração inicial | Score exploratório, n=6/perfil | Vantagem observada | Limite observado |
| --- | ---: | --- | --- |
| `baseline` | 32.990 | Comparador reproduzível do balanceamento anterior. | Intermediário/avançado sem derrotas e diversidade limitada a dois padrões. |
| `procedural_baseline` | 51.597 | Maior diversidade sem trocar escalares básicos. | Avançados ainda progridem muito além do alvo; iniciante pouco coleta. |
| `repair` | 45.350 | Avalia mais reparação com menor crescimento de vida. | Mais probabilidade não garante que o iniciante alcance os itens. |
| `balanced` | 39.182 | Mais drops e upgrades moderados. | Rounds altos e muitas partidas fortes censuradas; nome não prova equilíbrio. |
| `gentle` | 34.591 | Progressão e ambiente mais brandos. | Menor desafio para perfis fortes, sem resolver a coleta inicial. |
| `pressure` | 56.048 | Melhor score inicial e mais risco para perfis fortes. | Rounds 4,50/13,33/14,67 continuam acima dos alvos. |
| `variety` | 41.516 | Maior frequência de avançados testa diversidade de oportunidades. | Maior perigo não assegura coleta; avançados ainda atingem rounds altos. |
| `environment` | 39.122 | Testa maior influência ambiental. | Não se demonstrou benefício isolado; vários parâmetros mudam juntos. |

Como os candidatos iniciais não se aproximaram suficientemente dos alvos de rounds e a recuperação iniciante permaneceu baixa, fizemos quatro refinamentos limitados em sementes de calibração: maior crescimento de vida e janelas de coleta de 35/45 s, além da mistura de itens. Isso foi decidido antes de consultar heldout e sem mudar os agentes nem os pesos.

| Refinamento | Score exploratório | Rounds iniciante/intermediário/avançado | Interpretação |
| --- | ---: | --- | --- |
| `growth18` | 62.391 | 3.67 / 10.33 / 14.00 | Maior aproximação do alvo agregado, com redução de progressão; exige reconhecer custo aos iniciantes. |
| `paced` | 55.580 | 4.50 / 11.00 / 15.17 | Spawn mais rápido; segundo refinamento levado à calibração ampliada. |
| `recovery_long` | 55.524 | 4.83 / 12.83 / 14.83 | Janela longa e mais motores, mas menor proximidade agregada que os refinamentos escolhidos. |
| `growth14` | 52.860 | 4.50 / 13.17 / 15.50 | Aumenta resistência e janela de coleta, mas não foi um dos dois melhores refinamentos. |

A calibração ampliada reuniu o baseline, os três candidatos iniciais mais promissores e os dois refinamentos mais promissores. As sementes 1001–1020 não são validação independente das seis primeiras; serviram apenas para seleção.

| Calibração final, 20 sementes/perfil | Score | Rounds médios iniciante/intermediário/avançado |
| --- | ---: | --- |
| `growth18` | 61.396 | 3.60 / 10.10 / 14.65 |
| `procedural_baseline` | 52.372 | 4.50 / 12.30 / 16.30 |
| `paced` | 51.449 | 4.20 / 11.85 / 16.20 |
| `pressure` | 50.525 | 4.55 / 13.15 / 16.70 |
| `repair` | 48.366 | 4.70 / 12.85 / 16.90 |
| `baseline` | 32.127 | 4.70 / 16.80 / 19.05 |

A maior pontuação elegível foi de `growth18`. Essa escolha e seus parâmetros foram congelados em `selection.json`; não trocamos para outra configuração depois de observar a avaliação seguinte.

## Avaliação reservada: baseline versus recomendação

São 20 sementes inéditas por perfil/configuração. As tabelas usam média e IC95% t para medidas contínuas, e IC95% Wilson para taxas. Não são intervalos corrigidos para múltiplas comparações. O limite de 600 s introduz censura, sobretudo no baseline forte; “sobrevivência” abaixo significa tempo observado restrito ao horizonte. Pequenos desvios de 600 s resultam da representação float do passo fixo.

| Configuração / perfil | Round máximo, média [IC95%] | Sobrevivência observada, s [IC95%] | Derrota até o limite [IC95%] |
| --- | --- | --- | --- |
| `baseline` / beginner | 4.50 [4.08; 4.92] | 163.0 [140.3; 185.7] | 100% [83.9; 100.0] |
| `baseline` / intermediate | 17.10 [16.89; 17.31] | 600.0 [600.0; 600.0] | 0% [0.0; 16.1] |
| `baseline` / advanced | 19.05 [18.95; 19.15] | 600.0 [600.0; 600.0] | 0% [0.0; 16.1] |
| `growth18` / beginner | 3.70 [3.36; 4.04] | 109.4 [89.0; 129.9] | 100% [83.9; 100.0] |
| `growth18` / intermediate | 9.25 [8.02; 10.48] | 311.8 [251.1; 372.5] | 95% [76.4; 99.1] |
| `growth18` / advanced | 13.80 [12.52; 15.08] | 501.8 [433.6; 569.9] | 35% [18.1; 56.7] |

Todos os grupos chegaram ao round 2 em 20/20 partidas; o intervalo de Wilson dessa taxa é [83,9%; 100%]. Isso apoia apenas a acessibilidade inicial desses controladores. Não elimina o fato de que todos os iniciantes foram derrotados antes do horizonte.

| Configuração / perfil | Eliminações médias | Precisão agrupada | Dano médio aos aliens | Motores perdidos / recuperados, médias | Reparados/perdidos | Upgrades médios | Padrões usados, média |
| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| `baseline` / beginner | 31.40 | 17.4% | 2303.5 | 4.05 / 0.05 | 1.2% | 0.10 | 1.00 |
| `baseline` / intermediate | 305.15 | 20.2% | 49200.8 | 1.30 / 1.10 | 84.6% | 2.00 | 2.00 |
| `baseline` / advanced | 356.80 | 22.3% | 62002.0 | 0.15 / 0.15 | 100.0% | 2.00 | 2.00 |
| `growth18` / beginner | 22.95 | 19.9% | 1881.5 | 4.30 / 0.30 | 7.0% | 0.15 | 1.10 |
| `growth18` / intermediate | 113.30 | 33.2% | 18216.2 | 6.15 / 2.30 | 37.4% | 7.90 | 5.70 |
| `growth18` / advanced | 223.85 | 46.5% | 47881.8 | 3.70 / 2.20 | 59.5% | 13.50 | 7.55 |

Precisão agrupada é total de contatos de projéteis com aliens/asteroides dividido pelo total de projéteis inseridos; não é média de “chance de acertar” humana. O dano da tabela é dano efetivo aos aliens; dano a asteroides fica em coluna própria do CSV. Dano recebido é medido em motores, portanto coincide com perdas efetivas, respeitando invulnerabilidade. A fração de reparação é razão dos totais, não a média das razões individuais. Os ICs de todas as métricas, SDs, duração média dos rounds, frequência por minuto de upgrade, picos e custos computacionais estão em `heldout_summary/summary.csv` e `summary.json`.

### Diferenças pareadas e interpretação

Diferenças abaixo são `growth18 − baseline`, combinando a mesma semente/perfil nas duas configurações.

| Perfil | Δ round máximo [IC95%] | Δ sobrevivência observada, s [IC95%] | Δ motores recuperados [IC95%] |
| --- | --- | --- | --- |
| beginner | -0.80 [-1.32; -0.28] | -53.6 [-83.6; -23.5] | 0.25 [-0.15; 0.65] |
| intermediate | -7.85 [-9.10; -6.60] | -288.2 [-348.9; -227.5] | 1.20 [0.19; 2.21] |
| advanced | -5.25 [-6.54; -3.96] | -98.2 [-166.4; -30.1] | 2.05 [1.13; 2.97] |

**Limitação iniciante:** a sobrevivência caiu de 163,0 para 109,4 s e o round médio de 4,50 para 3,70. A diferença pareada de reparações é +0,25 motor por partida, mas seu IC inclui zero. A fração recuperada/perdida passa de 1,2% para 7,0%, continuando baixa. O controlador com alcance de coleta curto raramente converte maior disponibilidade em recuperação; não podemos declarar o problema humano de recuperação resolvido.

**Desafio dos perfis fortes:** o baseline não teve derrota de intermediários nem avançados em 600 s; a recomendação teve 95% e 35%, respectivamente. Há mais risco e diferenciação, mas a meta prévia de 45% para intermediários não foi atingida. O avançado ainda entra em média no round 13,8, acima do alvo 10. Portanto `growth18` é uma recomendação inicial auditável para diversidade e desafio, não uma configuração ótima para todos. O score maior resulta dos pesos declarados, não de evidência psicológica.

**Poder e progressão:** a diversidade realmente usada cresce de 2 para 5,70/7,55 nos perfis intermediário/avançado, mas o dano acumulado e a sobrevivência podem diminuir. As formações distribuem cobertura, diluem dano individual e alteram cadência; não são simplesmente aumentos de DPS concentrado. Não se deve interpretar menos rounds como regressão mecânica automaticamente, nem como melhoria subjetiva automaticamente.

## Eficiência observada por padrão

Agregado das 60 partidas heldout da configuração selecionada. As atribuições são observacionais: padrões surgem em tiers, instantes, perfis e situações diferentes. Não é um ensaio controlado em que só o padrão muda. “Dano/projétil” inclui tiros que erraram e exclui dano em asteroides; os contatos incluem ambos os tipos de alvo. Dano por segundo e experiência subjetiva não podem ser inferidos somente dessa razão.

| Padrão | Projéteis emitidos | Contatos | Dano efetivo em aliens | Contatos/projéteis | Dano/projétil |
| --- | ---: | ---: | ---: | ---: | ---: |
| standard | 23319 | 9983 | 250220 | 42.81% | 10.73 |
| burst | 9495 | 5975 | 138878 | 62.93% | 14.63 |
| heavy | 3534 | 2447 | 135159 | 69.24% | 38.25 |
| circle | 55270 | 9489 | 130772 | 17.17% | 2.37 |
| triangle | 28897 | 16278 | 247124 | 56.33% | 8.55 |
| square | 24035 | 12837 | 159518 | 53.41% | 6.64 |
| spread | 31650 | 11900 | 169343 | 37.60% | 5.35 |
| parallel | 15743 | 8572 | 128574 | 54.45% | 8.17 |

O pesado apresenta alto dano por projétil, mas cobra frequência e velocidade. O circular cobre 360° e emite mais tiros fora de um alvo concentrado; sua precisão baixa é compatível com essa compensação, não prova isolada de inutilidade. Triângulo, quadrado e paralelo mantêm direções concentradas com origens distintas. Rajadas são sucessivas no tempo, e não vários tiros instantâneos com o mesmo nome.

## Custos e limites de recursos

O maior pico em todas as 696 execuções foi **57 projéteis**, abaixo do teto global 256. Na recomendação reservada, o maior pico foi **56**; a emissão por arma permanece limitada a 16 projéteis e a rajada a cinco tiros. Os testes também exercitam centenas de variantes sem violar limites, sem estabelecer um teto global de aquisições.

Os campos `wall_seconds` somam **33,724 s** nas 696 execuções oficiais, correspondendo a aproximadamente **77,1 horas de tempo simulado**. Essa soma mede o trecho interno de execução informado pelo simulador; não inclui toda a configuração, compilação, análise, inicialização de processos nem o tempo humano de implementação. Não é um benchmark comparável entre máquinas, e os tempos não precisam se repetir bit a bit. Os custos médios heldout foram:

| Configuração / perfil | Tempo computacional médio por partida, s | Duração média de round concluído, s |
| --- | ---: | ---: |
| `baseline` / beginner | 0.0090 | 30.48 |
| `baseline` / intermediate | 0.0974 | 33.01 |
| `baseline` / advanced | 0.1055 | 29.77 |
| `growth18` / beginner | 0.0069 | 25.84 |
| `growth18` / intermediate | 0.0368 | 30.69 |
| `growth18` / advanced | 0.0652 | 33.57 |

## Recomendação implementada

`GameConfig::recommended()` aplica exatamente o conjunto congelado:

```text
drop = 0.90; motor condicional = 0.60; upgrade condicional = 0.40
alien avançado (desde round 2) = 0.30
vida normal = 50 + 18*(round-1); multiplicador avançado = 1.5
intervalo inicial de spawn = 1.6 s, reduzido até o piso configurado
força da progressão de armas = 1.2
chance de asteroide por tentativa = 0.15; gravidade = 12 m/s²
vida útil de item = 35 s
```

Quando a nave está ferida, o sorteio inicial de motor é mais frequente que no baseline; quando já tem quatro motores, a política evita criar uma reparação inútil, buscando um upgrade elegível ou deixando de gerar um item sem efeito. Normais continuam sem drops, avançados precisam ser eliminados pelo jogador, posições são validadas, motor recupera um e o teto é quatro. Nenhum agente ou perfil recebe esses itens automaticamente.

`GameConfig{}` conserva escalares do comparador para reprodução; `Main` escolhe `recommended()` para a partida normal. Parâmetros continuam ajustáveis. A semente é variável nas partidas normais, explícita em `--game-seed` e fixa nos experimentos. Aliens, explosões, asteroides, fragmentos, gravidade e portal usam as mesmas implementações reais. A espera em `RoundCompleted` permite atirar nas rochas sobreviventes.

## Artefatos e reprodução

- Dados iniciais: `experiments/balancing/exploration.csv` e `exploration_configs.csv`.
- Refinamentos: `refinement_exploration/runs.csv`, `configs.csv` e `commands.json`.
- Calibração: `calibration.csv` e `refinement_calibration/runs.csv`; o resumo consolidado de 360 condições fica em `final_calibration_summary/`.
- Escolha congelada: `selection.json`, com hashes dos dados de calibração.
- Avaliação: `heldout_baseline.csv`, `heldout_selected.csv`, seus `*_configs.csv` e `heldout_summary/`.
- Snapshot dos fontes no início dos experimentos: hashes em `experiment_source.sha256`; fontes entregues em `final_source.sha256`. Após o snapshot foram acrescentados a factory recomendada, HUD, metadados CSV e validação de CLI; o replay integral confirmou gameplay idêntico nos fontes finais.

Para compilar e repetir a sequência completa em outro destino:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/release --parallel
python3 tools/reproduce_balancing.py \
  --simulator build/release/space_arena_sim \
  --output-dir build/balancing-reproduction \
  --verify-against experiments/balancing
```

O wrapper executa a busca registrada, a seleção somente por calibração e a avaliação posterior. A verificação compara campos determinísticos e a escolha final; `wall_seconds` é excluído da igualdade. Os resultados de execução desse gate e das suítes estão em [VALIDATION.md](VALIDATION.md).

Comandos menores para inspecionar ou recalcular estatísticas:

```sh
# Um conjunto experimental; --max-rounds conta rounds concluidos
./build/release/space_arena_sim --preset all --profile all \
  --seed-start 1001 --runs 6 --seconds 600 --max-rounds 20 \
  --output build/exploration.csv --configs-output build/exploration_configs.csv

# Refina apenas sementes de calibracao, sem consultar heldout
python3 tools/run_refinement.py --simulator build/release/space_arena_sim \
  --runs 6 --output-dir build/refinement

# Recalcula a avaliacao reservada, sem permitir nova selecao
python3 tools/analyze_balancing.py \
  --input experiments/balancing/heldout_baseline.csv experiments/balancing/heldout_selected.csv \
  --output-dir build/heldout-analysis

./build/release/space_arena --game-seed 42
```

## Limites e próxima validação

Esta entrega implementa e testa a progressão e torna o balanceamento investigável/reproduzível. Não prova Flow humano, não determina dificuldade universal e não corrige por completo a recuperação de iniciantes. Sementes e perfis são amostras artificiais; escolhas de alvo, evasão e coleta dos bots não representam toda a variedade humana. Os ICs são descritivos, os score targets são preferências e a busca testou conjuntos de parâmetros, sem atribuição causal isolada a cada um.

A avaliação cobre até 600 s/20 rounds concluídos por partida; não garante equilíbrio em rounds arbitrariamente altos. Ter variantes úteis e limites de emissão não demonstra, por si só, poder adequadamente calibrado para duração infinita. Não alteramos regras, agentes ou recomendação após consultar heldout.

A próxima validação deve usar jogadores humanos de habilidades diferentes, observando clareza dos itens e padrões, estratégias de coleta, sentimento de controle, desafio percebido e questionários apropriados. A dificuldade iniciante e a interação entre reparações e sobrevivência merecem prioridade. Só então haverá base para decidir se DDA gradual é necessário e quais sinais devem orientá-lo.
