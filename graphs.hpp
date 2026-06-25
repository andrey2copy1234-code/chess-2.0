#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <sstream>

struct Graph {
    std::string name;
    std::vector<float> values; // Значения по оси Y (например, оценка в пешках: +1.5, -2.3)
    sf::Color color;
    bool visible = true;
};

class ChessAnalyticsWindow {
private:
    sf::RenderWindow window;
    std::vector<Graph> graphs;
    sf::Font font;
    
    // Настройки интерфейса
    float padding = 60.f;
    int selectedX = -1; // Текущий ход под курсором мыши

    // Поиск глобального минимума и максимума для масштабирования Y
    std::pair<float, float> getMinMaxY() {
        float minY = -5.0f; // Базовые границы (±5 пешек), если данных мало
        float maxY = 5.0f;
        bool hasData = false;

        for (const auto& graph : graphs) {
            if (!graph.visible || graph.values.empty()) continue;
            for (float val : graph.values) {
                if (!hasData) {
                    minY = maxY = val;
                    hasData = true;
                } else {
                    if (val < minY) minY = val;
                    if (val > maxY) maxY = val;
                }
            }
        }
        // Небольшой запас сверху и снизу
        minY -= 0.5f;
        maxY += 0.5f;
        return {minY, maxY};
    }

    // Поиск максимального количества ходов для масштабирования X
    size_t getMaxXSize() {
        size_t maxSize = 0;
        for (const auto& graph : graphs) {
            if (graph.visible) {
                maxSize = std::max(maxSize, graph.values.size());
            }
        }
        return maxSize == 0 ? 1 : maxSize;
    }

public:
    // Конструктор инициализирует окно и графики
    ChessAnalyticsWindow(const std::vector<Graph>& inputGraphs) : graphs(inputGraphs) {
        window.create(sf::VideoMode(1000, 600), "Chess Analytics Dashboard");
        window.setFramerateLimit(60);
        
        // Загрузка стандартного шрифта (укажите ваш путь к шрифту .ttf)
        if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
            // Если шрифта нет, текст просто не отобразится, но программа не упадет
        }
    }

    // Проверка, открыто ли окно
    bool isOpen() {
        return window.isOpen();
    }
    // Возвращает X (индекс хода), на который сейчас наведена мышь
    int getSelectedX() const {
        return selectedX;
    }

    // Метод для принудительного переключения X из вашей основной программы
    void setSelectedX(int x) {
        size_t maxMoves = getMaxXSize();
        if (x >= 0 && x < static_cast<int>(maxMoves)) {
            selectedX = x;
        }
    }

    // Включение/выключение графика по его индексу
    void toggleGraph(size_t index) {
        if (index < graphs.size()) {
            graphs[index].visible = !graphs[index].visible;
        }
    }

    // Обновление логики и обработка событий
    void update() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            // Управление видимостью через клавиши 1, 2, 3...
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    size_t index = event.key.code - sf::Keyboard::Num1;
                    toggleGraph(index);
                }
            }
        }

        // Логика отслеживания мыши
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2u winSize = window.getSize();
        
        float graphWidth = winSize.x - 2 * padding;
        size_t maxMoves = getMaxXSize();

        if (mousePos.x >= padding && mousePos.x <= winSize.x - padding &&
            mousePos.y >= padding && mousePos.y <= winSize.y - padding) {
            
            // Вычисляем, к какому ходу ближе всего курсор
            float relativeX = mousePos.x - padding;
            float stepX = graphWidth / (maxMoves > 1 ? maxMoves - 1 : 1);
            selectedX = std::round(relativeX / stepX);
            
            if (selectedX >= static_cast<int>(maxMoves)) selectedX = maxMoves - 1;
        }
    }

    // Отрисовка интерфейса
    void render() {
        window.clear(sf::Color(30, 30, 30)); // Темный фон

        sf::Vector2u winSize = window.getSize();
        float w = winSize.x;
        float h = winSize.y;
        
        auto [minY, maxY] = getMinMaxY();
        size_t maxMoves = getMaxXSize();

        float graphWidth = w - 2 * padding;
        float graphHeight = h - 2 * padding;

        // --- 1. Отрисовка осей и сетки ---
        sf::VertexArray axes(sf::Lines, 4);
        // Ось X (ноль оценки, если он попадает в диапазон)
        float zeroY = padding + graphHeight * (maxY / (maxY - minY));
        if (zeroY < padding) zeroY = padding;
        if (zeroY > h - padding) zeroY = h - padding;

        axes[0] = sf::Vertex(sf::Vector2f(padding, zeroY), sf::Color(100, 100, 100));
        axes[1] = sf::Vertex(sf::Vector2f(w - padding, zeroY), sf::Color(100, 100, 100));
        // Ось Y (левая граница)
        axes[2] = sf::Vertex(sf::Vector2f(padding, padding), sf::Color(100, 100, 100));
        axes[3] = sf::Vertex(sf::Vector2f(padding, h - padding), sf::Color(100, 100, 100));
        window.draw(axes);

        // --- 2. Отрисовка графиков ---
        float stepX = graphWidth / (maxMoves > 1 ? maxMoves - 1 : 1);
        float scaleY = graphHeight / (maxY - minY);

        for (const auto& graph : graphs) {
            if (!graph.visible || graph.values.empty()) continue;

            sf::VertexArray line(sf::LineStrip, graph.values.size());
            for (size_t i = 0; i < graph.values.size(); ++i) {
                float vx = padding + i * stepX;
                float vy = padding + (maxY - graph.values[i]) * scaleY;
                line[i] = sf::Vertex(sf::Vector2f(vx, vy), graph.color);
            }
            window.draw(line);
        }

        // --- 3. Линия под курсором мыши (Интерактивный маркер) ---
        if (selectedX >= 0 && selectedX < static_cast<int>(maxMoves)) {
            float markerX = padding + selectedX * stepX;
            sf::VertexArray verticalLine(sf::Lines, 2);
            verticalLine[0] = sf::Vertex(sf::Vector2f(markerX, padding), sf::Color(255, 255, 255, 100));
            verticalLine[1] = sf::Vertex(sf::Vector2f(markerX, h - padding), sf::Color(255, 255, 255, 100));
            window.draw(verticalLine);

            // Вывод информации в углу экрана
            sf::Text infoText;
            infoText.setFont(font);
            infoText.setCharacterSize(14);
            infoText.setFillColor(sf::Color::White);
            
            std::stringstream ss;
            ss << "Selected Move (X): " << selectedX + 1 << "\n";
            for (size_t i = 0; i < graphs.size(); ++i) {
                if (!graphs[i].visible) continue;
                ss << graphs[i].name << " [" << (i + 1) << "]: ";
                if (selectedX < static_cast<int>(graphs[i].values.size())) {
                    ss << std::fixed << std::setprecision(2) << graphs[i].values[selectedX];
                } else {
                    ss << "N/A";
                }
                ss << "\n";
            }
            
            infoText.setString(ss.str());
            infoText.setPosition(padding, 5.f);
            window.draw(infoText);
        }

        // Легенда (какие клавиши что отключают)
        sf::Text legendText;
        legendText.setFont(font);
        legendText.setCharacterSize(12);
        legendText.setFillColor(sf::Color(180, 180, 180));
        legendText.setString("Press 1-9 to toggle graphs. Move mouse to inspect values.");
        legendText.setPosition(padding, h - padding + 10.f);
        window.draw(legendText);

        window.display();
    }
};
