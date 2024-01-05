#ifndef COLOR
#define COLOR

#include "Mathf.h"

struct Color {
    public:
        float r;
        float g;
        float b;

        Color() : r(0), g(0), b(0) {}
        Color(float r, float g, float b): r(r), g(g), b(b) {}

        inline const uint32_t toGRB() const {
            return (uint32_t(g * 255) << 16) | (uint32_t(r * 255) << 8) | uint32_t(b * 255); 
        }

        inline const uint32_t toRGB() const {
            return (uint32_t(r * 255) << 16) | (uint32_t(g * 255) << 8) | uint32_t(b * 255); 
        }

        inline static Color lerp(const Color& a, const Color& b, float ratio) {
            return Color(Mathf::lerp(a.r, b.r, ratio), Mathf::lerp(a.g, b.g, ratio), Mathf::lerp(a.r, b.r, ratio));
        }

        inline static Color white(){
            return Color(1,1,1);
        }

        inline static Color black(){
            return Color();
        }

        inline static Color green(){
            return Color(0,1,0);
        }

        inline static Color red(){
            return Color(1,0,0);
        }

        inline static Color moveTowards(const Color& a, const Color& b, float distance) {
            Color delta = b - a;
            float currentDistance = sqrtf(delta.r * delta.r + delta.g * delta.g + delta.b * delta.b);
            
            if (currentDistance < distance)
                return b;

            Color normalizedDelta = delta / currentDistance;
            return a + normalizedDelta * distance;
        }

        inline Color operator-(const Color rhs) const { return Color(r-rhs.r, g-rhs.g, b-rhs.b); }
        inline Color operator+(const Color rhs) const { return Color(r+rhs.r, g+rhs.g, b+rhs.b); }
        inline Color operator/(const float factor) const { return Color(r/factor, g/factor, b/factor); }
        inline Color operator*(const float factor) const { return Color(r*factor, g*factor, b*factor); }
        inline bool operator==(const Color& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b; }
        inline bool operator!=(const Color& rhs) const { return r != rhs.r || g != rhs.g || b != rhs.b; }
};

#endif