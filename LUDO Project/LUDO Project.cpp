#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <optional>
#include <string>

#define TEST_MODE

const int CELL = 40;

// -1 = yard, 0..50 = main track, 51..56 = home lane, 57 = finished
int pos[4][4];

// token screen coordinates
float px[4][4], py[4][4];

// board coordinates
float yardX[4][4], yardY[4][4];
float homeX[4][6], homeY[4][6];
float pathX[52], pathY[52];

// start index on main track for Red, Green, Yellow, Blue
int startIdx[4] = { 0, 13, 26, 39 };

// game state
int turn = 0;
int dice = 0;
bool rolled = false;
bool stuck = false;
int win = -1;

sf::Color colors[4] = {
    sf::Color(220, 50, 50),   // Red
    sf::Color(50, 180, 50),   // Green
    sf::Color(240, 200, 40),  // Yellow
    sf::Color(50, 100, 220)   // Blue
};

void initBoard() {
    for (int p = 0; p < 4; p++) {
        for (int t = 0; t < 4; t++) {
            pos[p][t] = -1;
        }
    }

    int col[52] = {
        6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0,
        1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 7, 8,
        8, 8, 8, 8, 8, 9, 10, 11, 12, 13, 14, 14, 14,
        13, 12, 11, 10, 9, 8, 8, 8, 8, 8, 8, 7
    };
    int row[52] = {
        13, 12, 11, 10, 9, 8, 8, 8, 8, 8, 8, 7, 6,
        6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0,
        1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 7, 8,
        8, 8, 8, 8, 8, 9, 10, 11, 12, 13, 14, 14
    };

    for (int i = 0; i < 52; i++) {
        pathX[i] = col[i] * CELL + 20.f;
        pathY[i] = row[i] * CELL + 20.f;
    }

    // Red yard (bottom-left)
    yardX[0][0] = 1.5f * CELL; yardY[0][0] = 10.5f * CELL;
    yardX[0][1] = 4.5f * CELL; yardY[0][1] = 10.5f * CELL;
    yardX[0][2] = 1.5f * CELL; yardY[0][2] = 13.5f * CELL;
    yardX[0][3] = 4.5f * CELL; yardY[0][3] = 13.5f * CELL;

    // Green yard (top-left)
    yardX[1][0] = 1.5f * CELL; yardY[1][0] = 1.5f * CELL;
    yardX[1][1] = 4.5f * CELL; yardY[1][1] = 1.5f * CELL;
    yardX[1][2] = 1.5f * CELL; yardY[1][2] = 4.5f * CELL;
    yardX[1][3] = 4.5f * CELL; yardY[1][3] = 4.5f * CELL;

    // Yellow yard (top-right)
    yardX[2][0] = 10.5f * CELL; yardY[2][0] = 1.5f * CELL;
    yardX[2][1] = 13.5f * CELL; yardY[2][1] = 1.5f * CELL;
    yardX[2][2] = 10.5f * CELL; yardY[2][2] = 4.5f * CELL;
    yardX[2][3] = 13.5f * CELL; yardY[2][3] = 4.5f * CELL;

    // Blue yard (bottom-right)
    yardX[3][0] = 10.5f * CELL; yardY[3][0] = 10.5f * CELL;
    yardX[3][1] = 13.5f * CELL; yardY[3][1] = 10.5f * CELL;
    yardX[3][2] = 10.5f * CELL; yardY[3][2] = 13.5f * CELL;
    yardX[3][3] = 13.5f * CELL; yardY[3][3] = 13.5f * CELL;

    // 6 home lane tiles for each player
    for (int i = 0; i < 6; i++) {
        homeX[0][i] = 7 * CELL + 20.f;        homeY[0][i] = (13 - i) * CELL + 20.f;
        homeX[1][i] = (1 + i) * CELL + 20.f;  homeY[1][i] = 7 * CELL + 20.f;
        homeX[2][i] = 7 * CELL + 20.f;        homeY[2][i] = (1 + i) * CELL + 20.f;
        homeX[3][i] = (13 - i) * CELL + 20.f; homeY[3][i] = 7 * CELL + 20.f;
    }
}

void updateTokens() {
    for (int p = 0; p < 4; p++) {
        for (int t = 0; t < 4; t++) {
            int step = pos[p][t];

            if (step == -1) {
                px[p][t] = yardX[p][t];
                py[p][t] = yardY[p][t];
            }
            else if (step <= 50) {
                int idx = (startIdx[p] + step) % 52;
                px[p][t] = pathX[idx];
                py[p][t] = pathY[idx];
            }
            else if (step <= 56) {
                int idx = step - 51;
                px[p][t] = homeX[p][idx];
                py[p][t] = homeY[p][idx];
            }
            else {
                px[p][t] = 7.5f * CELL;
                py[p][t] = 7.5f * CELL;
            }
        }
    }
}

bool isSafe(int idx) {
    return (idx == 0 || idx == 8 || idx == 13 || idx == 21 ||
        idx == 26 || idx == 34 || idx == 39 || idx == 47);
}

bool cutEnemy(int p, int t) {
    int step = pos[p][t];
    if (step < 0 || step > 50) return false;

    int myIdx = (startIdx[p] + step) % 52;
    if (isSafe(myIdx)) return false;

    bool cut = false;
    for (int ep = 0; ep < 4; ep++) {
        if (ep == p) continue;

        for (int et = 0; et < 4; et++) {
            int eStep = pos[ep][et];
            if (eStep >= 0 && eStep <= 50) {
                int eIdx = (startIdx[ep] + eStep) % 52;
                if (myIdx == eIdx) {
                    pos[ep][et] = -1;
                    cut = true;
                }
            }
        }
    }
    return cut;
}

bool canMove(int p, int roll) {
    for (int t = 0; t < 4; t++) {
        if (pos[p][t] == -1 && roll == 6) return true;
        if (pos[p][t] >= 0 && pos[p][t] + roll <= 57) return true;
    }
    return false;
}

bool hasWon(int p) {
    for (int t = 0; t < 4; t++) {
        if (pos[p][t] != 57) return false;
    }
    return true;
}

int main() {
    srand((unsigned)time(0));

    initBoard();
    updateTokens();

    sf::RenderWindow winApp(sf::VideoMode({ 800, 600 }), "Ludo");
    winApp.setFramerateLimit(60);

    sf::Font font;
    bool hasFont = font.openFromFile("C:/Windows/Fonts/arial.ttf") || font.openFromFile("arial.ttf");

    sf::Text msg(font);
    sf::Text diceTxt(font);
    if (hasFont) {
        msg.setCharacterSize(18);
        msg.setFillColor(sf::Color::Black);

        diceTxt.setCharacterSize(48);
        diceTxt.setFillColor(sf::Color::Black);
    }

    while (winApp.isOpen()) {
        while (const std::optional ev = winApp.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                winApp.close();
            }

#ifdef TEST_MODE
            else if (const auto* key = ev->getIf<sf::Event::KeyPressed>()) {
                if (!rolled && win == -1) {
                    int n = 0;
                    if (key->code == sf::Keyboard::Key::Num1) n = 1;
                    else if (key->code == sf::Keyboard::Key::Num2) n = 2;
                    else if (key->code == sf::Keyboard::Key::Num3) n = 3;
                    else if (key->code == sf::Keyboard::Key::Num4) n = 4;
                    else if (key->code == sf::Keyboard::Key::Num5) n = 5;
                    else if (key->code == sf::Keyboard::Key::Num6) n = 6;

                    if (n > 0) {
                        dice = n;
                        rolled = true;
                        if (!canMove(turn, dice)) stuck = true;
                    }
                }
            }
#endif

            else if (const auto* click = ev->getIf<sf::Event::MouseButtonPressed>()) {
                if (click->button == sf::Mouse::Button::Left && win == -1) {
                    float mx = (float)click->position.x;
                    float my = (float)click->position.y;

                    // clicked dice box
                    if (mx >= 630 && mx <= 730 && my >= 220 && my <= 320) {
                        if (!rolled) {
                            dice = (rand() % 6) + 1;
                            rolled = true;
                            if (!canMove(turn, dice)) stuck = true;
                        }
                        else if (stuck) {
                            rolled = false;
                            stuck = false;
                            dice = 0;
                            turn = (turn + 1) % 4;
                        }
                    }

                    // clicked a token
                    if (rolled && !stuck) {
                        for (int t = 0; t < 4; t++) {
                            float dx = mx - px[turn][t];
                            float dy = my - py[turn][t];

                            if (dx * dx + dy * dy <= 225.f) {
                                bool moved = false;

                                if (pos[turn][t] == -1 && dice == 6) {
                                    pos[turn][t] = 0;
                                    moved = true;
                                }
                                else if (pos[turn][t] >= 0 && pos[turn][t] + dice <= 57) {
                                    pos[turn][t] += dice;
                                    moved = true;
                                }

                                if (moved) {
                                    bool cut = cutEnemy(turn, t);
                                    updateTokens();

                                    if (hasWon(turn)) win = turn;

                                    if (dice == 6 || cut) {
                                        rolled = false;
                                    }
                                    else {
                                        rolled = false;
                                        dice = 0;
                                        turn = (turn + 1) % 4;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        winApp.clear(sf::Color(245, 245, 245));

        // 4 yards
        sf::RectangleShape box({ 240.f, 240.f });
        box.setPosition({ 0.f, 360.f });   box.setFillColor(colors[0]); winApp.draw(box);
        box.setPosition({ 0.f, 0.f });     box.setFillColor(colors[1]); winApp.draw(box);
        box.setPosition({ 360.f, 0.f });   box.setFillColor(colors[2]); winApp.draw(box);
        box.setPosition({ 360.f, 360.f }); box.setFillColor(colors[3]); winApp.draw(box);

        // center home box
        sf::RectangleShape mid({ 120.f, 120.f });
        mid.setPosition({ 240.f, 240.f });
        mid.setFillColor(sf::Color(200, 200, 200));
        winApp.draw(mid);

        // grid tiles
        sf::RectangleShape cell({ CELL - 2.f, CELL - 2.f });
        cell.setOutlineThickness(1.f);
        cell.setOutlineColor(sf::Color(180, 180, 180));

        for (int r = 0; r < 15; r++) {
            for (int c = 0; c < 15; c++) {
                if ((r < 6 && c < 6) || (r < 6 && c > 8) || (r > 8 && c < 6) || (r > 8 && c > 8)) {
                    continue;
                }

                cell.setPosition({ c * 40.f + 1.f, r * 40.f + 1.f });
                cell.setFillColor(sf::Color::White);

                if (c == 7 && r >= 9 && r <= 13) cell.setFillColor(colors[0]);
                if (r == 7 && c >= 1 && c <= 5)  cell.setFillColor(colors[1]);
                if (c == 7 && r >= 1 && r <= 5)  cell.setFillColor(colors[2]);
                if (r == 7 && c >= 9 && c <= 13) cell.setFillColor(colors[3]);

                if (r == 13 && c == 6) cell.setFillColor(colors[0]);
                if (r == 6 && c == 1)  cell.setFillColor(colors[1]);
                if (r == 1 && c == 8)  cell.setFillColor(colors[2]);
                if (r == 8 && c == 13) cell.setFillColor(colors[3]);

                winApp.draw(cell);
            }
        }

        // safe zone dots
        sf::CircleShape dot(7.f);
        dot.setOrigin({ 7.f, 7.f });
        dot.setFillColor(sf::Color(140, 140, 140));
        int safeIdx[4] = { 8, 21, 34, 47 };
        for (int i = 0; i < 4; i++) {
            dot.setPosition({ pathX[safeIdx[i]], pathY[safeIdx[i]] });
            winApp.draw(dot);
        }

        // right panel
        sf::RectangleShape panel({ 200.f, 600.f });
        panel.setPosition({ 600.f, 0.f });
        panel.setFillColor(sf::Color(230, 230, 230));
        winApp.draw(panel);

        // turn circle
        sf::CircleShape badge(22.f);
        badge.setPosition({ 660.f, 50.f });
        badge.setFillColor(colors[turn]);
        winApp.draw(badge);

        // dice box
        sf::RectangleShape diceBox({ 100.f, 100.f });
        diceBox.setPosition({ 630.f, 220.f });
        diceBox.setFillColor(sf::Color::White);
        diceBox.setOutlineThickness(3.f);
        diceBox.setOutlineColor(sf::Color::Black);
        winApp.draw(diceBox);

        if (hasFont) {
            if (rolled) {
                diceTxt.setString(std::to_string(dice));
                diceTxt.setPosition({ 665.f, 240.f });
                winApp.draw(diceTxt);
            }

            if (win != -1) msg.setString("PLAYER WINS!");
            else if (!rolled) msg.setString("Click Dice\n(or Press 1-6)");
            else if (stuck) msg.setString("No Moves!\nClick Dice");
            else msg.setString("Click a Token");

            msg.setPosition({ 630.f, 350.f });
            winApp.draw(msg);
        }

        // tokens with stacking offset support
        sf::CircleShape tok(14.f);
        tok.setOrigin({ 14.f, 14.f });
        tok.setOutlineThickness(2.f);
        tok.setOutlineColor(sf::Color::Black);

        for (int p = 0; p < 4; p++) {
            for (int t = 0; t < 4; t++) {
                float x = px[p][t];
                float y = py[p][t];

                // Check how many tokens share this position and find our rank among them
                int sameCount = 0;
                int myIndex = 0;

                for (int ep = 0; ep < 4; ep++) {
                    for (int et = 0; et < 4; et++) {
                        if (std::abs(px[ep][et] - x) < 0.5f && std::abs(py[ep][et] - y) < 0.5f) {
                            if (ep < p || (ep == p && et < t)) {
                                myIndex++;
                            }
                            sameCount++;
                        }
                    }
                }

                // If multiple tokens are stacked here, apply a small offset fan-out
                if (sameCount > 1) {
                    float offX[16] = { -6,  6, -6,  6, -10,  10,   0,   0, -8,  8, -8,  8,  -4,   4, -12,  12 };
                    float offY[16] = { -6, -6,  6,  6,   0,   0, -10,  10, -8, -8,  8,  8, -12, -12,   4,   4 };
                    int idx = myIndex % 16;
                    x += offX[idx];
                    y += offY[idx];
                }

                tok.setPosition({ x, y });
                tok.setFillColor(colors[p]);
                winApp.draw(tok);
            }
        }

        winApp.display();
    }

    return 0;
}