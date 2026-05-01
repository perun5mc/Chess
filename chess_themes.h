#ifndef CHESS_THEMES_H
#define CHESS_THEMES_H

#include <map>
#include <string>
#include <vector>
#include "raylib.h"

std::map<std::string, std::vector<Color>> CHESS_THEMES = {
    {"classic", {
                    {238, 238, 210, 255}, // Light square
                    {118, 150, 86, 255},  // Dark square
                    {250, 250, 0, 127}    // Highlight
                }},
    {"brown", {
                  {240, 217, 181, 255}, // Cream
                  {181, 136, 99, 255},  // Brown
                  {255, 255, 100, 127}  // Yellow highlight
              }},
    {"blue", {
                 {240, 240, 240, 255}, // Light gray-blue
                 {120, 160, 200, 255}, // Blue
                 {100, 200, 255, 127}  // Light blue highlight
             }},
    {"gray", {
                 {220, 220, 220, 255}, // Light gray
                 {160, 160, 160, 255}, // Dark gray
                 {255, 150, 50, 127}   // Orange highlight
             }},
    {"purple", {
                   {230, 210, 230, 255}, // Light lavender
                   {170, 130, 180, 255}, // Purple
                   {255, 100, 255, 127}  // Pink highlight
               }},
    {"warm", {
                 {255, 230, 200, 255}, // Light peach
                 {200, 130, 80, 255},  // Warm brown
                 {255, 200, 50, 127}   // Gold highlight
             }},
    {"mono", {
                 {255, 255, 255, 255}, // White
                 {50, 50, 50, 255},    // Almost black
                 {255, 50, 50, 127}    // Red highlight
             }},
    {"ocean", {
                  {200, 230, 255, 255}, // Light blue
                  {70, 130, 180, 255},  // Steel blue
                  {50, 255, 255, 127}   // Cyan highlight
              }},
    {"forest", {
                   {220, 240, 210, 255}, // Light green
                   {100, 150, 100, 255}, // Forest green
                   {150, 255, 150, 127}  // Light green highlight
               }},
    {"golden", {
                   {255, 240, 200, 255}, // Light gold
                   {180, 150, 80, 255},  // Dark gold
                   {255, 215, 0, 127}    // Gold highlight
               }},
    {"midnight", {
                     {180, 180, 200, 255}, // Light blue-gray
                     {80, 80, 120, 255},   // Midnight blue
                     {100, 200, 255, 127}  // Blue highlight
                 }},
    {"autumn", {
                   {255, 230, 200, 255}, // Light beige
                   {180, 100, 60, 255},  // Rust brown
                   {255, 100, 0, 127}    // Deep orange
               }},
    {"ice", {
                {240, 248, 255, 255}, // Alice blue
                {170, 210, 230, 255}, // Light steel blue
                {50, 150, 255, 150}   // Bright blue
            }},
    {"rose", {
                 {255, 240, 245, 255}, // Lavender blush
                 {220, 170, 190, 255}, // Rose
                 {255, 50, 100, 127}   // Bright pink-red
             }},
    {"olive", {
                  {240, 240, 200, 255}, // Light olive
                  {150, 150, 100, 255}, // Olive
                  {255, 255, 150, 127}  // Pale yellow highlight
              }},
    {"modern", {
                   {245, 245, 245, 255}, // Off-white
                   {100, 120, 140, 255}, // Slate gray
                   {70, 130, 180, 127}   // Steel blue highlight
               }}};

#endif