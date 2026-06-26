// cd C:/Users/Azerty/Desktop/Программы/filescpp; g++ chess.cpp -o chess.exe -I c:\Users\Azerty\Downloads\SFML-2.6.1/include -L c:\Users\Azerty\Downloads\SFML-2.6.1/lib -lsfml-graphics-d -lsfml-window-d -lsfml-system-d -lopengl32 -lwinmm -lgdi32 -lcomdlg32 -O3 -fopenmp -std=c++20
// cd C:/Users/Azerty/Desktop/Программы/filescpp ; C:\Users\Azerty\AppData\Local\Android\Sdk\ndk\28.2.13676358\toolchains\llvm\prebuilt\windows-x86_64\bin\clang++.exe --target=x86_64-w64-mingw32 chess.cpp -o chess.exe -I c:\Users\Azerty\Downloads\SFML-2.6.1/include -L c:\Users\Azerty\Downloads\SFML-2.6.1/lib -I C:\Users\Azerty\Downloads\mingw64\include -L C:\Users\Azerty\Downloads\mingw64\lib -lsfml-graphics-d -lsfml-window-d -lsfml-system-d -lopengl32 -lwinmm -lgdi32 -lcomdlg32 -static -O3 -ffast-math -ffp-contract=fast -funroll-loops -lwinpthread -fno-exceptions -fno-rtti -std=c++20
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <math.h>
#include <filesystem> // C++17
#include <fstream>
#include <sstream>
#include <thread>
#include <optional>
#include <windows.h>
#include <commdlg.h>
#include <omp.h>
#include "el_lange/el_alocator.cpp"
#include "graphs.hpp"

namespace fs = std::filesystem;
template <typename T, int sizex, int sizey>
class Array2D {
private:
    T data[sizex*sizey];
public:
    Array2D() = default;
    T& operator()(int indexx, int indexy) {
        // if (indexx>=sizex || indexy>=sizey) [[unlikely]] {
        //     std::cerr << "get indexs:" << indexx << "," << indexy << std::endl;
        //     exit(1);
        // }
        return data[indexy*sizex+indexx];
    }

    const T& operator()(int indexx, int indexy) const {
        // if (indexx>=sizex || indexy>=sizey) [[unlikely]] {
        //     std::cerr << "get indexs:" << indexx << "," << indexy << std::endl;
        //     exit(1);
        // }
        return data[indexy*sizex+indexx];
    }
};
class Anim {
private:
    sf::Clock clock;
public:
    float anim_time;
    sf::Vector2i start_pos;
    sf::Vector2i end_pos;
    Anim(float anim_time, sf::Vector2i start_pos, sf::Vector2i end_pos): anim_time(anim_time), start_pos(start_pos), end_pos(end_pos) {
        clock.restart();
    }
    Anim(float anim_time, std::pair<uint8_t, uint8_t> start_pos, std::pair<uint8_t, uint8_t> end_pos): anim_time(anim_time), start_pos(sf::Vector2i(start_pos.first, start_pos.second)), end_pos(sf::Vector2i(end_pos.first, end_pos.second)) {
        clock.restart();
    }
    sf::Vector2f get_current_pos() {
        float left_time = clock.getElapsedTime().asSeconds();
        sf::Vector2f diff = sf::Vector2f(end_pos-start_pos);
        sf::Vector2f start_posf = sf::Vector2f(start_pos);
        float pg = std::min(left_time, anim_time)/anim_time;
        float rad = pg*3.14159265*0.5;
        float pg_dis = sin(rad);
        return start_posf+diff*pg_dis;
    }
    bool is_end() {
        float left_time = clock.getElapsedTime().asSeconds();
        return anim_time<=left_time;
    }
};
struct figureType {
    // Внутренний тип данных
    int8_t value;

    // Конструктор для инициализации
    figureType() = default;
    constexpr figureType(int8_t v) : value(v) {}

    static const figureType Empty;
    static const figureType Pawn;
    static const figureType Knight;
    static const figureType Bishop;
    static const figureType Rook;
    static const figureType Queen;
    static const figureType King;

    constexpr operator int8_t() const { return abs(value); }

    friend figureType operator+(figureType, figureType) = delete;
    friend figureType operator-(figureType, figureType) = delete;
    friend figureType operator*(figureType, figureType) = delete;
    friend figureType operator/(figureType, figureType) = delete;

    // Операторы сравнения для удобства
    constexpr bool operator==(const figureType& other) const { return (value==other.value) || (value==-other.value); }
    constexpr bool operator!=(const figureType& other) const { return !((value==other.value) || (value==-other.value)); }
};
inline constexpr figureType figureType::Empty{0};
inline constexpr figureType figureType::Pawn{1};
inline constexpr figureType figureType::Knight{3};
inline constexpr figureType figureType::Bishop{4};
inline constexpr figureType figureType::Rook{5};
inline constexpr figureType figureType::Queen{9};
inline constexpr figureType figureType::King{40};
struct figure {
    figureType type;
    //bool color; // if true, it's white
    bool getColor() {
        return type.value>0;
    }
    void setColor(bool color) {
        if (getColor()!=color) {
            type.value = -type.value;
        }
    }
    void setType(figureType f) {
        if (getColor()) {
            type.value = f.value;
        } else {
            type.value = -f.value;
        }
    }
};
struct Textures_struct {
    sf::Texture King_img;
    sf::Texture Queen_img;
    sf::Texture Rook_img;
    sf::Texture Bishop_img;
    sf::Texture Knight_img;
    sf::Texture Pawn_img;
    Textures_struct() {
        if (!King_img.loadFromFile("chess_imgs/King.png") || 
            !Queen_img.loadFromFile("chess_imgs/Queen.png") ||
            !Rook_img.loadFromFile("chess_imgs/Rook.png") ||
            !Bishop_img.loadFromFile("chess_imgs/Bishop.png") ||
            !Knight_img.loadFromFile("chess_imgs/Knight.png") ||
            !Pawn_img.loadFromFile("chess_imgs/Pawn.png")
        ) {
            std::cerr << "don't found all imgs :(" << std::endl;
        }
    }
    const sf::Texture* getTexture(figureType f) {
        switch (f) {
        case figureType::Empty:
            return &Pawn_img;
        case figureType::King:
            return &King_img;
        case figureType::Queen:
            return &Queen_img;
        case figureType::Rook:
            return &Rook_img;
        case figureType::Bishop:
            return &Bishop_img;
        case figureType::Knight:
            return &Knight_img;
        case figureType::Pawn:
            return &Pawn_img;
        }
    }
};
Textures_struct Textures;
std::vector<std::string> splitBySpaces(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}

sf::Font defaultFont;
int showPromotionWindow(
    sf::RenderWindow& mainWindow, 
    const std::vector<std::pair<const sf::Texture*, std::wstring>>& buttonsData,
    const std::wstring& windowTitle = L"Превращение пешки",
    const std::wstring& headerTextStr = L"Выберите фигуру для превращения:"
) {
    sf::RenderWindow promoWindow(sf::VideoMode(600, 250), windowTitle, sf::Style::Default);
    promoWindow.setFramerateLimit(30);

    sf::Text headerText(headerTextStr, defaultFont, 20);
    headerText.setFillColor(sf::Color::White);

    size_t n = buttonsData.size();
    std::vector<sf::RectangleShape> buttonBackgrounds(n);
    std::vector<sf::Sprite> buttonSprites(n);
    std::vector<sf::Text> buttonTexts(n);

    for (size_t i = 0; i < n; ++i) {
        buttonTexts[i].setFont(defaultFont);
        buttonTexts[i].setString(buttonsData[i].second);
        buttonTexts[i].setCharacterSize(16);
        buttonTexts[i].setFillColor(sf::Color::Black);

        if (buttonsData[i].first) {
            buttonSprites[i].setTexture(*(buttonsData[i].first));
        }

        buttonBackgrounds[i].setFillColor(sf::Color(200, 200, 200));
        buttonBackgrounds[i].setOutlineThickness(2.f);
        buttonBackgrounds[i].setOutlineColor(sf::Color(100, 100, 100));
    }

    int selectedIndex = -1;

    while (promoWindow.isOpen() && mainWindow.isOpen()) {
        sf::Event event;

        while (mainWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                mainWindow.close();
                promoWindow.close();
                return -1;
            }
        }

        while (promoWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                promoWindow.close();
                return -1;
            } else if (event.type==sf::Event::Resized) {
                sf::View view(sf::FloatRect(0.f, 0.f, (float)event.size.width, (float)event.size.height));
                promoWindow.setView(view);
            } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(promoWindow);
                for (size_t i = 0; i < n; ++i) {
                    if (buttonBackgrounds[i].getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        selectedIndex = static_cast<int>(i);
                        promoWindow.close();
                    }
                }
            }
        }

        sf::Vector2u winSize = promoWindow.getSize();
        
        headerText.setPosition(20.f, 15.f);

        float padding = 15.f;
        float startY = headerText.getGlobalBounds().top + headerText.getGlobalBounds().height + 25.f;
        
        float btnWidth = 110.f;
        float btnHeight = 120.f;

        bool isVerticalMode = (winSize.y > winSize.x) || ((btnWidth + padding) * n > winSize.x);

        if (isVerticalMode) {
            btnWidth = std::max(150.f, static_cast<float>(winSize.x) - padding * 2.f);
            btnHeight = std::max(40.f, (static_cast<float>(winSize.y) - startY - padding * n) / n);
        } else {
            btnWidth = std::max(60.f, (static_cast<float>(winSize.x) - padding * (n + 1)) / n);
            btnHeight = std::max(80.f, static_cast<float>(winSize.y) - startY - padding * 2.f);
        }

        for (size_t i = 0; i < n; ++i) {
            float x = 0.0f, y = 0.0f;
            if (isVerticalMode) {
                x = padding;
                y = startY + i * (btnHeight + padding);
            } else {
                x = padding + i * (btnWidth + padding);
                y = startY;
            }

            buttonBackgrounds[i].setSize(sf::Vector2f(btnWidth, btnHeight));
            buttonBackgrounds[i].setPosition(x, y);

            // Подсвечиваем кнопку при наведении мыши (Hover effect)
            sf::Vector2i mousePos = sf::Mouse::getPosition(promoWindow);
            if (buttonBackgrounds[i].getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                buttonBackgrounds[i].setFillColor(sf::Color(230, 230, 230));
            } else {
                buttonBackgrounds[i].setFillColor(sf::Color(190, 190, 190));
            }

            if (buttonsData[i].first) {
                sf::Vector2u texSize = buttonsData[i].first->getSize();
                
                float scale = std::min((btnWidth * 0.5f) / texSize.x, (btnHeight * 0.5f) / texSize.y);
                if (scale > 1.f) scale = 1.f; 

                buttonSprites[i].setScale(scale, scale);
                
                float spriteX = x + (btnWidth - texSize.x * scale) / 2.f;
                float spriteY = isVerticalMode ? (y + (btnHeight - texSize.y * scale) / 2.f) : (y + 10.f);
                
                if (isVerticalMode) {
                    spriteX = x + 15.f;
                }

                buttonSprites[i].setPosition(spriteX, spriteY);
            }

            float textX = x + (btnWidth - buttonTexts[i].getGlobalBounds().width) / 2.f;
            float textY = y + btnHeight - buttonTexts[i].getGlobalBounds().height - 15.f;
            
            if (isVerticalMode) {
                textX = x + btnWidth * 0.4f + (btnWidth * 0.6f - buttonTexts[i].getGlobalBounds().width) / 2.f;
                textY = y + (btnHeight - buttonTexts[i].getGlobalBounds().height) / 2.f - 5.f;
            }

            buttonTexts[i].setPosition(textX, textY);
        }

        promoWindow.clear(sf::Color(45, 45, 48)); // Темный фон окна
        
        promoWindow.draw(headerText);
        for (size_t i = 0; i < n; ++i) {
            promoWindow.draw(buttonBackgrounds[i]);
            if (buttonsData[i].first) {
                promoWindow.draw(buttonSprites[i]);
            }
            promoWindow.draw(buttonTexts[i]);
        }
        
        promoWindow.display();
    }

    return selectedIndex;
}
std::string* keyToString(sf::Keyboard::Key keyCode) {
    if (keyCode >= sf::Keyboard::Num0 && keyCode <= sf::Keyboard::Num9) {
        return new std::string(1, static_cast<char>('0' + (keyCode - sf::Keyboard::Num0)));
    }
    return nullptr;
}
int getNumber(const std::string& title, const std::string& content, sf::RenderWindow& mainWindow) {
    sf::VideoMode vidioMode(300, 200);
    sf::RenderWindow input_window(vidioMode, title);
    input_window.setFramerateLimit(30);

    sf::Text contentText;
    contentText.setString(content);
    contentText.setCharacterSize(std::min(20, (int)(600/content.length())));
    contentText.setFont(defaultFont);
    contentText.setPosition(sf::Vector2f(10.f, 10.f));
    contentText.setFillColor(sf::Color::Black);
    sf::Text inputText;
    inputText.setString("Input: ");
    inputText.setCharacterSize(20);
    inputText.setPosition(sf::Vector2f(5, 40));
    inputText.setFont(defaultFont);
    inputText.setFillColor(sf::Color::Black);
    std::string input_string = "";
    sf::Event event;
    while (input_window.isOpen() && mainWindow.isOpen()) {
        while (input_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                input_string = "";
                input_window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    input_window.close();
                } else if (event.key.code == sf::Keyboard::BackSpace) {
                    if (input_string != "") {
                        input_string.pop_back();
                        inputText.setString("Input: "+input_string);
                    }
                } else if (event.key.code == sf::Keyboard::Subtract) {
                    input_string += "-";
                    inputText.setString("Input: "+input_string);
                }
                std::string* char_string = keyToString(event.key.code);
                if (char_string != nullptr) {
                    std::string string = *char_string;
                    input_string.append(string);
                    delete char_string;
                    inputText.setString("Input: "+input_string);
                }
            }

        }
        while (mainWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                input_window.close();
                mainWindow.close();
            }
        }
        input_window.clear(sf::Color::White);
        input_window.draw(contentText);
        input_window.draw(inputText);
        input_window.display();
        //mainWindow.display();
    }
    mainWindow.setActive(true);
    if (input_string == "") {
        return -1;
    }
    try {
        return std::stoi(input_string);
    } catch (std::exception& _) {
        return -1;
    }
}
const std::vector<figureType> ftcf = {figureType::Queen, figureType::Knight, figureType::Rook, figureType::Bishop};
struct Board {
    Array2D<figure, 8, 8> board;
    std::vector<Anim> animations;
    int x=0;
    int y=0;
    int size=600;
    std::pair<uint8_t, uint8_t> select_ceil = {-1, -1};
    bool ccolor = true;
    bool bot = false;
    bool no_rotate_screen = true;
    bool bot_thinking = false;
    int victory_type = 0; // 0 - нет. 1 - победа белых. 2 - ничья. 3 - победа чёрных
    std::vector<std::pair<std::pair<figure, std::pair<figure, figure>>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>>> history;
    int history_pos = 0;
    Graph graph_score_white = {"score_white", {}, sf::Color(170, 170, 170), 1};
    Graph graph_score_black = {"score_black", {}, sf::Color(20, 20, 20), 1};
    Graph graph_score_step_white = {"score_white_step", {}, sf::Color(150, 150, 150), 2};
    Graph graph_score_step_black = {"score_black_step", {}, sf::Color(40, 40, 40), 2};
    Graph graph_max_score_step_white = {"score_white_max_step", {}, sf::Color(130, 130, 130), 2};
    Graph graph_max_score_step_black = {"score_black_max_step", {}, sf::Color(60, 60, 60), 2};
    std::shared_ptr<std::optional<ChessAnalyticsWindow>> analytics;
    std::optional<std::pair<int, std::pair<std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>, figure>>> hint;
    int bot_depth = 6;

    inline std::vector<std::pair<uint8_t, uint8_t>, LinearPoolAllocator<std::pair<uint8_t, uint8_t>>> get_steps(int x, int y) {
        figure f = board(x, y);
        #define add_stepif(x, y, add) if (0<=x && y>=0 && x<8 && y<8 && (board(x, y).type==figureType::Empty || board(x, y).getColor()!=f.getColor())) {steps.emplace_back(x, y);add}
        #define add_stepifna(x, y, add) if (x>=0 && y>=0 && x<8 && y<8 && board(x, y).type==figureType::Empty) {steps.emplace_back(x, y);add}
        #define add_stepifa(x, y) if (x>=0 && y>=0 && x<8 && y<8 && board(x, y).getColor()!=f.getColor() && board(x, y).type!=figureType::Empty) {steps.emplace_back(x, y);}
        #define add_lines(...) for (auto step: __VA_ARGS__) {std::pair<int8_t, int8_t> last_pos = {x, y}; while (true) { last_pos.first+=step.first; last_pos.second+=step.second; add_stepif(last_pos.first, last_pos.second, if(board(last_pos.first, last_pos.second).type!=figureType::Empty) {break;}) else {break;}}}
        #define add_diagonal add_lines(std::vector<std::pair<int8_t, int8_t>>{{1, 1}, {-1, 1}, {-1, -1}, {1, -1}})
        #define add_straight add_lines(std::vector<std::pair<int8_t, int8_t>>{{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
        std::vector<std::pair<uint8_t, uint8_t>, LinearPoolAllocator<std::pair<uint8_t, uint8_t>>> steps;
        switch (f.type) {
        case figureType::Empty:
            return steps;
        case figureType::King:
            steps.reserve(8);
            //{{x-1, y-1}, {x, y-1}, {x+1, y-1}, {x+1, y}, {x+1, y+1}, {x, y+1}, {x-1, y+1}, {x-1, y}}

            add_stepif(x-1, y-1,);
            add_stepif(x, y-1,);
            add_stepif(x+1, y-1,);
            add_stepif(x+1, y,);
            add_stepif(x+1, y+1,);
            add_stepif(x, y+1,);
            add_stepif(x-1, y+1,);
            add_stepif(x-1, y,);
            return steps;
        case figureType::Knight:
            steps.reserve(8);
            
            add_stepif(x - 1, y - 2,);
            add_stepif(x + 1, y - 2,);
            add_stepif(x - 2, y - 1,);
            add_stepif(x + 2, y - 1,);
            add_stepif(x - 2, y + 1,);
            add_stepif(x + 2, y + 1,);
            add_stepif(x - 1, y + 2,);
            add_stepif(x + 1, y + 2,);
            return steps;
        case figureType::Pawn:
            steps.reserve(4);
            {
            int step = (f.getColor()?-1:1);
            int ay = y-step;
            add_stepifa(x-1, ay);
            add_stepifna(x, ay, if ((y==6 && !f.getColor()) || (y==1 && f.getColor())) {
                add_stepifna(x, y-step-step,);
            });
            add_stepifa(x+1, ay);
            }
            return steps;
        case figureType::Rook:
            steps.reserve(10);
            add_straight
            return steps;
        case figureType::Bishop:
            steps.reserve(11);
            add_diagonal
            return steps;
        case figureType::Queen:
            steps.reserve(21);
            add_straight
            add_diagonal
            return steps;
        }
        return steps;
    }
    void draw(sf::RenderWindow& window, int x, int y, int size) {
        this->x=x;
        this->y=y;
        this->size=size;
        std::vector<bool> deleted(animations.size());
        std::vector<std::pair<sf::Vector2i, sf::Vector2f>> positions(animations.size());
        for (int i = 0; i!=animations.size(); i++) {
        // for (auto& anim :animations) {
            auto& anim = animations[i];
            if (anim.is_end()) {
                deleted[i] = true;
            } else {
                positions[i].first = anim.end_pos;
                positions[i].second = anim.get_current_pos();
            }
        }
        for (int i = animations.size() - 1; i >= 0; i--) {
            if (deleted[i]) {
                animations.erase(animations.begin() + i);
                positions.erase(positions.begin() + i);
            }
        }
        std::vector<std::pair<uint8_t, uint8_t>, LinearPoolAllocator<std::pair<uint8_t, uint8_t>>> steps;
        if (select_ceil.first!=255) {
            steps = get_steps(select_ceil.first, select_ceil.second);
        }
        float size_ceil = size*0.125f;
        float ry;
        if (ccolor || no_rotate_screen) ry = y+size-size_ceil;
        else ry = y;
        for (int cy = 0; cy!=8; cy++) {
            float rx = x;
            if (ccolor || no_rotate_screen) {
                rx = x+size-size_ceil;
            }
            for (int cx = 0; cx!=8; cx++) {
                sf::Color ceil_color = (((cx + cy) & 1) ? sf::Color(115, 80, 56) : sf::Color(220, 203, 184));
                sf::RectangleShape rect;
                sf::Color colOutLine = sf::Color::White;
                if (select_ceil.first==cx && select_ceil.second==cy) {
                    colOutLine = sf::Color::Blue;
                } else if (std::find(steps.begin(), steps.end(), std::pair<uint8_t, uint8_t>{cx, cy}) != steps.end()) {
                    bool can = true;
                    auto d = take_a_step(select_ceil, {cx, cy});
                    if (check_pic(ccolor)) {
                        can=false;
                    }
                    untake_a_step(d);
                    if (can) {
                        colOutLine = sf::Color::Green;
                        if (board(cx, cy).type!=figureType::Empty && board(cx, cy).getColor()!=ccolor) {
                            colOutLine = sf::Color::Red;
                        }
                    }
                }

                if (colOutLine!=sf::Color::White) {
                    rect.setSize(sf::Vector2f(size_ceil, size_ceil));
                    rect.setPosition(sf::Vector2f(rx, ry));
                    rect.setFillColor(colOutLine);
                    window.draw(rect);

                    rect.setSize(sf::Vector2f(size_ceil*0.9, size_ceil*0.9));
                    rect.setPosition(sf::Vector2f(rx+size_ceil*0.05, ry+size_ceil*0.05));
                    rect.setFillColor(ceil_color);
                } else {
                    rect.setSize(sf::Vector2f(size_ceil, size_ceil));
                    rect.setPosition(sf::Vector2f(rx, ry));
                    rect.setFillColor(ceil_color);
                }
                window.draw(rect);
                figure f = board(cx, cy);
                if (f.type==figureType::Empty) {
                    if (!ccolor && !no_rotate_screen) {
                        rx += size_ceil;
                    } else {
                        rx -= size_ceil;
                    }
                    continue;
                }
                bool found = false;
                for (auto [ceil, pos]:positions) {
                    if (ceil==sf::Vector2i(cx, cy)) {
                        found=true;
                    }
                }
                if (!found) {
                    sf::RectangleShape figureR;
                    figureR.setTexture(Textures.getTexture(f.type));
                    figureR.setSize(sf::Vector2f(size_ceil, size_ceil));
                    figureR.setPosition(sf::Vector2f(rx, ry));
                    figureR.setFillColor(f.getColor()?sf::Color::White:sf::Color::Black);
                    window.draw(figureR);
                }   
                if (!ccolor && !no_rotate_screen) {
                    rx+=size_ceil;
                } else {
                    rx-=size_ceil;
                }            
            }   
            if (!ccolor && !no_rotate_screen) {
                ry += size_ceil;
            } else {
                ry -= size_ceil;
            }
        }
        for (auto [ceil, pos]: positions) {
            figure f = board(ceil.x, ceil.y);
            sf::RectangleShape figureR;
            figureR.setTexture(Textures.getTexture(f.type));
            figureR.setSize(sf::Vector2f(size_ceil, size_ceil));
            sf::Vector2f pos_ceil = pos*size_ceil;
            if (ccolor || no_rotate_screen) {
                pos_ceil.y = size-pos_ceil.y-size_ceil;
                pos_ceil.x = size-pos_ceil.x-size_ceil;
            }
            figureR.setPosition(sf::Vector2f(x, y)+pos_ceil);
            figureR.setFillColor(f.getColor()?sf::Color::White:sf::Color::Black);
            window.draw(figureR);
        }
        // победный текст
        if (victory_type!=0) {
            sf::Text text;
            text.setFont(defaultFont);
            std::wstring txt = L"Ничья";
            if (victory_type==1) txt=L"Победа белых";
            if (victory_type==3) txt=L"Победа чёрных";
            text.setString(txt);
            text.setFillColor(sf::Color::Green);
            if (victory_type==2) text.setFillColor(sf::Color::Yellow);
            text.setOutlineColor(sf::Color(text.getFillColor().r*0.5f, text.getFillColor().g*0.5f, text.getFillColor().b*0.5f));

            unsigned int startSize = 100;
            text.setCharacterSize(startSize);

            sf::FloatRect textRect = text.getLocalBounds();

            if (textRect.width > 0 && textRect.height > 0) {
                float scaleX = size*0.8 / textRect.width;
                float scaleY = size*0.8 / textRect.height;
                
                float finalScale = std::min(scaleX, scaleY);
                
                unsigned int finalCharSize = static_cast<unsigned int>(startSize * finalScale);
                text.setOutlineThickness(finalCharSize*0.05);
                
                if (finalCharSize < 1) finalCharSize = 1;
                
                text.setCharacterSize(finalCharSize);
                
                textRect = text.getLocalBounds();
                
                float posX = x + (size - textRect.width) / 2.0f - textRect.left;
                float posY = y + (size - textRect.height) / 2.0f - textRect.top;
                
                text.setPosition(posX, posY);
            }
            window.draw(text);
        }
        if (analytics) {
            (*analytics)->update();
            (*analytics)->render();
            if (animations.empty()) {
                int gx = (*analytics)->getSelectedX();
                if (history_pos>gx) {
                    control_z();
                } else if (history_pos<gx) {
                    control_shift_z();
                }
            }
            if (!(*analytics)->isOpen()) {
                analytics.reset();
            }
        }
    }
    std::pair<std::pair<figure, figure>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> take_a_step(std::pair<uint8_t, uint8_t> pos, std::pair<uint8_t, uint8_t> step) {
        figure af = board(step.first, step.second);
        figure mf = board(pos.first, pos.second);
        board(step.first, step.second) = mf;
        board(pos.first, pos.second).type = figureType::Empty;
        return {{mf, af}, {pos, step}};
    }
    void take_a_step(std::pair<std::pair<figure, std::pair<figure, figure>>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> data) {
        board(data.second.second.first, data.second.second.second) = data.first.second.second;
        board(data.second.first.first, data.second.first.second).type = figureType::Empty;
    }
    void take_a_step_anim(std::pair<std::pair<figure, std::pair<figure, figure>>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> data) {
        board(data.second.second.first, data.second.second.second) = data.first.second.second;
        board(data.second.first.first, data.second.first.second).type = figureType::Empty;
        animations.emplace_back(0.3, data.second.first, data.second.second);
    }
    void untake_a_step(std::pair<std::pair<figure, figure>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> data) {
        board(data.second.first.first, data.second.first.second) = data.first.first; // перемещяем фигуру которая передвигалась
        board(data.second.second.first, data.second.second.second) = data.first.second; // создаём съединую фигуру/пустое поле
    }
    void untake_a_step(std::pair<std::pair<figure, std::pair<figure, figure>>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> data) {
        board(data.second.first.first, data.second.first.second) = data.first.first; // перемещяем фигуру которая передвигалась
        board(data.second.second.first, data.second.second.second) = data.first.second.first; // создаём съединую фигуру/пустое поле
    }
    void untake_a_step_anim(std::pair<std::pair<figure, std::pair<figure, figure>>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> data) {
        board(data.second.first.first, data.second.first.second) = data.first.first; // перемещяем фигуру которая передвигалась
        board(data.second.second.first, data.second.second.second) = data.first.second.first; // создаём съединую фигуру/пустое поле
        animations.emplace_back(0.3, data.second.second, data.second.first);
    }
    int check_end(bool color) { // 0 - нет. 1 - мат. 2 - ничья
        bool need_check = false;
        for (int cy = 0; cy!=8; cy++) {
            for (int cx = 0; cx!=8; cx++) {
                figure f = board(cx, cy);
                if (f.type!=figureType::Empty && f.getColor()==color) {
                    auto steps = get_steps(cx, cy);
                    for (auto step: steps) {
                        figure fa = board(step.first, step.second);
                        if (fa.type==figureType::King) {
                            need_check = true;
                            goto end_for;
                        }
                    }
                }
            }
        }
        end_for:
        bool dnmadnc = false;
        for (int cy = 0; cy!=8; cy++) {
            for (int cx = 0; cx!=8; cx++) {
                figure f = board(cx, cy);
                if (f.type!=figureType::Empty && f.getColor()==!color) {
                    auto steps = get_steps(cx, cy);
                    for (auto step: steps) {
                        auto data = take_a_step({cx, cy}, step);
                        bool diw = true;
                        for (int cy2 = 0; cy2!=8; cy2++) {
                            for (int cx2 = 0; cx2!=8; cx2++) {
                                figure f = board(cx2, cy2);
                                if (f.type!=figureType::Empty && f.getColor()==color) {
                                    auto steps2 = get_steps(cx2, cy2);
                                    for (auto step2: steps2) {
                                        figure fa = board(step2.first, step2.second);
                                        if (fa.type==figureType::King && fa.getColor()==!color) {
                                            diw = false;
                                            goto end_for2;
                                        }
                                    }
                                }
                            }
                        }
                        end_for2:
                        untake_a_step(data);
                        if (diw) {
                            return 0;
                        }
                    }
                }
            }
        }
        if (need_check) {
            return 1;
        } else {
            return 2;
        }
    }
    bool check_pic(bool ccolor) {
        for (int cy = 0; cy!=8; cy++) {
            for (int cx = 0; cx!=8; cx++) {
                if (board(cx, cy).getColor()!=ccolor) {
                    for (auto step: get_steps(cx, cy)) {
                        figure f = board(step.first, step.second);
                        if (f.type==figureType::King && f.getColor()==ccolor) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
    bool check_dead(bool color) {
        for (int cy = 0; cy!=8; cy++) {
            for (int cx = 0; cx!=8; cx++) {
                figure f = board(cx, cy);
                if (f.type==figureType::King && f.getColor()==color) {
                    return false;
                }
            }
        }
        return true;
    }
    int calc_score(bool color) {
        int score = 0;
        for (int cy = 0; cy!=8; cy++) {
            for (int cx = 0; cx!=8; cx++) {
                figure f = board(cx, cy);
                if (f.type==figureType::King) continue;
                score += f.type.value;
            }
        }
        if (!color) {
            score = -score;
        }
        return score;
    }
    #define parallel_if(ysl, copy_path, st, ed, name, ...) if (ysl) [[unlikely]] {\
        _Pragma("omp parallel for")\
        for (int pi = 0; pi!=omp_get_max_threads(); pi++) {\
            copy_path\
            for (int name = (ed-st)/omp_get_max_threads()*pi; name!=(ed-st)/omp_get_max_threads()*(pi+1);name++) {\
                __VA_ARGS__\
            }\
        }\
    } else {\
        for (int name = st; name!=ed; name++) {\
            __VA_ARGS__\
        }\
    }
    std::pair<int, std::pair<std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>, figure>> make_move(bool color, int depth = 8, int alpha = -1000000, int beta = 1000000, int score_cache=0) {
        int best_score = -1000000;
        std::pair<std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>, figure> best_step;
        // parallel_if(depth==6, Board board_copy = *this;, 0, 8, cy, 
        for (int cy = 0; cy != 8; cy++) {
            for (int cx = 0; cx != 8; cx++) {
                figure f = board(cx, cy);
                if (f.type != figureType::Empty && f.getColor() == color) { // (исправлен цвет)
                    auto steps = get_steps(cx, cy);
                    for (auto step : steps) {
                        int score = 0;
                        int new_score_cache = score_cache;
                        bool kill_king = board(step.first, step.second).type == figureType::King;
                        if (!kill_king) {
                            new_score_cache += abs(board(step.first, step.second).type.value);
                        }
                        
                        auto data = take_a_step({cx, cy}, step);
                        bool change_to_pawn = false;
                        if (f.type==figureType::Pawn && ((step.second==7 && f.getColor()) || (step.second==0 && !f.getColor()))) {
                            change_to_pawn = true;
                        }
                        for (int i = 0; i!=(change_to_pawn?4:1); i++) {
                            if (change_to_pawn) {
                                board(step.first, step.second).setType(ftcf[i]);
                                new_score_cache += ftcf[i].value-1;
                            }
                            if (kill_king) {
                                // score = 900000 + depth*100 + calc_score(color);
                                score = 900000 + depth*100 + new_score_cache;
                            } else {
                                if (depth == 1) {
                                    //score = calc_score(color);
                                    score = score_cache;
                                } else {
                                    score = -make_move(!color, depth - 1, -beta, -alpha, -new_score_cache).first;
                                }
                            }
                            if (score > best_score) {
                                best_score = score;
                                best_step = {{{cx, cy}, step}, board(step.first, step.second)};
                            }
                            
                            if (best_score > alpha) {
                                alpha = best_score;
                            }
                            
                            if (alpha >= beta) {
                                untake_a_step(data);
                                return {best_score, best_step}; 
                            }
                        }
                        untake_a_step(data);
                    }
                }   
            }        
        }
        return {best_score, best_step};
    }
    int calc_score_step(std::pair<std::pair<figure, std::pair<figure, figure>>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> data_step, int depth=6) {
        int score = 0;
        figure f = board(data_step.second.first.first, data_step.second.first.second);
        bool kill_king = board(data_step.second.second.first, data_step.second.second.second).type == figureType::King;
        
        take_a_step(data_step);
        bool change_to_pawn = false;
        if (kill_king) {
            score = 900000 - depth*100 + calc_score(ccolor);
        } else {
            score = -make_move(!ccolor, depth - 1, 1000000, -1000000).first;
        }
        untake_a_step(data_step);
        return score;
    }
    void giveEvent(sf::RenderWindow& window, sf::Event event) {
        if (event.type==sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            int x_click = event.mouseButton.x;
            int y_click = event.mouseButton.y;
            if (x_click>=x && y_click>=y && x_click<=x+size && y_click<=y+size) {
                x_click-=x;
                y_click-=y;

                x_click/=(size*0.125);
                y_click/=(size*0.125);
                if (ccolor || no_rotate_screen) {
                    y_click=7-y_click;
                    x_click=7-x_click;
                }
                if (animations.empty() && victory_type==0 && !bot_thinking) {
                    if (select_ceil.first==255) {
                        if (board(x_click, y_click).type!=figureType::Empty && board(x_click, y_click).getColor()==ccolor) {
                            select_ceil.first = x_click;
                            select_ceil.second = y_click;
                        }
                    } else {
                        auto steps = get_steps(select_ceil.first, select_ceil.second);
                        bool found = false;
                        for (auto step: steps) {
                            if (step.first==x_click && step.second==y_click) {
                                auto d = take_a_step(select_ceil, step);
                                if (check_pic(ccolor)) {
                                    untake_a_step(d);
                                    break;
                                }
                                untake_a_step(d);
                                figure f = board(select_ceil.first, select_ceil.second);
                                std::pair<std::pair<figure, std::pair<figure, figure>>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> data = {{f, {board(step.first, step.second), f}}, {select_ceil, step}};
                                if (f.type==figureType::Pawn && ((step.second==7 && f.getColor()) || (step.second==0 && !f.getColor()))) {
                                    int select = showPromotionWindow(window, {
                                        {Textures.getTexture(figureType::Queen), L"ферзь"},
                                        {Textures.getTexture(figureType::Knight), L"конь"},
                                        {Textures.getTexture(figureType::Rook), L"ладья"},
                                        {Textures.getTexture(figureType::Bishop), L"слон"},
                                    });
                                    if (select==0) f.type=figureType::Queen;
                                    else if (select==1) f.type=figureType::Knight;
                                    else if (select==2) f.type=figureType::Rook;
                                    else if (select==3) f.type=figureType::Bishop;
                                    else window.close();
                                }
                                if (history.size()!=history_pos) {
                                    history.resize(history_pos);
                                    int add_for_white = 0;
                                    if ((history_pos&1)==1) add_for_white++;
                                    graph_score_white.values.resize(history_pos);
                                    graph_score_black.values.resize(history_pos);
                                    graph_score_step_white.values.resize(history_pos/2+add_for_white);
                                    graph_score_step_black.values.resize(history_pos/2);
                                    graph_max_score_step_white.values.resize(history_pos/2+add_for_white);
                                    graph_max_score_step_black.values.resize(history_pos/2);
                                }
                                graph_score_white.values.push_back(calc_score(true));
                                graph_score_black.values.push_back(calc_score(false));
                                int score_me = calc_score_step(data);
                                if (abs(score_me)>8000) {
                                    if (score_me==abs(score_me)) {
                                        score_me = 40;
                                    } else {
                                        score_me = -40;
                                    }
                                }
                                if (ccolor) {
                                    graph_score_step_white.values.push_back(score_me);
                                } else {
                                    graph_score_step_black.values.push_back(score_me);
                                }
                                auto [score, bot_step] = make_move(ccolor, 5);
                                if (abs(score)>8000) {
                                    if (score==abs(score)) {
                                        score = 40;
                                    } else {
                                        score = -40;
                                    }
                                }
                                if (ccolor) {
                                    graph_max_score_step_white.values.push_back(score);
                                } else {
                                    graph_max_score_step_black.values.push_back(score);
                                }
                                board(select_ceil.first, select_ceil.second).type = figureType::Empty;
                                data.first.second.second = f;
                                board(step.first, step.second) = f;
                                animations.emplace_back(0.3, select_ceil, step);
                                history.insert(history.begin()+history_pos, data);
                                history_pos++;

                                // сбрасываем выделение
                                select_ceil.first = 255;
                                select_ceil.second = 255;

                                int end_check_black = check_end(ccolor);
                                int end_check_white = check_end(!ccolor);
                                if (!ccolor) {
                                    std::swap(end_check_black, end_check_white);
                                }
                                if (end_check_black==1) {
                                    victory_type = 1;
                                    ccolor = !ccolor;
                                    return;
                                } else if (end_check_white==1) {
                                    victory_type = 3;
                                    ccolor = !ccolor;
                                    return;
                                } else if (end_check_black==2) {
                                    victory_type = 2;
                                    ccolor = !ccolor;
                                    return;
                                }
                                if (!bot) {
                                    ccolor = !ccolor;
                                }
                                found = true;
                                

                                if (bot) {
                                    bot_thinking = true;
                                    std::thread th([&]() {
                                        Board board_copy = *this; 
                                        auto [score, bot_step] = board_copy.make_move(!ccolor, bot_depth);
                                        std::cout << "score:" << score << std::endl;
                                        std::pair<std::pair<figure, std::pair<figure, figure>>, std::pair<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>>> bot_data = {{board(bot_step.first.first.first, bot_step.first.first.second), {board(bot_step.first.second.first, bot_step.first.second.second), bot_step.second}}, {bot_step.first.first, bot_step.first.second}};
                                        board(bot_step.first.second.first, bot_step.first.second.second) = bot_step.second;
                                        board(bot_step.first.first.first, bot_step.first.first.second).type = figureType::Empty;
                                        animations.emplace_back(0.3, bot_step.first.first, bot_step.first.second);
                                        // записываем в историю
                                        history.insert(history.begin()+history_pos, bot_data);
                                        history_pos++;
                                        if (!ccolor) {
                                            if (abs(score)>8000) {
                                                if (score==abs(score)) {
                                                    score = 40;
                                                } else {
                                                    score = -40;
                                                }
                                            }
                                            graph_score_step_white.values.push_back(score);
                                            graph_max_score_step_white.values.push_back(score);
                                        } else {
                                            graph_score_step_black.values.push_back(score);
                                            graph_max_score_step_black.values.push_back(score);
                                        }
                                        // проверяем на победы
                                        int end_check_black = check_end(ccolor);
                                        int end_check_white = check_end(!ccolor);
                                        if (ccolor) {
                                            std::swap(end_check_black, end_check_white);
                                        }
                                        if (end_check_black==1) {
                                            victory_type = 1;
                                            ccolor = !ccolor;
                                            return;
                                        } else if (end_check_white==1) {
                                            victory_type = 3;
                                            ccolor = !ccolor;
                                            return;
                                        } else if (end_check_black==2) {
                                            victory_type = 2;
                                            ccolor = !ccolor;
                                            return;
                                        }
                                        bot_thinking = false;
                                    });
                                    th.detach();
                                }
                                break;
                            }
                        }
                        if (!found) {
                            if (board(x_click, y_click).type!=figureType::Empty && board(x_click, y_click).getColor()==ccolor) {
                                select_ceil.first = x_click;
                                select_ceil.second = y_click;
                            } else {
                                select_ceil.first = 255;
                                select_ceil.second = 255;
                            }
                        }
                    }
                }
            }
        } else if (event.type==sf::Event::KeyPressed) {
            if (event.key.control && event.key.code==sf::Keyboard::Z && animations.empty()) {
                if (!event.key.shift) {
                    control_z();
                } else {
                    control_shift_z();
                }
            } else if (event.key.code==sf::Keyboard::A && victory_type!=0) {
                if (!analytics) {
                    analytics = std::make_shared<std::optional<ChessAnalyticsWindow>>(
                        std::vector<Graph>{
                            graph_score_white, 
                            graph_score_black, 
                            graph_score_step_white, 
                            graph_score_step_black, 
                            graph_max_score_step_white, 
                            graph_max_score_step_black
                        }
                    );
                } else if (event.key.alt) {
                    if (hint) {
                        hint = std::nullopt;
                    } else {
                        make_move(ccolor);
                    }
                }
            } else if (event.key.code == sf::Keyboard::D) {
                int depth = getNumber("select depth bot", "select depth (current depth "+std::to_string(bot_depth)+")", window);
                if (depth>0) {
                    bot_depth = depth;
                }
            }
        }
    }
    void control_z() {
        select_ceil.first = 255;
        select_ceil.second = 255;
        int num_steps = -1;
        if (bot && !analytics) num_steps = -2;
        for (int i = 0; i!=abs(num_steps); i++) {
            history_pos--;
            if(history_pos==-1) {
                history_pos++;
                return;
            }
            untake_a_step_anim(history[history_pos]);
            ccolor=!ccolor;
        }
    }
    void control_shift_z() {
        select_ceil.first = 255;
        select_ceil.second = 255;
        int num_steps = -1;
        if (bot && !analytics) num_steps = -2;
        for (int i = 0; i!=abs(num_steps); i++) {
            if(history_pos==history.size()) {
                return;
            }
            take_a_step_anim(history[history_pos]);
            history_pos++;
            ccolor = !ccolor;
        }
    }
    void loadFromFile(std::wstring filename) {
        std::ifstream file(filename.c_str());
        if (!file.is_open()) {
            std::cerr << "error open file!\n";
            return;
        }
        for (int cy = 0; cy!=8; cy++) {
            for (int cx = 0; cx!=8; cx++) {
                board(cx, cy).type = figureType::Empty;
            }
        }
        history.clear();
        graph_score_white.values.clear();
        graph_score_black.values.clear();
        graph_score_step_white.values.clear();
        graph_score_step_black.values.clear();
        graph_max_score_step_white.values.clear();
        graph_max_score_step_black.values.clear();
        victory_type = 0;
        select_ceil.first = 255;
        select_ceil.second = 255;

        std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
        file.close();
        std::vector<std::string> words = splitBySpaces(content);
        for (int i = 0; i!=words.size()-1; i+=5) {
            // name
            figureType type;
            if (words[i]=="король") type=figureType::King;
            else if (words[i]=="пешка") type=figureType::Pawn;
            else if (words[i]=="ферзь") type=figureType::Queen;
            else if (words[i]=="лодья") type=figureType::Rook;
            else if (words[i]=="слон") type=figureType::Bishop;
            else if (words[i]=="конь") type=figureType::Knight;
            else {
                std::cerr << "errors in file. got:" << words[i] << std::endl;
                return;
            }
            // pos
            int x = std::stoi(words[i+1])-1;
            int y = std::stoi(words[i+2])-1;
            // color
            bool color = words[i+4]=="True";
            board(x, y).setType(type);
            board(x, y).setColor(color);
        }
        ccolor = words[words.size()-1]=="True";
    }
    void saveInFile(std::wstring filename) {
        std::ofstream file(filename.c_str());
        if (!file.is_open()) {
            std::cerr << "error open writen file!\n";
            return;
        }
        std::string content;
        for (int cy = 0; cy!=8; cy++) {
            for (int cx = 0; cx!=8; cx++) {
                if (board(cx, cy).type!=figureType::Empty) {
                    figure f = board(cx, cy);
                    if (f.type==figureType::King) content += "король ";
                    else if (f.type==figureType::Queen) content += "ферзь ";
                    else if (f.type==figureType::Knight) content += "конь ";
                    else if (f.type==figureType::Pawn) content += "пешка ";
                    else if (f.type==figureType::Bishop) content += "слон ";
                    else if (f.type==figureType::Rook) content += "лодья ";
                    else {
                        std::cerr << "save error" << std::endl;
                    }
                    content += std::to_string(cx+1) + ".0 ";
                    content += std::to_string(cy+1) + ".0 ";
                    content += "0.0 ";
                    content += (f.getColor()?"True ": "False ");
                }
            }
        }
        content+=(ccolor?"True ": "False ");
        file.write(content.c_str(), content.size());
        file.close();
    }
};
std::string getString(const std::string& title, const std::wstring& content, sf::RenderWindow& mainWindow) {
    sf::VideoMode vidioMode(300, 200);
    sf::RenderWindow input_window(vidioMode, title);
    input_window.setFramerateLimit(30);

    sf::Text contentText;
    contentText.setString(content);
    contentText.setCharacterSize(std::min(20, (int)(600/content.length())));
    contentText.setFont(defaultFont);
    contentText.setPosition(sf::Vector2f(10.f, 10.f));
    contentText.setFillColor(sf::Color::Black);
    sf::Text inputText;
    inputText.setString("Input: ");
    inputText.setCharacterSize(20);
    inputText.setPosition(sf::Vector2f(5, 40));
    inputText.setFont(defaultFont);
    inputText.setFillColor(sf::Color::Black);
    std::string input_string = "";
    sf::Event event;
    while (input_window.isOpen() && mainWindow.isOpen()) {
        while (input_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                input_string = "";
                input_window.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    input_window.close();
                } else if (event.key.code == sf::Keyboard::BackSpace) {
                    if (!input_string.empty()) {
                        input_string.pop_back();
                    }
                }
            }
            else if (event.type == sf::Event::TextEntered) {
                uint32_t code = event.text.unicode;
                if (code >= 32 && code<256) {
                    input_string += (char)code;
                }
            }

            inputText.setString("Input: " + input_string);
        }
        while (mainWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                input_window.close();
                mainWindow.close();
            }
        }
        input_window.clear(sf::Color::White);
        input_window.draw(contentText);
        input_window.draw(inputText);
        input_window.display();
        //mainWindow.display();
    }
    mainWindow.setActive(true);
    return input_string;
}
std::optional<std::wstring> SaveFileDialog(
    const sf::RenderWindow& window,
    const wchar_t* filter = L"All Files (*.*)\0*.*\0",
    const std::wstring& defaultExt = L""
) {
    // Буфер на 32768 широких символов (wchar_t) для поддержки Юникода
    std::vector<wchar_t> filePath(32768, L'\0'); 
    
    HWND hwndOwner = static_cast<HWND>(window.getSystemHandle());
    
    OPENFILENAMEW ofn = {}; 
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFile = filePath.data(); 
    ofn.nMaxFile = static_cast<DWORD>(filePath.size());

    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    if (!defaultExt.empty()) {
        ofn.lpstrDefExt = defaultExt.c_str();
    }
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    if (GetSaveFileNameW(&ofn) == TRUE) { 
        return std::wstring(filePath.data()); 
    }
    return std::nullopt;
}
std::optional<std::wstring> OpenFileDialog(
    const sf::RenderWindow& window,
    const wchar_t* filter = L"All Files (*.*)\0*.*\0"
) {
    std::vector<wchar_t> filePath(32768, L'\0'); 
    HWND hwndOwner = static_cast<HWND>(window.getSystemHandle());
    
    OPENFILENAMEW ofn = {}; 
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFile = filePath.data(); 
    ofn.nMaxFile = static_cast<DWORD>(filePath.size());
    
    // ПРЯМАЯ ПЕРЕДАЧА УКАЗАТЕЛЯ: Windows сама прочитает двойной нуль в конце
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_HIDEREADONLY;
    
    if (GetOpenFileNameW(&ofn) == TRUE) { 
        return std::wstring(filePath.data()); 
    }
    
    return std::nullopt;
}
int main() {
    // try {
        defaultFont.loadFromFile("C:/Windows/Fonts/arial.ttf");
        float width = 600;
        float height = 700;
        sf::VideoMode vidioMode(600, 700);
        sf::View view(sf::FloatRect(0.f, 0.f, 600.f, 700.f));
        sf::RenderWindow window(vidioMode, "Chess 2.0");
        window.setFramerateLimit(30);
        Board cBoard;
        cBoard.loadFromFile(L"./start.txt");
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                cBoard.giveEvent(window, event);
                if (event.type == sf::Event::Closed) {
                    window.close();
                } else if (event.type == sf::Event::Resized) {
                    sf::FloatRect visibleArea(0.f, 0.f, event.size.width, event.size.height);
                    width = event.size.width;
                    height = event.size.height;
                    window.setView(sf::View(visibleArea));
                } else if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::B) {
                        cBoard.bot = !cBoard.bot;
                    } else if (event.key.code == sf::Keyboard::R && cBoard.history_pos!=cBoard.history.size()) {
                        cBoard.victory_type = 0;
                    } else if (event.key.code == sf::Keyboard::S) {
                        cBoard.no_rotate_screen = !cBoard.no_rotate_screen;
                    }
                } else if (event.type==sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && (event.mouseButton.y>std::min(width, height) || event.mouseButton.x>std::min(width, height))) {
                    int button;
                    if (height>width) button = ((float)event.mouseButton.x)/width*3;
                    else button = ((float)event.mouseButton.y)/width*3;
                    if (button==0) { // restart
                        cBoard.loadFromFile(L"./start.txt");
                    } else if (button==1) { // open
                        std::optional<std::wstring> filename = OpenFileDialog(window,  L"Text Files (*.txt)\0*.txt\0"
              L"Binary Files (*.bin)\0*.bin\0"
              L"All Files (*.*)\0*.*\0\0");
                        if (filename) {
                            cBoard.loadFromFile(*filename);
                        }
                    } else if (button==2) { // save
                        std::optional<std::wstring> filename = SaveFileDialog(window,  L"Text Files (*.txt)\0*.txt\0"
                                                                    L"Binary Files (*.bin)\0*.bin\0"
                                                                    L"All Files (*.*)\0*.*\0", L"txt");
                        if (filename) {
                            cBoard.saveInFile(*filename);
                        }
                    }
                }
            }
            window.clear(sf::Color::Black);
            cBoard.draw(window, 0, 0, std::min(width, height));
            sf::RectangleShape button;
            sf::Text text;
            text.setFont(defaultFont);
            if (height>width) {
                button.setSize(sf::Vector2f(width*0.3333333333, std::max(width, height)-std::min(width, height)));
            } else {
                button.setSize(sf::Vector2f(width-height, height*0.3333333333));
            }
            // restart
            if (height>width) button.setPosition(sf::Vector2f(0, width));
            else button.setPosition(sf::Vector2f(height, 0));
            button.setFillColor(sf::Color(200, 200, 20));
            window.draw(button);
            text.setString("restart");
            text.setCharacterSize(100);
            {
            float k = text.getLocalBounds().width/100.0;
            text.setCharacterSize(std::min(button.getSize().y, button.getSize().x/k));
            }
            text.setFillColor(sf::Color(40, 40, 4));
            text.setPosition(button.getPosition()+sf::Vector2f((button.getSize().x-text.getLocalBounds().width)*0.5f, 0.0));
            window.draw(text);

            // open
            if (height>width) button.setPosition(sf::Vector2f(width*0.33333, width));
            else button.setPosition(sf::Vector2f(height, height*0.33333));
            button.setFillColor(sf::Color(20, 200, 20));
            window.draw(button);
            text.setString("open");
            text.setCharacterSize(100);
            {
            float k = text.getLocalBounds().width/100.0;
            text.setCharacterSize(std::min(button.getSize().y, button.getSize().x/k));
            }
            text.setFillColor(sf::Color(4, 40, 4));
            text.setPosition(button.getPosition()+sf::Vector2f((button.getSize().x-text.getLocalBounds().width)*0.5f, 0.0));
            window.draw(text);

            // save
            if (height>width) button.setPosition(sf::Vector2f(width*0.666666, width));
            else button.setPosition(sf::Vector2f(height, height*0.666666));
            button.setFillColor(sf::Color(5, 40, 200));
            window.draw(button);
            text.setString("save");
            text.setCharacterSize(100);
            {
            float k = text.getLocalBounds().width/100.0;
            text.setCharacterSize(std::min(button.getSize().y, button.getSize().x/k));
            }
            text.setFillColor(sf::Color(1, 8, 40));
            text.setPosition(button.getPosition()+sf::Vector2f((button.getSize().x-text.getLocalBounds().width)*0.5f, 0.0));
            window.draw(text);

            window.display();
        }
    // } catch (const std::bad_alloc& e) {
    //     std::cerr << "Error message: " << e.what() << std::endl;
    //     std::cin.get(); 
    //     return -1;
    // }
}
