/**
 *  GameOfLife - An implementation of game of life using xengine
 *  Copyright (C) 2021  Julian Zampiccoli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef GAMEOFLIFE_GAMEOFLIFE_HPP
#define GAMEOFLIFE_GAMEOFLIFE_HPP

#include <fstream>

#include "xengine.hpp"

#include "gamegrid.hpp"

using namespace xng;

class GameOfLife : public Application {
public:
    enum ShadeMode {
        SHADE_DEFAULT,
        SHADE_SCALE_NEIGHBOUR, // Scale the color intensity with the number of neighbours of the cell,
    };

    GameOfLife(int argc, char *argv[])
            : Application(argc, argv),
              shaderCompiler(DriverRegistry::load<SPIRVCompiler>("shaderc")),
              shaderDecompiler(DriverRegistry::load<SPIRVDecompiler>("spirv-cross")),
              ren2d(*renderDevice, *shaderCompiler, *shaderDecompiler),
              gridRenderer2d(*renderDevice, *shaderCompiler, *shaderDecompiler) {
        window->setTitle("Game Of Life");
    }

protected:
    void start() override {
        std::ifstream stream("asset/Roboto-Regular.ttf");
        fontDriver = DriverRegistry::load<FontDriver>("freetype");
        font = fontDriver->createFont(stream);
        textRenderer = std::make_unique<TextRenderer>(*font, ren2d);
    }

    void update(DeltaTime deltaTime) override {
        updateInput(deltaTime);

        if (tickAccum + deltaTime >= tickDuration && !blockTick) {
            tickAccum = 0;
            grid = grid.stepTime();
        } else {
            if (blockTick)
                tickAccum = 0;
            else
                tickAccum += deltaTime;
        }

        auto &target = window->getRenderTarget();

        ren2d.renderBegin(target);
        ren2d.renderPresent();

        drawGrid(target);
        drawCursor(target);
        drawGui(target, deltaTime);

        Application::update(deltaTime);
    }

private:
    Vec2f worldToScreen(Vec2i pos, Vec2f targetSize) {
        return (pos.convert<float>() - viewPos) * (cellSize + cellSpacing) * viewScale +
               (targetSize / 2).convert<float>();
    }

    Vec2i screenToWorld(Vec2f pos, Vec2f targetSize) {
        auto ret = ((pos - (targetSize / 2)) / viewScale / (cellSize + cellSpacing) + viewPos);
        return Vec2f(std::round(ret.x), std::round(ret.y)).convert<int>();
    }

    Vec2i getMousePosition() {
        return screenToWorld(window->getInput().getMouse().position.convert<float>()
                             - Vec2f(cellSize + cellSpacing, cellSize + cellSpacing) * viewScale / 2,
                             window->getRenderTarget().getDescription().size.convert<float>());
    }

    std::vector<Vec2i> getBrushInfluence(Vec2i position) {
        std::vector<Vec2i> ret;
        for (int x = position.x - (int) brushSize; x <= position.x + (int) brushSize; x++) {
            for (int y = position.y - (int) brushSize; y <= position.y + (int) brushSize; y++) {
                ret.emplace_back(Vec2i(x, y));
            }
        }
        return ret;
    }

    void drawTiles(Renderer2D &ren, RenderTarget &target, const std::vector<Vec2i> &positions, ColorRGBA color) {
        std::vector<std::pair<Vec2f, float>> offsets;
        offsets.reserve(positions.size());

        for (auto &t: positions) {
            auto screenPos = worldToScreen(t, target.getDescription().size.convert<float>());
            offsets.emplace_back(std::pair<Vec2f, float>(screenPos, 0));
        }

        auto size = cellSize * viewScale;
        ren.drawInstanced(offsets, {size, size}, color);
    }

    void drawTile(Renderer2D &ren, RenderTarget &target, Vec2i pos, ColorRGBA color, bool fillSpacing) {
        Vec2f screenPos = worldToScreen(pos, target.getDescription().size.convert<float>());

        auto size = cellSize * viewScale;

        if (fillSpacing) {
            screenPos -= Vec2f(cellSpacing, cellSpacing) / 2 * viewScale;
            size += cellSpacing * viewScale;
        }

        Rectf rect(screenPos, {size, size});

        ren.draw(rect, color);
    }

    void drawCursor(RenderTarget &target) {
        ren2d.renderBegin(target, false);

        auto mpos = getMousePosition();
        auto influence = getBrushInfluence(mpos);

        for (auto &pos: influence) {
            Vec2f screenPos = worldToScreen(pos, target.getDescription().size.convert<float>());

            auto size = cellSize * viewScale;

            screenPos += Vec2f(cellSize, cellSize) / 4 * viewScale;
            size /= 2;

            Rectf rect(screenPos, {size, size});

            ColorRGBA color;
            if (window->getInput().getKeyboard().getKey(xng::KEY_LCTRL))
                color = ColorRGBA::red();
            else
                color = ColorRGBA::green();
            ren2d.draw(rect, color);
        }

        ren2d.renderPresent();
    }

    void drawGrid(RenderTarget &target) {
        Vec2i min = screenToWorld({0, 0}, target.getDescription().size.convert<float>());
        Vec2i max = screenToWorld(target.getDescription().size.convert<float>(),
                                  target.getDescription().size.convert<float>());

        std::vector<Vec2i> positions;
        for (auto &x: grid.cells) {
            for (auto &y: x.second) {
                if (x.first < min.x
                    || x.first > max.x
                    || y < min.y
                    || y > max.y)
                    continue;
                positions.emplace_back(Vec2i(x.first, y));
            }
        }

        if (!positions.empty()) {
            gridRenderer2d.renderBegin(target, false);
            switch (mode) {
                case SHADE_SCALE_NEIGHBOUR:
                    for (auto &p: positions) {
                        auto n = grid.getNeighbours(p);
                        float scale = 0.2f;
                        if (n > 0)
                            scale = (float) n / 10.0f;
                        drawTile(gridRenderer2d, target, p, ColorRGBA::white(scale), true);
                    }
                    break;
                case SHADE_DEFAULT:
                default:
                    drawTiles(gridRenderer2d, target, positions, ColorRGBA::white());
                    break;
            }
            gridRenderer2d.renderPresent();
        }
    }

    void drawGui(RenderTarget &target, float deltaTime) {
        auto cellCount = 0;
        for (auto &x: grid.cells)
            for (auto &y: x.second)
                cellCount++;

        font->setPixelSize({0, 50});

        auto fps = 1 / deltaTime;

        auto deltaText = textRenderer->render(std::to_string(deltaTime) + " sec / " + std::to_string(fps) + " fps", 30);
        auto text = textRenderer->render("Alive cells: " + std::to_string(cellCount), 30);

        auto mpos = getMousePosition();
        auto mtext = textRenderer->render("Position: " + std::to_string(mpos.x) + " " + std::to_string(mpos.y)
                                          + "\nZoom: " + std::to_string(viewScale),
                                          30);
        auto btext = textRenderer->render("Game Paused", 30);

        auto ttext = textRenderer->render("Tick Duration: " + std::to_string(tickDuration) + "s", 30);

        ren2d.renderBegin(target, false);

        static const float padding = 10.0f;

        ren2d.draw(deltaText,
                   Rectf({padding, padding},
                         deltaText.getTexture().getDescription().size.convert<float>()),
                   ColorRGBA::white());
        ren2d.draw(mtext,
                   Rectf({padding, padding
                                   + deltaText.getTexture().getDescription().size.y
                                   + padding},
                         mtext.getTexture().getDescription().size.convert<float>()),
                   ColorRGBA::white());
        ren2d.draw(ttext,
                   Rectf({padding, padding
                                   + deltaText.getTexture().getDescription().size.y
                                   + padding
                                   + mtext.getTexture().getDescription().size.y
                                   + padding},
                         ttext.getTexture().getDescription().size.convert<float>()),
                   ColorRGBA::white());

        ren2d.draw(text,
                   Rectf({padding, padding
                                   + deltaText.getTexture().getDescription().size.y
                                   + padding
                                   + mtext.getTexture().getDescription().size.y
                                   + padding
                                   + ttext.getTexture().getDescription().size.y
                                   + padding},
                         text.getTexture().getDescription().size.convert<float>()),
                   ColorRGBA::white());

        if (blockTick) {
            ren2d.draw(btext,
                       Rectf({(target.getDescription().size.convert<float>() / 2.0f) -
                              (btext.getTexture().getDescription().size.convert<float>() / 2.0f)},
                             btext.getTexture().getDescription().size.convert<float>()),
                       ColorRGBA::white());
        }

        ren2d.renderPresent();
    }

    void updateInput(float deltaTime) {
        auto &input = window->getInput();
        auto &keyboard = input.getKeyboard();

        if (keyboard.getKey(KEY_LEFT)
            || keyboard.getKey(KEY_A)) {
            viewPos.x -= deltaTime * panSpeed;
        }
        if (keyboard.getKey(KEY_RIGHT)
            || keyboard.getKey(KEY_D)) {
            viewPos.x += deltaTime * panSpeed;
        }
        if (keyboard.getKey(KEY_UP)
            || keyboard.getKey(KEY_W)) {
            viewPos.y -= deltaTime * panSpeed;
        }
        if (keyboard.getKey(KEY_DOWN)
            || keyboard.getKey(KEY_S)) {
            viewPos.y += deltaTime * panSpeed;
        }

        if (keyboard.getKeyDown(KEY_SPACE)) {
            keyboardBlockToggle = !keyboardBlockToggle;
        }

        if (keyboard.getKeyDown(KEY_R)) {
            if (brushSize > 0)
                brushSize--;
        } else if (keyboard.getKeyDown(KEY_T)) {
            brushSize++;
        }

        if (keyboard.getKeyDown(KEY_1)) {
            mode = SHADE_DEFAULT;
        } else if (keyboard.getKeyDown(KEY_2)) {
            mode = SHADE_SCALE_NEIGHBOUR;
        }

        if (keyboard.getKey(KEY_Q))
            tickDuration -= 0.2f * deltaTime;
        else if (keyboard.getKey(KEY_E))
            tickDuration += 0.2f * deltaTime;

        if (tickDuration <= 0)
            tickDuration = 0.000001;
        else if (tickDuration > 5)
            tickDuration = 5;

        auto &mouse = input.getMouse();

        if (mouse.getButton(RIGHT)) {
            viewPos += mouse.positionDelta.convert<float>() * deltaTime * panSpeed;
        }

        if (keyboard.getKey(KEY_LSHIFT)) {
            if (mouse.wheelDelta > 0) {
                brushSize++;
            } else if (mouse.wheelDelta < 0) {
                if (brushSize > 0)
                    brushSize--;
            }
        } else {
            if (mouse.wheelDelta > 0.1 || mouse.wheelDelta < -0.1)
                viewScale += mouse.wheelDelta * zoomSpeed * viewScale;
            if (viewScale < 0.01)
                viewScale = 0.01;
        }

        if (mouse.getButton(LEFT)) {
            // Block the grid from ticking if left mouse button is held down or pressed
            blockTick = true;
            tickAccum = 0;

            // Check if mouse position has changed while left button is held down
            bool updateGrid = false;
            if (mouse.positionDelta.x != 0 || mouse.positionDelta.y != 0) {
                auto mpos = getMousePosition();
                if (currentMousePosition != mpos) {
                    updateGrid = true;
                }
            }

            // Set cell if left mouse button was pressed or if the mouse was moved while the left mouse button was held down
            if (mouse.getButtonDown(LEFT) || updateGrid) {
                currentMousePosition = getMousePosition();

                auto mpos = getMousePosition();
                auto influence = getBrushInfluence(mpos);

                for (auto &pos: influence) {
                    if (keyboard.getKey(KEY_LCTRL)) {
                        grid.setCell(pos, false);
                    } else if (updateGrid) {
                        // Set cells to alive if the user is pressing left button and dragging the mouse
                        grid.setCell(pos, true);
                    } else {
                        grid.setCell(pos, !grid.getCell(pos));
                    }
                }
            }
        } else {
            blockTick = false;
        }

        if (keyboardBlockToggle)
            blockTick = true;
    }

    std::unique_ptr<SPIRVCompiler> shaderCompiler;
    std::unique_ptr<SPIRVDecompiler> shaderDecompiler;

    Renderer2D ren2d;
    Renderer2D gridRenderer2d;

    std::unique_ptr<FontDriver> fontDriver;
    std::unique_ptr<Font> font;
    std::unique_ptr<TextRenderer> textRenderer;

    GameGrid<int> grid;

    Vec2f viewPos = Vec2f(0);
    float viewScale = 1;

    float cellSize = 100;
    float cellSpacing = 10;

    float tickAccum = 1.0f;
    float tickDuration = 1.0f;

    float panSpeed = 10.0f;
    float zoomSpeed = 0.1f;

    bool blockTick = false;

    bool keyboardBlockToggle = false;

    Vec2i currentMousePosition;

    uint brushSize = 0;

    ShadeMode mode = SHADE_DEFAULT;
};

#endif //GAMEOFLIFE_GAMEOFLIFE_HPP
