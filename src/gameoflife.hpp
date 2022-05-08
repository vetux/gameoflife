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

using namespace xengine;

class GameOfLife : public Application {
public:
    GameOfLife(int argc, char *argv[])
            : Application(argc, argv), ren2d(*renderDevice) {
        window->setTitle("Game Of Life");
    }

protected:
    void start() override {
        std::ifstream stream("asset/Roboto-Regular.ttf");
        font = Font::createFont(stream);
        textRenderer = std::make_unique<TextRenderer>(*font, *renderDevice);
    }

    void update(float deltaTime) override {
        updateInput(deltaTime);

        if (tickAccum + deltaTime >= tickDuration) {
            tickAccum = 0;
            grid = grid.stepTime();
        } else {
            tickAccum += deltaTime;
        }

        auto &target = window->getRenderTarget(graphicsBackend);

        renderDevice->getRenderer().renderClear(target, ColorRGBA::black(), 1);

        drawCursor(target);
        drawGrid(target);
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
        return screenToWorld(window->getInput().getMice().at(0).position.convert<float>()
                             - Vec2f(cellSize + cellSpacing, cellSize + cellSpacing) * viewScale / 2,
                             window->getRenderTarget(graphicsBackend).getSize().convert<float>());
    }

    void drawTile(RenderTarget &target, Vec2i pos, ColorRGBA color, bool fillSpacing) {
        Vec2f screenPos = worldToScreen(pos, target.getSize().convert<float>());

        auto size = cellSize * viewScale;

        if (fillSpacing) {
            screenPos -= Vec2f(cellSpacing, cellSpacing) / 2 * viewScale;
            size += cellSpacing * viewScale;
        }

        Rectf rect(screenPos, {size, size});

        ren2d.draw(rect, color);
    }

    void drawCursor(RenderTarget &target) {
        ren2d.renderBegin(target, false);

        auto mpos = getMousePosition();

        drawTile(target, mpos, ColorRGBA::green(0.7), true);

        ren2d.renderPresent();
    }

    void drawGrid(RenderTarget &target) {
        ren2d.renderBegin(target, false);

        for (auto &x: grid.cells) {
            for (auto &y: x.second) {
                Vec2i pos(x.first, y);
                drawTile(target, pos, ColorRGBA::white(), false);
            }
        }

        ren2d.renderPresent();
    }

    void drawGui(RenderTarget &target, float deltaTime) {
        auto cellCount = 0;
        for (auto &x: grid.cells)
            for (auto &y: x.second)
                cellCount++;

        font->setPixelSize({0, 50});

        auto deltaText = textRenderer->render("Delta Time: " + std::to_string(deltaTime), 30);
        auto text = textRenderer->render("Alive cells: " + std::to_string(cellCount), 30);

        auto mpos = getMousePosition();
        auto mtext = textRenderer->render("Position: " + std::to_string(mpos.x) + " " + std::to_string(mpos.y)
                                          + "\nZoom: " + std::to_string(viewScale),
                                          30);

        ren2d.renderBegin(target, false);

        static const float padding = 10.0f;

        ren2d.draw(deltaText,
                   Rectf({padding, padding},
                         deltaText.getTexture().getAttributes().size.convert<float>()),
                   ColorRGBA::white());
        ren2d.draw(mtext,
                   Rectf({padding, padding
                                   + deltaText.getTexture().getAttributes().size.y
                                   + padding},
                         mtext.getTexture().getAttributes().size.convert<float>()),
                   ColorRGBA::white());

        ren2d.draw(text,
                   Rectf({padding, padding
                                   + deltaText.getTexture().getAttributes().size.y
                                   + padding
                                   + mtext.getTexture().getAttributes().size.y
                                   + padding},
                         text.getTexture().getAttributes().size.convert<float>()),
                   ColorRGBA::white());

        ren2d.renderPresent();
    }

    void updateInput(float deltaTime) {
        auto &input = window->getInput();
        auto &keyboard = input.getKeyboards().at(0);

        if (keyboard.getKey(xengine::KEY_LEFT)) {
            viewPos.x -= deltaTime * panSpeed;
        }
        if (keyboard.getKey(xengine::KEY_RIGHT)) {
            viewPos.x += deltaTime * panSpeed;
        }
        if (keyboard.getKey(xengine::KEY_UP)) {
            viewPos.y -= deltaTime * panSpeed;
        }
        if (keyboard.getKey(xengine::KEY_DOWN)) {
            viewPos.y += deltaTime * panSpeed;
        }

        auto wheelDelta = input.getMice().at(0).wheelDelta;
        if (wheelDelta > 0.1 || wheelDelta < -0.1)
            viewScale += wheelDelta * deltaTime * zoomSpeed;
        if (viewScale < 0.01)
            viewScale = 0.01;

        auto &mouse = input.getMice().at(0);
        if (mouse.getButton(xengine::LEFT)) {
            auto mpos = getMousePosition();
            grid.setCell(mpos, !grid.getCell(mpos));
        }
    }

    Renderer2D ren2d;

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
    float zoomSpeed = 10.0f;
};

#endif //GAMEOFLIFE_GAMEOFLIFE_HPP
