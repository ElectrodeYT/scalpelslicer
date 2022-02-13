//
// Created by alexander on 2/12/22.
//
#include <iostream>
#include <thread>
#include <slicer.h>
#include <omp.h>

void slice_layer(const std::vector<tri>& input, float height, float layer_height, slicer_layer_out* output, float extrusion_per_mm, float extrusion_width) {
    // height = cutting plane
    // std::cout << "Starting Slice at height " << height << std::endl;
    // std::cout << "Input objects size: " << input.size() << std::endl;
    // we basically want to loop through all triangles, find each line of a triangle that intersects, and try to connect the contours
    line line_a;
    line line_b;
    line line_c;
    sf::Vector3f line_a_point;
    sf::Vector3f line_b_point;
    sf::Vector3f line_c_point;

    for(size_t i = 0; i < input.size(); i++) {
        const tri& t = input[i];
        if(t.lowest_z_point() > (height + layer_height)) { break; }
        // We now construct the three lines and check if they intersect
        // if they do, we can calculate the amount of intersecting points we have with this tri
        // if its more then 1, then we can construct a line to work with this
        line_a.update(t.points[0], t.points[1]);
        line_b.update(t.points[1], t.points[2]);
        line_c.update(t.points[2], t.points[0]);

        if(t.isFlat()) {
            // Some more checks to see if this can be added
            float tri_z = t.points[0].z;
            if(tri_z >= height && tri_z <= (height + layer_height) && (tri_z - height) < layer_height) {
                output->fill.push_back(t);
            }
            continue;
        }

        bool line_a_plane = line_a.lineIntersectsPlane(height, layer_height);
        bool line_b_plane = line_b.lineIntersectsPlane(height, layer_height);
        bool line_c_plane = line_c.lineIntersectsPlane(height, layer_height);
        if(line_a_plane) { line_a_point = line_a.getIntersection(height); }
        if(line_b_plane) { line_b_point = line_b.getIntersection(height); }
        if(line_c_plane) { line_c_point = line_c.getIntersection(height); }


        if(line_a_plane && line_b_plane) {
            output->contour.emplace_back(line_a_point, line_b_point);
        }
        if(line_b_plane && line_c_plane) {
            output->contour.emplace_back(line_b_point, line_c_point);
        }
        if(line_c_plane && line_a_plane) {
            output->contour.emplace_back(line_c_point, line_a_point);
        }
    }

    if(!output->fill.empty()) {
        output->solid_infill_lines = fill_to_lines(output->fill, extrusion_width);
    }

    // Generate extrusion lines
    // Basically they are just lines where the material amount that must be extruded is listed
    // They also generally have a thickness
    output->extrusions.resize(output->contour.size());
    for(size_t i = 0; i < output->contour.size(); i++) {
        output->extrusions[i] = line( output->contour[i], output->contour[i].length() * extrusion_per_mm, extrusion_width);
    }
}

slicer_output slice(const std::vector<tri>& input, float layer_height, float extrusion_per_mm) {
    // Get the maximum Z height
    float max_height = max_z_height(input);
    // Copy the input and sort it by lowest point in tri
    std::vector<tri> sorted_input = input;
    std::sort(sorted_input.begin(), sorted_input.end());

    slicer_output output;

    int layer_count = max_height / layer_height;
    // We now know how big output.output is going to be, reserve the size for it
    output.output.resize(((int)(max_height / layer_height) + (int)(fmod(max_height, layer_height) > 0 ? 1 : 0)));
#pragma omp parallel for
    for(int layer = 0; layer <= layer_count; layer++) {
        slice_layer(sorted_input, layer * layer_height, layer_height, &output.output[layer], extrusion_per_mm, 0.4);
    }
    return output;
}

float max_z_height(const std::vector<tri>& input) {
    float output = -1;
    for(size_t i = 0; i < input.size(); i++) {
        if(input[i].points[0].z > output) { output = input[i].points[0].z; }
        if(input[i].points[1].z > output) { output = input[i].points[1].z; }
        if(input[i].points[2].z > output) { output = input[i].points[2].z; }
    }
    return output;
}

std::vector<line> fill_to_lines(std::vector<tri> to_fill, float extrusion_width) {
    // Get the highest point


    return std::vector<line>();
}