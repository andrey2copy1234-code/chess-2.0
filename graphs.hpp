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
    float scalex;
    bool visible = true;
};

class ChessAnalyticsWindow {
private:
    sf::RenderWindow window;
    std::vector<Graph> graphs;
    sf::Font font;
    sf::View mainView; // Камера для масштабирования и перемещения
    
    // Настройки интерфейса
    float padding = 60.f;
    int selectedX = -1; // Текущий ход под курсором мыши
    float zoomFactor = 1.0f; // Текущий уровень зума

    // Поиск глобального минимума и максимума для масштабирования Y
    std::pair<float, float> getMinMaxY() {
        float minY = -5.0f; 
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
        minY -= 0.5f;
        maxY += 0.5f;
        return {minY, maxY};
    }

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
    ChessAnalyticsWindow(const std::vector<Graph>& inputGraphs) : graphs(inputGraphs) {
        window.create(sf::VideoMode(800, 600), "Chess Analytics Dashboard");
        window.setFramerateLimit(60);
        
        // Инициализируем камеру размером с окно
        mainView = window.getDefaultView();

        if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
            // Шрифта нет — текст не отобразится
        }
    }

    bool isOpen() {
        return window.isOpen();
    }

    int getSelectedX() const {
        return selectedX;
    }

    void setSelectedX(int x) {
        size_t maxMoves = getMaxXSize();
        if (x >= 0 && x < static_cast<int>(maxMoves)) {
            selectedX = x;
        }
    }

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
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    size_t index = event.key.code - sf::Keyboard::Num1;
                    toggleGraph(index);
                }
                // Сброс зума по нажатию на колесико / клавишу R
                if (event.key.code == sf::Keyboard::R) {
                    mainView = window.getDefaultView();
                    zoomFactor = 1.0f;
                }
            }
            else if (event.type == sf::Event::Resized) {
                // Корректно обновляем камеру с сохранением пропорций зума
                sf::Vector2f center = mainView.getCenter();
                mainView.setSize(event.size.width * zoomFactor, event.size.height * zoomFactor);
                mainView.setCenter(center);
            }
            // --- Логика приближения/отдаления в точку курсора ---
            else if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    float zoomRatio = (event.mouseWheelScroll.delta > 0) ? 0.9f : 1.1f;
                    
                    // Переводим позицию мыши из пикселей в мировые координаты ДО зума
                    sf::Vector2i pixelPos(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                    sf::Vector2f beforeZoom = window.mapPixelToCoords(pixelPos, mainView);

                    // Применяем зум к камере
                    mainView.zoom(zoomRatio);
                    zoomFactor *= zoomRatio;

                    // Переводим ту же позицию мыши в мировые координаты ПОСЛЕ зума
                    sf::Vector2f afterZoom = window.mapPixelToCoords(pixelPos, mainView);

                    // Сдвигаем камеру на разницу, чтобы точка под курсором осталась на месте
                    mainView.move(beforeZoom - afterZoom);
                }
            }
        }

        // Переводим позицию мыши в координаты игрового мира (с учетом зума)
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(mousePixelPos, mainView);
        
        // Базовые размеры окна (не деформированные зумом)
        sf::Vector2u winSize = window.getSize();
        float graphWidth = static_cast<float>(winSize.x) - 2 * padding;
        size_t maxMoves = getMaxXSize();

        // Проверка границ теперь работает в мировых координатах
        if (mousePos.x >= padding && mousePos.x <= winSize.x - padding &&
            mousePos.y >= padding && mousePos.y <= winSize.y - padding) {
            
            float relativeX = mousePos.x - padding;
            float stepX = graphWidth / (maxMoves > 1 ? maxMoves - 1 : 1);
            selectedX = std::round(relativeX / stepX);
            
            if (selectedX >= static_cast<int>(maxMoves)) selectedX = maxMoves - 1;
            if (selectedX < 0) selectedX = 0;
        } else {
            selectedX = -1; // Курсор вне графика
        }
    }

    // Отрисовка интерфейса
    void render() {
        window.clear(sf::Color(30, 30, 30));

        sf::Vector2u winSize = window.getSize();
        float w = winSize.x;
        float h = winSize.y;
        
        auto [minY, maxY] = getMinMaxY();
        size_t maxMoves = getMaxXSize();

        float graphWidth = w - 2 * padding;
        float graphHeight = h - 2 * padding;

        // --- РЕНДЕРИНГ ГРАФИКОВ (с зумом) ---
        window.setView(mainView);

        // Ось X и Y
        sf::VertexArray axes(sf::Lines, 4);
        float zeroY = padding + graphHeight * (maxY / (maxY - minY));
        if (zeroY < padding) zeroY = padding;
        if (zeroY > h - padding) zeroY = h - padding;

        axes[0] = sf::Vertex(sf::Vector2f(padding, zeroY), sf::Color(100, 100, 100));
        axes[1] = sf::Vertex(sf::Vector2f(w - padding, zeroY), sf::Color(100, 100, 100));
        axes[2] = sf::Vertex(sf::Vector2f(padding, padding), sf::Color(100, 100, 100));
        axes[3] = sf::Vertex(sf::Vector2f(padding, h - padding), sf::Color(100, 100, 100));
        window.draw(axes);

        // Отрисовка графиков
        float stepX = graphWidth / (maxMoves > 1 ? maxMoves - 1 : 1);
        float scaleY = graphHeight / (maxY - minY);

        for (const auto& graph : graphs) {
            if (!graph.visible || graph.values.empty()) continue;

            sf::VertexArray line(sf::LineStrip, graph.values.size());
            for (size_t i = 0; i < graph.values.size(); ++i) {
                float vx = padding + i * stepX*graph.scalex;
                float vy = padding + (maxY - graph.values[i]) * scaleY;
                line[i] = sf::Vertex(sf::Vector2f(vx, vy), graph.color);
            }
            window.draw(line);
        }

        // Вертикальный маркер под курсором
        if (selectedX >= 0 && selectedX < static_cast<int>(maxMoves)) {
            float markerX = padding + selectedX * stepX;
            sf::VertexArray verticalLine(sf::Lines, 2);
            verticalLine[0] = sf::Vertex(sf::Vector2f(markerX, padding), sf::Color(255, 255, 255, 100));
            verticalLine[1] = sf::Vertex(sf::Vector2f(markerX, h - padding), sf::Color(255, 255, 255, 100));
            window.draw(verticalLine);
        }

        // --- РЕНДЕРИНГ ИНТЕРФЕЙСА (статичный UI без зума) ---
        window.setView(window.getDefaultView());

        if (selectedX >= 0 && selectedX < static_cast<int>(maxMoves)) {
            sf::Text infoText;
            infoText.setFont(font);
            infoText.setCharacterSize(14);
            infoText.setFillColor(sf::Color::White);
            
            std::stringstream ss;
            ss << "Selected Move (X): " << selectedX + 1 << "\n";
            for (size_t i = 0; i < graphs.size(); ++i) {
                if (!graphs[i].visible) continue;
                ss << graphs[i].name << " [" << (i + 1) << "]: ";
                if (selectedX/graphs[i].scalex < static_cast<int>(graphs[i].values.size())) {
                    ss << std::fixed << std::setprecision(2) << graphs[i].values[selectedX/graphs[i].scalex];
                } else {
                    ss << "N/A";
                }
                ss << "\n";
            }
            
            infoText.setString(ss.str());
            infoText.setPosition(padding, 5.f);
            window.draw(infoText);
        }

        sf::Text legendText;
        legendText.setFont(font);
        legendText.setCharacterSize(12);
        legendText.setFillColor(sf::Color(180, 180, 180));
        legendText.setString("Scroll to Zoom. Press 'R' to Reset View. Press 1-9 to toggle graphs.");
        legendText.setPosition(padding, h - padding + 10.f);
        window.draw(legendText);

        window.display();
    }
};
