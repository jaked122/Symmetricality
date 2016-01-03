//
// Created by awhite on 12/27/15.
//

#include "PlanRenderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

PlanRenderer::PlanRenderer() {
    blueprint.setDesignation(current_designation->first);
}

PlanRenderer::~PlanRenderer() {

}

void PlanRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    states.transform *= getTransform();
    states.texture = NULL;
    target.draw(Rendering_plan, states);
    target.draw(Symmetries, states);
    target.draw(Cursor, states);
    states.texture = &buildingTexture;
    target.draw(Buildings, states);
    if (blueprint.isDesignating())
        target.draw(Designation_preview, states);

}

void PlanRenderer::build_vertex_array() {
    sf::Vertex *current;
    auto current_floor = &blueprint.getLevelDesignation(Floornum);
    //if(designation_changed) {
    buildDesignationArray(current, current_floor);
    //    designation_changed = false;
    //}
    Cursor.resize(4);
    Cursor.setPrimitiveType(sf::PrimitiveType::Quads);
    current = &Cursor[0];
    current[0].position.x = (0.25f + cursorpos.x) * m_square_size;
    current[0].position.y = (0.25f + cursorpos.y) * m_square_size;
    current[1].position = sf::Vector2f(current[0].position.x + .5f * m_square_size, current[0].position.y);
    current[2].position = sf::Vector2f(current[1].position.x, current[1].position.y + 0.5f * m_square_size);
    current[3].position = sf::Vector2f(current[2].position.x - 0.5f * m_square_size, current[2].position.y);
    for (int i = 0; i < 4; i++)
        current[i].color =
                building_mode && !blueprint.canPlace(cursorpos.x, cursorpos.y, Floornum, current_building->second)
                ? sf::Color::Red//the building can't be built
                : sf::Color::White;//the building can be built.
    //Buildings
    //if(building_changed) {
    const std::unordered_map<sf::Vector2i, std::string> &f = blueprint.getLevelBuildings(Floornum);
    Buildings.resize(f.size() * 4);
    Buildings.setPrimitiveType(sf::Quads);
    current = &Buildings[0];
    for (auto i:f) {
        std::string e = i.second;
        _building_types[e].getTexCoords(current);
        _building_types[e].getAdjustedCoords(i.first.x, i.first.y, m_square_size, current);
        current++;
        current++;
        current++;
        current++;
    }
    building_changed = false;
    // }
    //Symmetries
    auto symmetries = blueprint.getSymmetries();
    Symmetries.resize(3 * symmetries.size() + 3);
    Symmetries.setPrimitiveType(sf::PrimitiveType::Triangles);

    for (int i = 0; i < symmetries.size(); i++) {
        current = &Symmetries[i * 3];
        auto v = symmetries[i].getCursor();
        current[0].position = sf::Vector2f(v.x * m_square_size, v.y * m_square_size);
        current[1].position = sf::Vector2f((1 + v.x) * m_square_size, v.y * m_square_size);
        current[2].position = sf::Vector2f((0.5f + v.x) * m_square_size, (1 + v.y) * m_square_size);
        for (int z = 0; z < 3; z++)
            current[z].color = symmetries[i].getColor();
    }
    auto blueprint_start_point = blueprint.getStartPoint();
    current = &Symmetries[Symmetries.getVertexCount() - 3];
    current[0].position = sf::Vector2f((blueprint_start_point.x) * m_square_size,
                                       (1 + blueprint_start_point.y) * m_square_size);
    current[1].position = sf::Vector2f((1 + blueprint_start_point.x) * m_square_size,
                                       (1 + blueprint_start_point.y) * m_square_size);
    current[2].position = sf::Vector2f((blueprint_start_point.x + 0.5f) * m_square_size,
                                       (blueprint_start_point.y) * m_square_size);
    current[0].color = sf::Color::Magenta;
    current[1].color = sf::Color::Cyan;
    current[2].color = sf::Color::White;
    build_designation();
}

void PlanRenderer::buildDesignationArray(sf::Vertex *current,
                                         const std::unordered_map<sf::Vector2i, char> *current_floor) {
    Rendering_plan.resize(current_floor->size() * 4);
    Rendering_plan.setPrimitiveType(sf::Quads);
    //TODO figure out a more efficient way to do this..
    //TODO figure out if it makes sense to make it more efficient than this.
    int iter = 0;
    for (auto i:*current_floor) {
        current = &Rendering_plan[iter];
        current[0].position = sf::Vector2f(i.first.x * m_square_size, i.first.y * m_square_size);
        current[1].position = sf::Vector2f((i.first.x + 1) * m_square_size, i.first.y * m_square_size);
        current[2].position = sf::Vector2f((i.first.x + 1) * m_square_size, (1 + i.first.y) * m_square_size);
        current[3].position = sf::Vector2f((i.first.x) * m_square_size, (1 + i.first.y) * m_square_size);
        for (int z = 0; z < 4; z++)
            current[z].color = designation_colors.find(i.second)->second;
        iter += 4;
    }
}

void PlanRenderer::set_pos(sf::Vector2i i, char i1) {
    build_vertex_array();
}

void PlanRenderer::handle_event(sf::Event event) {
    if (event.type == sf::Event::KeyPressed) {
        int offset_size = event.key.shift ? 10 : 1;
        switch (event.key.code) {
            case sf::Keyboard::Left:
            case sf::Keyboard::Numpad4:
                move_cursor(-offset_size, 0);
                break;
            case sf::Keyboard::Numpad6:
            case sf::Keyboard::Right:
                move_cursor(offset_size, 0);
                break;
            case sf::Keyboard::Up:
            case sf::Keyboard::Numpad8:
                move_cursor(0, -offset_size);
                break;
            case sf::Keyboard::Down:
            case sf::Keyboard::Numpad2:
                move_cursor(0, offset_size);
                break;
            case sf::Keyboard::Space:
                if (building_mode) {
                    blueprint.placeBuilding(cursorpos.x, cursorpos.y, Floornum, current_building->second);
                    building_changed = true;
                }
                else {
                    designation_changed = true;
                    blueprint.insert(cursorpos.x, cursorpos.y, Floornum, current_designation->first);
                }
                break;
            case sf::Keyboard::Comma:
                move_z(1);
                break;
            case sf::Keyboard::Period:
                move_z(-1);
                break;
            case sf::Keyboard::R:
                add_symmetry(Symmetry::Radial);
                break;
            case sf::Keyboard::X:
                add_symmetry(Symmetry::X);
                break;
            case sf::Keyboard::Y:
                add_symmetry(Symmetry::Y);
                break;
            case sf::Keyboard::Num9:
                add_symmetry(Symmetry::Rotational);
                break;
            case sf::Keyboard::Equal:
                if (event.key.control) {
                    m_square_size++;
                } else
                    change_designation(true);
                break;
            case sf::Keyboard::Dash:
                if (event.key.control) {
                    m_square_size--;
                } else
                    change_designation(false);
                break;
            case sf::Keyboard::B:
                building_mode = !building_mode;
                break;
            case sf::Keyboard::Return:
                blueprint.setDesignation(current_designation->first);
                blueprint.setDesignationToggle(cursorpos.x, cursorpos.y, Floornum,
                                               event.key.shift ? Blueprint::CIRCLE : (event.key.control
                                                                                      ? Blueprint::LINE
                                                                                      : Blueprint::RECTANGLE));
                designation_changed = true;
                break;
            case sf::Keyboard::Numpad9:
                move_cursor(-offset_size, -offset_size);
                break;
            case sf::Keyboard::Numpad1:
                move_cursor(offset_size, offset_size);
                break;
            case sf::Keyboard::Numpad7:
                move_cursor(offset_size, -offset_size);
                break;
            case sf::Keyboard::Numpad3:
                move_cursor(-offset_size, offset_size);
                break;
            case sf::Keyboard::S:
                blueprint.setStart(cursorpos.x, cursorpos.y);
                break;
        }
        if (event.key.code == sf::Keyboard::C) {
            if (clear_primed) {
                blueprint = Blueprint();
                Floornum = 0;
            }
            clear_primed = true;
        } else
            clear_primed = false;
    }

}

void PlanRenderer::move_cursor(int x, int y) {
    cursorpos.x += x;
    cursorpos.y += y;
    build_vertex_array();
}

void PlanRenderer::move_z(int offset) {
    this->Floornum += offset;
    build_vertex_array();
}


void PlanRenderer::add_symmetry(Symmetry::Symmetry_Type type) {
    blueprint.addSymmetry(cursorpos.x, cursorpos.y, type);

}

void PlanRenderer::build_designation() {
    if (blueprint.isDesignating()) {
        const sf::Vector3i &start = blueprint.getDesignationStart();
        const sf::Vector3i &end = sf::Vector3i(cursorpos.x, cursorpos.y, Floornum);
        Designation_preview.resize(8);
        Designation_preview.setPrimitiveType(sf::PrimitiveType::Lines);
        sf::Vertex *the_square = &Designation_preview[0];
        the_square[0].position = sf::Vector2f((0.5f + start.x) * m_square_size, (0.5f + start.y) * m_square_size);
        the_square[1].position = sf::Vector2f((0.5f + end.x) * m_square_size, (0.5f + start.y) * m_square_size);
        the_square[2].position = sf::Vector2f((0.5f + start.x) * m_square_size, (0.5f + start.y) * m_square_size);
        the_square[3].position = sf::Vector2f((0.5f + start.x) * m_square_size, (0.5f + end.y) * m_square_size);
        the_square[4].position = sf::Vector2f((0.5f + end.x) * m_square_size, (0.5f + start.y) * m_square_size);
        the_square[5].position = sf::Vector2f((0.5f + end.x) * m_square_size, (0.5f + end.y) * m_square_size);
        the_square[6].position = sf::Vector2f((0.5f + start.x) * m_square_size, (0.5f + end.y) * m_square_size);
        the_square[7].position = sf::Vector2f((0.5f + end.x) * m_square_size, (0.5f + end.y) * m_square_size);
        for (int i = 0; i < 8; i++)
            the_square[i].color = sf::Color(109, 93, 98);
    } else {
        Designation_preview.resize(0);
    }

}

void PlanRenderer::change_designation(bool up) {

    if (up) {
        if (building_mode) {
            current_building++;
            if (current_building == _building_types.end())
                current_building = _building_types.begin();
        } else {
            current_designation++;
            if (current_designation == designation_colors.end())
                current_designation = designation_colors.begin();
        }
    } else {
        if (building_mode) {
            if (current_building == _building_types.begin())
                current_building = _building_types.end();
            current_building--;
        } else {
            if (current_designation == designation_colors.begin())
                current_designation = designation_colors.end();
            current_designation--;
        }
    }

    blueprint.setDesignation(current_designation->first);
}

void PlanRenderer::export_csv(const std::string &string) const {
    blueprint.export_csv(string);
}

void PlanRenderer::serialize(const std::string &string) const {
    blueprint.serialize(string);
}

void PlanRenderer::deserialize(const std::string &string) {
    blueprint.deserialize(string);
}

void PlanRenderer::handle_mouse(sf::Event event, const sf::Vector2f &b) {
    sf::Vector2i mouse_position = sf::Vector2i((int) floor(b.x / m_square_size), (int) floor(b.y / m_square_size));
    blueprint.setDesignationToggle(sf::Vector3i(mouse_position.x, mouse_position.y, Floornum),
                                   event.mouseButton.button == 0 ? (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)
                                                                    ? Blueprint::CIRCLE : Blueprint::RECTANGLE)
                                                                 : Blueprint::LINE);
    designation_changed = true;
    build_vertex_array();
}

void PlanRenderer::handleMouseOver(const sf::Vector2f &b) {
    sf::Vector2i mouse_position = sf::Vector2i((int) floor(b.x / m_square_size), (int) floor(b.y / m_square_size));
    if (blueprint.isDesignating())
        cursorpos = mouse_position;
    build_designation();

}

void PlanRenderer::setColor(char f, sf::Color c) {
    designation_colors[f] = c;
}

void PlanRenderer::loadBuildingTexture(const std::string &filename) {
    this->buildingTexture.loadFromFile(filename);

}

void PlanRenderer::getLoadBuildings(GetPot &pot) {
    int number = pot("buildings/BuildingCount", 0);
    std::cout << "Found " << number << " Buildings.\n";
    for (int i = 0; i < number; i++) {
        std::stringstream z;
        z << "buildings/" << i << "/" << "Buildingname";
        std::string curbuild = std::to_string(i);
        std::string bname = pot(z.str().c_str(), "");
        std::cout << "Building_name:" << bname << std::endl;
        z.str("");
        z << "buildings/" << i << "/" << "key_sequence";

        std::string keySeq = pot(z.str().c_str(), "");
        std::cout << "Key sequence:" << keySeq << std::endl;
        z.str("");
        z << "buildings/" << i << "/" << "size_x";
        int sx = pot(z.str().c_str(), 0);
        z.str("");
        z << "buildings/" << i << "/" << "size_y";
        int sy = pot(z.str().c_str(), 0);
        z.str("");
        z << "buildings/" << i << "/" << "center_x";
        int cx = pot(z.str().c_str(), 0);
        z.str("");
        z << "buildings/" << i << "/" << "center_y";
        int cy = pot(z.str().c_str(), 0);
        z.str("");
        z << "buildings/" << i << "/" << "texturecoords/X1";

        int tx1 = pot(z.str().c_str(), 0);
        z.str("");
        z << "buildings/" << i << "/" << "texturecoords/Y1";
        int ty1 = pot(z.str().c_str(), 0);
        z.str("");
        z << "buildings/" << i << "/" << "texturecoords/X2";
        int tx2 = pot(z.str().c_str(), 0);
        z.str("");
        z << "buildings/" << i << "/" << "texturecoords/Y2";
        int ty2 = pot(z.str().c_str(), 0);
        _building_types[keySeq] = Building(bname, keySeq, sf::Vector2i(sx, sy), sf::Vector2i(cx, cy),
                                           tx1, ty1, tx2, ty2);
    }
    current_building = _building_types.begin();
}
