//
// Created by Jeramy on 4/18/2015.
//

#include "Ai.hpp"
#include "MainGame.hpp"
#include "components.hpp"
#include "Engine.hpp"
#include "PhysicsEngine.hpp"
#include "LoadAnimation.hpp"
#include "UpdateSprites.hpp"

#include <json/json.h>

#include <fstream>
#include <cstdlib>

using components::BoundingBox;
using components::Sprite;
using components::Solid;
using components::NoCollide;
using components::Velocity;
using components::LookAt;

MainGame::MainGame() {
    load_level("data/sandy.json");
    cam.setSize(800, 600);
}

MainGame::MainGame(Json::Value json) {
    load(json);
    cam.setSize(800, 600);
}

void MainGame::load_level(std::string fname) {
    Json::Value json;
    std::ifstream file(fname.c_str());
    file >> json;

    load(json);
}

void MainGame::load(Json::Value json) {
    animations.clear();
    animations["player"] = loadAnimation("player.json", texcache);
    animations["bear"] = loadAnimation("bear.json", texcache);
    animations["goomba"] = loadAnimation("goomba.json", texcache);

    {
        Json::Value tjson;
        std::ifstream file("data/terraindata.json");
        file >> tjson;
        for (auto& tdatum : tjson) {
            auto id = tdatum["id"].asInt();
            auto is_solid = tdatum.get("solid", false).asBool();
            terrain_data[id].is_solid = is_solid;
        }
    }

    entities = DB{};

    auto& tex = texcache.get("terrain");
    auto sprW = tex.getSize().x / 16.f;
    auto sprH = tex.getSize().y / 16.f;
    std::vector<Sprite> terrasprites;
    terrasprites.reserve(256);
    for (int i = 0; i < 256; ++i) {
        Sprite sprite{tex, sf::IntRect((i % 16) * sprW, (i / 16) * sprH, sprW, sprH),
                      terrain_data[i].is_solid ? 1 : -1};
        terrasprites.push_back(sprite);
    }

    auto width = json["width"].asInt();
    auto height = json["height"].asInt();

    auto y = 0.f;
    for (auto& row : json["rows"]) {
        auto x = 0.f;
        for (auto& tile : row) {
            auto t = tile.asInt();
            if (t > 0) {
                Sprite spr = terrasprites[t];
                auto ent = entities.makeEntity();
                entities.makeComponent(ent, spr);
                BoundingBox bb{{x, y, sprW, sprH}};
                entities.makeComponent(ent, bb);
                if (terrain_data[t].is_solid) {
                    entities.makeComponent(ent, Solid{});
                } else {
                    entities.makeComponent(ent, NoCollide{});
                }
            }
            x += sprW;
        }
        y += sprH;
    }
    {
        Sprite player_sprite;
        player_sprite.spr.setAnimation(animations.at("player").at("walk"));
        player_sprite.spr.setFrameTime(sf::seconds(0.25f));
        player_sprite.spr.setLooped(true);
        player_sprite.spr.update(sf::seconds(1));
        player_sprite.layer = 0;

        BoundingBox player_bb{{json["player_spawn"]["col"].asInt() * sprW,
                               json["player_spawn"]["row"].asInt() * sprH, sprW, sprH * 2}};
        Velocity player_vel{};
        AIComponent player_ai{AIComponent{PlayerAI{[=](EntID peid) {
            auto pbb = peid.get<BoundingBox>().data().rect;
            Sprite bear_sprite;
            bear_sprite.spr.setAnimation(animations.at("bear").at("walk"));
            bear_sprite.spr.setFrameTime(sf::seconds(0.25f));
            bear_sprite.spr.setLooped(true);
            bear_sprite.spr.update(sf::seconds(1));
            bear_sprite.layer = 1;

            BoundingBox bear_bb{{pbb.left + sprW, pbb.top, sprW, sprH}};
            Velocity bear_vel{};

            auto bear = entities.makeEntity();
            entities.makeComponent(bear, bear_sprite);
            entities.makeComponent(bear, bear_bb);
            entities.makeComponent(bear, bear_vel);
            return bear;
        }}}};

        player = entities.makeEntity();
        entities.makeComponent(player, player_sprite);
        entities.makeComponent(player, player_bb);
        entities.makeComponent(player, player_vel);
        entities.makeComponent(player, player_ai);
    }
    {
        Sprite goomba_sprite;
        goomba_sprite.spr.setAnimation(animations.at("goomba").at("walk"));

        BoundingBox goomba_bb{{750, 200, sprW, sprH}};
        Velocity goomba_vel{};
        GoombaAI goombaBrain{};
        AIComponent goomba_ai(goombaBrain);

        EntID goomba = entities.makeEntity();
        entities.makeComponent(goomba, goomba_sprite);
        entities.makeComponent(goomba, goomba_bb);
        entities.makeComponent(goomba, goomba_vel);
        entities.makeComponent(goomba, goomba_ai);
        entities.makeComponent(goomba, Enemy{});
    }
    {
        Sprite goomba_sprite;
        goomba_sprite.spr.setAnimation(animations.at("goomba").at("walk"));

        BoundingBox goomba_bb{{700, 200, sprW, sprH}};
        Velocity goomba_vel{};
        GoombaAI goombaBrain{};
        AIComponent goomba_ai(goombaBrain);

        EntID goomba = entities.makeEntity();
        entities.makeComponent(goomba, goomba_sprite);
        entities.makeComponent(goomba, goomba_bb);
        entities.makeComponent(goomba, goomba_vel);
        entities.makeComponent(goomba, goomba_ai);
        entities.makeComponent(goomba, Enemy{});
    }
    {
        Sprite goomba_sprite;
        goomba_sprite.spr.setAnimation(animations.at("goomba").at("walk"));

        BoundingBox goomba_bb{{500, 200, sprW, sprH}};
        Velocity goomba_vel{};
        GoombaAI goombaBrain{};
        AIComponent goomba_ai(goombaBrain);

        EntID goomba = entities.makeEntity();
        entities.makeComponent(goomba, goomba_sprite);
        entities.makeComponent(goomba, goomba_bb);
        entities.makeComponent(goomba, goomba_vel);
        entities.makeComponent(goomba, goomba_ai);
        entities.makeComponent(goomba, Enemy{});
    }

}

bool MainGame::halts_update() const {
    return true;
}

bool MainGame::halts_draw() const {
    return true;
}

void MainGame::update(Engine& engine, double time_step) {
    update_ais(engine, entities);
    physics_step(entities, time_step);
    update_sprites(entities, time_step);
    update_timers(entities, time_step);
    update_animations();

    {
        auto lookat = player;
        while (auto lookatinfo = lookat.get<LookAt>()) {
            lookat = lookatinfo.data().target;
        }
        auto& playerbb = lookat.get<BoundingBox>().data().rect;
        cam.setPosition(playerbb.left + playerbb.width / 2.0 - 400, playerbb.top + playerbb.height / 2.0 - 300);
    }

    if (engine.wasKeyPressed(sf::Keyboard::Escape)) {
        return engine.states.pop();
    }
}

void MainGame::update_animations() {
    using namespace components;
    auto items = entities.query<AiStateComponent, Sprite>();

    for (auto& ent : items) {
        AiStateComponent& ai = std::get<1>(ent).data();
        Sprite& spr = std::get<2>(ent).data();

        if (std::tie(ai.flipped,ai.anim_name,ai.anim_group) != std::tie(spr.prev_state.flipped,spr.prev_state.anim_name,spr.prev_state.anim_group)) {
            spr.flipped = ai.flipped;
            spr.spr.setAnimation(animations.at(ai.anim_group).at(ai.anim_name));
            spr.prev_state = ai;
        }
    }
}

void MainGame::draw(sf::RenderWindow& window) const {
    cam.draw(window, entities);
}
