#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <stl_reader.h>
#include <slicer.h>
#include <string>
#include <vector>
#include <math.h>

#define PI 3.141592654

std::vector<tri> object;

void attemptLoadSTL(const std::string& s) {
    try {
        stl_reader::StlMesh<float, unsigned int> mesh(s.c_str());
        object.clear();
        for(size_t i = 0; i < mesh.num_tris(); i++) {
            tri new_tri;
            for(size_t icorner = 0; icorner < 3; icorner++) {
                const float *c = mesh.vrt_coords(mesh.tri_corner_ind(i, icorner));
                new_tri.points[icorner].x = c[0];
                new_tri.points[icorner].y = c[1];
                new_tri.points[icorner].z = c[2];
            }
            new_tri.dump();
            object.push_back(new_tri);
        }
        std::cout << "Tri count: " << mesh.num_tris() << std::endl;
    } catch(std::exception& e) {
        std::cout << "failed to load stl file: " << e.what() << std::endl;
    }
}

void renderPreview(sf::RenderWindow& window, slicer_layer_out output, float expansion_factor, float preview_move_x, float preview_move_y, bool renderContour, bool renderFill, bool renderExtrusion) {
    if(renderContour) {
        for (size_t i = 0; i < output.contour.size(); i++) {
            sf::Vector2f a = output.contour[i].a_2d() * expansion_factor;
            sf::Vector2f b = output.contour[i].b_2d() * expansion_factor;
            a.x += preview_move_x;
            a.y += preview_move_y;
            b.x += preview_move_x;
            b.y += preview_move_y;
            sf::Vertex line[2] = {
                    sf::Vertex(a),
                    sf::Vertex(b),
            };
            window.draw(line, 2, sf::Lines);
        }
    }
    if(renderFill) {
        sf::ConvexShape convex;
        convex.setPointCount(3);
        convex.setFillColor(sf::Color::Green);
        for (size_t i = 0; i < output.fill.size(); i++) {
            tri t = output.fill[i];
            sf::Vector2f point_a(t.points[0].x, t.points[0].y);
            sf::Vector2f point_b(t.points[1].x, t.points[1].y);
            sf::Vector2f point_c(t.points[2].x, t.points[2].y);
            point_a *= expansion_factor;
            point_b *= expansion_factor;
            point_c *= expansion_factor;

            point_a.x += preview_move_x;
            point_a.y += preview_move_y;
            point_b.x += preview_move_x;
            point_b.y += preview_move_y;
            point_c.x += preview_move_x;
            point_c.y += preview_move_y;

            convex.setPoint(0, point_a);
            convex.setPoint(1, point_b);
            convex.setPoint(2, point_c);
            window.draw(convex);
        }
    }
    if(renderExtrusion) {
        sf::RectangleShape rect;
        rect.setFillColor(sf::Color::Blue);
        for(size_t i = 0; i < output.extrusions.size(); i++) {
            const line& extrusion = output.extrusions[i];
            float angle = atan((extrusion.b.y - extrusion.a.y) / (extrusion.b.x - extrusion.a.x));
            rect.setSize(sf::Vector2f(extrusion.length() * expansion_factor, extrusion.width * expansion_factor));
            rect.setRotation(((angle * 180) / PI) + 90.0f);
            rect.setPosition(sf::Vector2f((extrusion.a.x * expansion_factor) + preview_move_x, (extrusion.a.y * expansion_factor) + preview_move_y));
            window.draw(rect);
        }

        /*
          for (size_t i = 0; i < output.extrusions.size(); i++) {
            sf::Vector2f a = output.extrusions[i].a_2d() * expansion_factor;
            sf::Vector2f b = output.extrusions[i].b_2d() * expansion_factor;
            a.x += preview_move_x;
            a.y += preview_move_y;
            b.x += preview_move_x;
            b.y += preview_move_y;
            sf::Vertex line[2] = {
                    sf::Vertex(a, sf::Color::Blue),
                    sf::Vertex(b, sf::Color::Green),
            };
            window.draw(line, 2, sf::Lines);
        }
        */
    }
}


int main() {
    sf::RenderWindow window(sf::VideoMode(2000, 2000), "ScalpelSlicer");

    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    sf::Clock deltaClock;

    attemptLoadSTL("3DBenchy.stl");

    bool first_loop = true;

    sf::Vector2i preview_offset;
    preview_offset.x = window.getSize().x / 2;
    preview_offset.y = window.getSize().y / 2;

    // Slicer engine stuff
    slicer_output slicer_output;
    sf::Time reslice_time;
    bool reslice_required = false;

    // Preview moving stuff
    bool middle_mouse_moving = false;
    sf::Vector2i old_pos;
    while(window.isOpen()) {
        static int layer = 0;
        static float layer_height = 0.2;
        static float preview_expansion_factor = 10;

        static bool renderContour = true;
        static bool renderFill = true;
        static bool renderExtrusion = true;

        sf::Event event;
        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed) { window.close(); break; }
            else if (event.type == sf::Event::Resized) {
                // update the view to the new size of the window
                sf::FloatRect visibleArea(0.f, 0.f, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
                preview_offset.x = window.getSize().x / 2;
                preview_offset.y = window.getSize().y / 2;
                ImGui::SFML::ProcessEvent(window, event);
                break; // immideatly redraw
            } else if(event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Button::Right) {
                // We dont let ImGui know of the middle mouse button
                middle_mouse_moving = true;
                old_pos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
            } else if(event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Button::Right) {
                middle_mouse_moving = false;
            } else if(event.type == sf::Event::MouseMoved && middle_mouse_moving) {
                sf::Vector2i new_pos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
                sf::Vector2i delta_pos;
                delta_pos.x = old_pos.x - new_pos.x;
                delta_pos.y = old_pos.y - new_pos.y;
                preview_offset.x -= delta_pos.x;
                preview_offset.y -= delta_pos.y;
                old_pos = new_pos;
            } else if(event.type == sf::Event::MouseWheelScrolled) {
                preview_expansion_factor += event.mouseWheelScroll.delta;
                ImGui::SFML::ProcessEvent(window, event);
            } else {
                ImGui::SFML::ProcessEvent(window, event);
            }
        }
        ImGui::SFML::Update(window, deltaClock.restart());
        ImGui::Begin("Settings");


        if(first_loop) { reslice_required = true; }

        ImGui::InputInt("Layer", &layer, 1, 1 / layer_height);
        if(layer < 0) { layer = 0.0f; }
        if(layer >= slicer_output.output.size()) { layer = slicer_output.output.size() - 1; }
        if(ImGui::InputFloat("Layer Height", &layer_height, 0.05f, 0.1f)) { reslice_required = true; }
        if(layer_height <= 0 || layer_height == -0.0f) { layer_height = 0.0f; }
        ImGui::Text("Current Height: %f", layer * layer_height);
        ImGui::InputFloat("Preview Expansion Factor", &preview_expansion_factor, 0.5f, 1.0f);
        ImGui::Checkbox("Render Contour", &renderContour);
        ImGui::Checkbox("Render Fill", &renderFill);
        ImGui::Checkbox("Render Extrusion", &renderExtrusion);
        if(layer_height <= 0.00001) { ImGui::Text("Invalid slicing settings!"); } else {
            if (ImGui::Button("Slice") || first_loop) {
                sf::Clock reslice_clock;
                slicer_output = slice(object, layer_height);
                reslice_time = reslice_clock.restart();
                layer = 0;
                reslice_required = false;
            }
        }
        ImGui::Text("Time to slice: %i ms", reslice_time.asMilliseconds());
        if(reslice_required) {
            ImGui::Text("Reslice required");
        }
        ImGui::End();

        window.clear(sf::Color::Black);

        if(slicer_output.output.size() > layer) {
            renderPreview(window, slicer_output.output[layer], preview_expansion_factor, preview_offset.x,
                          preview_offset.y, renderContour, renderFill, renderExtrusion);
        }

        ImGui::SFML::Render(window);
        window.display();

        if(first_loop) { first_loop = false; }
    }

    return 0;
}
