// Writes an SVG picture of an ellipsoid using only arda_core, so the math can be
// eyeballed in a browser before a real renderer exists.
//
//   arda_viz [output.svg]
//
// Left panel:  orthographic 3D view with a lat/lon grid, geodetic normals and a
//              ComputeCurve arc (hidden parts drawn faint).
// Right panel: meridian cross-section comparing geodetic vs geocentric normals
//              and ScaleToGeodeticSurface vs ScaleToGeocentricSurface.
//
// The ellipsoid is deliberately squashed; WGS84's flattening is invisible at this scale.

#include <arda/core/Trig.h>
#include <arda/core/geometry/Ellipsoid.h>

#include <cstdio>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using arda::core::Geodetic2D;
using arda::core::Trig;
using arda::core::Vector3;
using arda::core::geometry::Ellipsoid;

namespace {
    struct Point2 {
        double x;
        double y;
    };

    // Orthographic camera looking at the origin from direction `toViewer`.
    class OrthoView {
    public:
        OrthoView(const Vector3<double>& toViewer, Point2 center, double scale)
            : m_toViewer(toViewer.Normalize()), m_center(center), m_scale(scale) {
            m_right = Vector3<double>::UnitZ().Cross(m_toViewer).Normalize();
            m_up = m_toViewer.Cross(m_right);
        }

        Point2 Project(const Vector3<double>& p) const {
            return {m_center.x + m_scale * p.Dot(m_right), m_center.y - m_scale * p.Dot(m_up)};
        }

        // On a convex surface a point faces the viewer when its normal does.
        bool Faces(const Vector3<double>& normal) const { return normal.Dot(m_toViewer) > 0.0; }

    private:
        Vector3<double> m_toViewer;
        Vector3<double> m_right;
        Vector3<double> m_up;
        Point2 m_center;
        double m_scale;
    };

    class Svg {
    public:
        void Line(Point2 a, Point2 b, const std::string& cssClass) {
            m_body << "<line x1='" << a.x << "' y1='" << a.y << "' x2='" << b.x << "' y2='" << b.y
                   << "' class='" << cssClass << "'/>\n";
        }

        void Circle(Point2 c, double r, const std::string& cssClass) {
            m_body << "<circle cx='" << c.x << "' cy='" << c.y << "' r='" << r << "' class='" << cssClass << "'/>\n";
        }

        void Text(Point2 p, const std::string& text, const std::string& cssClass = "label") {
            m_body << "<text x='" << p.x << "' y='" << p.y << "' class='" << cssClass << "'>" << text << "</text>\n";
        }

        void Save(const std::string& path, int width, int height) const {
            std::ofstream out(path);
            if (!out) {
                throw std::runtime_error("cannot open " + path);
            }
            out << "<svg xmlns='http://www.w3.org/2000/svg' width='" << width << "' height='" << height
                << "' viewBox='0 0 " << width << ' ' << height << "'>\n"
                << "<style>\n"
                << "  svg { background: #0b1020; font-family: sans-serif; }\n"
                << "  line { stroke-linecap: round; }\n"
                << "  .grid { stroke: #6f86b8; stroke-width: 1; }\n"
                << "  .grid-hidden { stroke: #6f86b8; stroke-width: 0.6; opacity: 0.18; }\n"
                << "  .equator { stroke: #c9d6f5; stroke-width: 1.8; }\n"
                << "  .curve { stroke: #ffb347; stroke-width: 2.5; }\n"
                << "  .curve-hidden { stroke: #ffb347; stroke-width: 1.5; opacity: 0.3; }\n"
                << "  .curve-pt { fill: #ffb347; }\n"
                << "  .normal { stroke: #5ee0a0; stroke-width: 1.6; }\n"
                << "  .geocentric { stroke: #ff6b6b; stroke-width: 1.4; stroke-dasharray: 4 3; }\n"
                << "  .outline { stroke: #c9d6f5; stroke-width: 1.8; }\n"
                << "  .axis { stroke: #3a4568; stroke-width: 1; }\n"
                << "  .query { fill: #ffffff; }\n"
                << "  .geodetic-pt { fill: #5ee0a0; }\n"
                << "  .geocentric-pt { fill: #ff6b6b; }\n"
                << "  .label { fill: #c9d6f5; font-size: 13px; }\n"
                << "  .title { fill: #ffffff; font-size: 17px; font-weight: 600; }\n"
                << "</style>\n"
                << m_body.str() << "</svg>\n";
        }

    private:
        std::ostringstream m_body;
    };

    Vector3<double> Surface(const Ellipsoid& ellipsoid, double latDeg, double lonDeg) {
        return ellipsoid.ToCartesian(Trig::DegreesToRadians(Geodetic2D(latDeg, lonDeg)));
    }

    // Draws a polyline, styling each segment by whether it faces the viewer.
    void DrawSurfacePolyline(Svg& svg, const OrthoView& view, const Ellipsoid& ellipsoid,
                             const std::vector<Vector3<double>>& points,
                             const std::string& frontClass, const std::string& backClass) {
        for (size_t i = 1; i < points.size(); ++i) {
            const Vector3<double> mid = (points[i - 1] + points[i]) * 0.5;
            const bool front = view.Faces(ellipsoid.GeodeticSurfaceNormal(ellipsoid.ScaleToGeocentricSurface(mid)));
            svg.Line(view.Project(points[i - 1]), view.Project(points[i]), front ? frontClass : backClass);
        }
    }

    void DrawGlobeView(Svg& svg, const Ellipsoid& ellipsoid) {
        const OrthoView view(Surface(Ellipsoid::UnitSphere(), 25.0, 30.0), {300.0, 330.0}, 210.0);
        svg.Text({40.0, 40.0}, "3D view: lat/lon grid, geodetic normals, ComputeCurve", "title");

        for (int lat = -75; lat <= 75; lat += 15) {
            std::vector<Vector3<double>> parallel;
            for (int lon = -180; lon <= 180; lon += 3) {
                parallel.push_back(Surface(ellipsoid, lat, lon));
            }
            DrawSurfacePolyline(svg, view, ellipsoid, parallel, lat == 0 ? "equator" : "grid", "grid-hidden");
        }
        for (int lon = -180; lon < 180; lon += 15) {
            std::vector<Vector3<double>> meridian;
            for (int lat = -90; lat <= 90; lat += 3) {
                meridian.push_back(Surface(ellipsoid, lat, lon));
            }
            DrawSurfacePolyline(svg, view, ellipsoid, meridian, "grid", "grid-hidden");
        }

        for (int lat = -60; lat <= 60; lat += 30) {
            for (int lon = -30; lon <= 90; lon += 30) {
                const Vector3<double> p = Surface(ellipsoid, lat, lon);
                const Vector3<double> n = ellipsoid.GeodeticSurfaceNormal(p);
                if (view.Faces(n)) {
                    svg.Line(view.Project(p), view.Project(p + n * 0.18), "normal");
                }
            }
        }

        const Vector3<double> p = Surface(ellipsoid, -20.0, -40.0);
        const Vector3<double> q = Surface(ellipsoid, 55.0, 110.0);
        const auto curve = ellipsoid.ComputeCurve(p, q, Trig::DegreesToRadians(5.0));
        DrawSurfacePolyline(svg, view, ellipsoid, curve, "curve", "curve-hidden");
        for (const auto& point : curve) {
            if (view.Faces(ellipsoid.GeodeticSurfaceNormal(point))) {
                svg.Circle(view.Project(point), 2.5, "curve-pt");
            }
        }
        svg.Text({40.0, 600.0}, "orange: ComputeCurve (-20,-40) to (55,110), 5 deg granularity, "
                                + std::to_string(curve.size()) + " points");
        svg.Text({40.0, 620.0}, "green: GeodeticSurfaceNormal");
    }

    void DrawCrossSection(Svg& svg, const Ellipsoid& ellipsoid) {
        // Looking down -Y, so screen x = world X and screen y = world Z.
        const OrthoView view(Vector3<double>(0.0, -1.0, 0.0), {870.0, 320.0}, 200.0);
        svg.Text({660.0, 40.0}, "Meridian cross-section (lon = 0)", "title");

        svg.Line(view.Project({-1.3, 0.0, 0.0}), view.Project({1.3, 0.0, 0.0}), "axis");
        svg.Line(view.Project({0.0, 0.0, -1.1}), view.Project({0.0, 0.0, 1.1}), "axis");

        const double a = ellipsoid.GetSemiMajorAxis();
        const double c = ellipsoid.GetSemiMinorAxis();
        for (int deg = 0; deg < 360; deg += 2) {
            const double t0 = Trig::DegreesToRadians(deg);
            const double t1 = Trig::DegreesToRadians(deg + 2);
            svg.Line(view.Project({a * std::cos(t0), 0.0, c * std::sin(t0)}),
                     view.Project({a * std::cos(t1), 0.0, c * std::sin(t1)}), "outline");
        }

        // Normals at a few geodetic latitudes: green (geodetic) vs red (geocentric).
        const Vector3<double> origin = Vector3<double>::Zero();
        for (int lat = 15; lat <= 75; lat += 30) {
            const Vector3<double> p = Surface(ellipsoid, lat, 0.0);
            svg.Line(view.Project(origin), view.Project(p * 1.25), "geocentric");
            svg.Line(view.Project(p), view.Project(p + ellipsoid.GeodeticSurfaceNormal(p) * 0.35), "normal");
            svg.Circle(view.Project(p), 3.0, "curve-pt");
            const Point2 at = view.Project(p + ellipsoid.GeodeticSurfaceNormal(p) * 0.38);
            svg.Text({at.x + 4.0, at.y}, std::to_string(lat) + " deg");
        }

        // Project an off-surface point both ways.
        const Vector3<double> query(0.95, 0.0, -0.95);
        const Vector3<double> geodetic = ellipsoid.ScaleToGeodeticSurface(query);
        const Vector3<double> geocentric = ellipsoid.ScaleToGeocentricSurface(query);
        svg.Line(view.Project(query), view.Project(geodetic), "normal");
        svg.Line(view.Project(origin), view.Project(query), "geocentric");
        svg.Circle(view.Project(query), 4.0, "query");
        svg.Circle(view.Project(geodetic), 4.0, "geodetic-pt");
        svg.Circle(view.Project(geocentric), 4.0, "geocentric-pt");
        const Point2 q = view.Project(query);
        svg.Text({q.x + 8.0, q.y + 4.0}, "query point");

        const Vector3<double> radii = ellipsoid.GetRadii();
        std::ostringstream caption;
        caption << "ellipsoid radii (" << radii.X() << ", " << radii.Y() << ", " << radii.Z() << ")";
        svg.Text({660.0, 580.0}, caption.str());
        svg.Text({660.0, 600.0}, "green: geodetic normal / ScaleToGeodeticSurface");
        svg.Text({660.0, 620.0}, "red dashed: geocentric direction / ScaleToGeocentricSurface");
    }
}

int main(int argc, char** argv) {
    try {
        const std::string path = argc > 1 ? argv[1] : "ellipsoid.svg";
        const Ellipsoid ellipsoid(1.0, 1.0, 0.6);

        Svg svg;
        DrawGlobeView(svg, ellipsoid);
        DrawCrossSection(svg, ellipsoid);
        svg.Save(path, 1200, 650);

        std::printf("wrote %s\n", path.c_str());
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
