#ifndef GAME_ASTAR_HPP
#define GAME_ASTAR_HPP

#include "Vector2D.hpp"

#include <string>
#include <vector>

/**
 * Encontra um caminho minimo em uma grade retangular: '.' livre, '#' parede.
 * As coordenadas sao indices inteiros de celulas (x = coluna, y = linha),
 * com origem em (0, 0). Ha quatro vizinhos, sem diagonais, e custo 1 por passo.
 * Usa A*: f(n) = g(n) + h(n), com h igual a distancia Manhattan ao destino.
 *
 * Retorna as celulas em ordem, incluindo inicio e destino. Retorna um vetor
 * vazio se nao houver caminho ou se a grade/as posicoes forem invalidas.
 * Inicio igual ao destino livre retorna um caminho com uma unica celula.
 * Nao altera a grade nem guarda estado entre buscas; nao depende de SFML.
 */
[[nodiscard]] std::vector<Vector2D> findPathAStar(
    const std::vector<std::string>& maze,
    const Vector2D& start,
    const Vector2D& goal);

#endif // GAME_ASTAR_HPP
