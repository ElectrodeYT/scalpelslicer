//
// Created by alexander on 2/12/22.
//

#ifndef SCALPELSLICER_SLICER_H
#define SCALPELSLICER_SLICER_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

namespace bg = boost::geometry;


struct tri {
    sf::Vector3f points[3];

    void dump() {
        if(points[1].z > 3) { return; }
        std::cout << points[0].x << " " << points[0].y << " " << points[0].z << std::endl;
        std::cout << "\t" << points[1].x << " " << points[1].y << " " << points[1].z << std::endl;
        std::cout << "\t" << points[2].x << " " << points[2].y << " " << points[2].z << std::endl;
    }

    constexpr bool isFlat() const {
        return (points[0].z == points[1].z) && (points[1].z == points[2].z);
    }

    constexpr float lowest_z_point() const {
        float lowest = points[0].z;
        if(points[1].z < lowest) { lowest = points[1].z; }
        if(points[2].z < lowest) { lowest = points[2].z; }
        return lowest;
    }

    bool operator<(const tri& other) const {
        return lowest_z_point() < other.lowest_z_point();
    }

    bool operator>(const tri& other) const {
        return lowest_z_point() > other.lowest_z_point();
    }
};

struct line {
    sf::Vector3f a;
    sf::Vector3f b;

    float extrusion_amount = 0.0;
    float width = 0.0;

    sf::Vector2f a_2d() const { return {a.x, a.y}; }
    sf::Vector2f b_2d() const { return {b.x, b.y}; }


    line() = default;
    line(sf::Vector3f _a, sf::Vector3f _b, float _e = 0, float _w = 0) : a(_a), b(_b), extrusion_amount(_e), width(_w) { }
    line(line contour, float _e, float _w) : a(contour.a), b(contour.b), extrusion_amount(_e), width(_w) { }

    constexpr bool lineIntersectsPlane(float height, float layer_height) {
        float low_z = a.z > b.z ? a.z : b.z;
        float high_z = a.z > b.z ? b.z : a.z;
        return (low_z >= height) && (high_z <= height);
    }

    inline sf::Vector3f getIntersection(float height) const {
        float t = (height - b.z) / (a.z - b.z);
        return {(a.x * t) + (b.x * (1 - t)), (a.y * t) + (b.y * (1 - t)), height};
    }

    constexpr void update(const sf::Vector3f& _a, const sf::Vector3f& _b) {
        a = _a;
        b = _b;
    }

    constexpr double length() const {
        return sqrt(pow((b.x - a.x), 2) + pow((b.y - a.y), 2) + pow((b.y - a.y), 2));
    }
};

struct slicer_layer_out {
    // Information for displaying
    std::vector<line> contour;
    std::vector<tri> fill;

    // Generated lines for solid infill
    std::vector<line> solid_infill_lines;

    // Lines to extrude
    std::vector<line> extrusions;
    float extrusion_per_mm = 1;
};

struct slicer_output {
    // Information for displaying
    std::vector<slicer_layer_out> output;
};

void slice_layer(const std::vector<tri>& input, float height, float layer_height, slicer_layer_out* output, float extrusion_per_mm = 1, float extrusion_width = 0.4);
slicer_output slice(const std::vector<tri>& input, float layer_height, float extrusion_per_mm = 1);

float max_z_height(const std::vector<tri>& input);

std::vector<std::string> generate_gcode_for_layer(slicer_layer_out input);
std::vector<line> fill_to_lines(std::vector<tri> to_fill, float extrusion_width);

#endif //SCALPELSLICER_SLICER_H
