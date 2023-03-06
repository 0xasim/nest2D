#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <typeinfo>

#include <libnest2d/libnest2d.hpp>
#include <array>

#include "../tools/printer_parts.hpp"
#include "../tools/svgtools.hpp"


namespace py = pybind11;

using Point = libnest2d::Point;
using Box = libnest2d::Box;
using Item = libnest2d::Item;
using PackGroup = libnest2d::PackGroup;
using SVGWriter = libnest2d::svg::SVGWriter<libnest2d::PolygonImpl>;

PYBIND11_MODULE(nest2D, m)
{
    m.doc() = "2D irregular bin packaging and nesting for python";

    py::class_<Point>(m, "Point", "2D Point")
        .def(py::init<int, int>(),  py::arg("x"), py::arg("y"))
        //.def_property_readonly("x", &Point::X)
        .def_property_readonly("x", [](const Point &p) { return p.X; })
        .def_property_readonly("y", [](const Point &p) { return p.Y; })
        .def("__repr__",
             [](const Point &p) {
                 std::string r("Point(");
                 r += boost::lexical_cast<std::string>(p.X);
                 r += ", ";
                 r += boost::lexical_cast<std::string>(p.Y);
                 r += ")";
                 return r;
             }
        )
        .def("__eq__",
            [](const Point &p, const Point & q) {
                return p == q;
            }
        );

    // see lib/libnest2d/include/libnest2d/geometry_traits.hpp
    py::class_<Box>(m, "Box", "2D Box point pair")
        //.def(py::init<int, int>())
        // custom constructor to define box center
        .def(py::init([](int x, int y) {
            return std::unique_ptr<Box>(new Box(x, y, {x/2, y/2}));
        }))
        ;

    // Item is a shape defined by points
    // see lib/libnest2d/include/libnest2d/nester.hpp
    py::class_<Item>(m, "Item", "An item to be placed on a bin.")
        .def(py::init<std::vector<Point>>())
        .def("__repr__",
             [](const Item &i) {
                 std::string r("Item(area: ");
                 r += boost::lexical_cast<std::string>(i.area());
                 r += ", bin_id: ";
                 r += boost::lexical_cast<std::string>(i.binId());
                 r += ", vertices: ";
                 r += boost::lexical_cast<std::string>(i.vertexCount());
                 r += ")";
                 return r;
             }
        )
        .def("translation", [](Item & ite) {
             auto tsh = ite.transformedShape();
             auto ret = libnest2d::shapelike::contour(tsh);
             // py::print("size:", sizeof(ret));
             // py::print("[0]size:", sizeof(ret[0]));
             // py::print("contours:", ret);
             // py::list cpp_array_to_python_list(const std::vector<std::vector<long long>>& array) {
             py::list result;
             for (Point &pt : ret) {
                 const auto row = {pt.X, pt.Y};
                 py::list py_row;
                 for (const auto& element : row) {
                     py_row.append(element);
                 }
                 result.append(py_row);
             }
             return result;
         })
        .def("get_rotation",
             [](const Item &i) {
                 double rot;
                 rot = i.rotation();
                 return rot;
             }
        )
        .def("toString",
             [](const Item &i) {
                 std::string r;
                 r = i.toString();
                 return r;
             }
        )
        .def("isFixed", [](Item &i) {
                return i.isFixed();
            }
         );

    // The nest function takes two parameters input and box
    // see lib/libnest2d/include/libnest2d/libnest2d.hpp
    m.def("nest", [](std::vector<Item>& input, const Box& box, int& distance, bool rotation, int& corner) {
            using namespace libnest2d;
            // sucks overlaps
            // NestConfig<NfpPlacer, DJDHeuristic> cfg;

            NestConfig<NfpPlacer, FirstFitSelection> cfg;

            if(corner == 0){ cfg.placer_config.alignment = NestConfig<>::Placement::Alignment::BOTTOM_RIGHT;}
            else if (corner == 1) {cfg.placer_config.alignment = NestConfig<>::Placement::Alignment::BOTTOM_LEFT;}
            else if (corner == 2) {cfg.placer_config.alignment = NestConfig<>::Placement::Alignment::TOP_LEFT;}
            else if (corner == 3) {cfg.placer_config.alignment = NestConfig<>::Placement::Alignment::TOP_RIGHT;}

            if(rotation==0){
                cfg.placer_config.rotations = {0};
            }
            /*
            else {
                cfg.placer_config.rotations = {0.0, 0.7853981633974483, 1.5707963267948966, 2.356194490192345, 3.141592653589793, 3.9269908169872414, 4.71238898038469, 5.497787143782138};
                // cfg.placer_config.rotations = {0, 45, 90, 135, 180, 225, 270, 315};
            }*/
            // NestConfig<BottomLeftPlacer, FirstFitSelection> cfg;
            // NestConfig<NfpPlacer, DJDHeuristic> cfg;

            /*
            Radians arr[r.size()];
            for (int i=0; i<r.size(); i++){
              arr[i] = Radians[i];
              std::cout << "print "<<r[i]<<"\n";
              std::cout << r.size()<<"\n";
            }
            */
            // std::vector<Radians>
            // std::array<int,4> arr;
            // arr.assign(j,j+2);
            // int* arr = &r[0];
            // int[] *tarr = r.data();
            // std::copy(r.begin(), r.end(), arr);
            // int* ra = &r;
            // cfg.placer_config.rotations = j;
            size_t bins = libnest2d::nest(input, box, distance, cfg);

            PackGroup pgrp(bins);

            for (Item &itm : input) {
                if (itm.binId() >= 0) pgrp[size_t(itm.binId())].emplace_back(itm);
                //py::print("bin_id: ", itm.binId());
                //py::print("vertices: ", itm.vertexCount());
            }

            //return pgrp;
            // we need to convert c++ type to python using py::cast
            py::object obj = py::cast(pgrp);
            return obj;
        },
        py::arg("input"),
        py::arg("box"),
        py::arg("min_distance"),
        py::arg("rotations"),
        py::arg("corner"),
        "Nest and pack the input items into the box bin."
        )
        ;
    m.def("temp_nest", [](std::vector<Item>& input, std::vector<Item>& fixed_input,
          std::vector<std::vector<int>>& fixed_centers, const Box& box, int& distance, int& placer_id) {
            using namespace libnest2d;
            NestConfig<> cfg;

            if(placer_id==1){
                NestConfig<NfpPlacer, FirstFitSelection> cfg;
                std::cout << "selecting "<<placer_id<<" NfpPlacer, FirstFitSelection> ";
            } else if (placer_id==2){
                NestConfig<BottomLeftPlacer, FirstFitSelection> cfg;
                std::cout << "selecting "<<placer_id<<" <BottomLeftPlacer, FirstFitSelection> ";
            } else if (placer_id==3){
                NestConfig<NfpPlacer, DJDHeuristic> cfg;
                std::cout << "selecting "<<placer_id<<" <NfpPlacer, DJDHeuristic> ";
            } else if (placer_id==4){
                NestConfig<BottomLeftPlacer, DJDHeuristic> cfg;
                std::cout << "selecting "<<placer_id<<" <BottomLeftPlacer, DJDHeuristic> ";
            } else{
                NestConfig<NfpPlacer, FirstFitSelection> cfg;
                std::cout << "selecting "<<placer_id<<" NfpPlacer, FirstFitSelection> ";
            }
            cfg.placer_config.alignment = NestConfig<>::Placement::Alignment::DONT_ALIGN;
            cfg.placer_config.rotations = {0};
            // cfg.epsilon = 500e6l;

            // Fix the items in the fixed_input
            input.reserve(fixed_input.size());
            int i = 0;
            for (Item &itm : fixed_input) {
                auto cntr = fixed_centers[i];
                input.emplace_back(itm);
                Item &fixed_itm = input.back();
                const Point &c{0+cntr[0], 0+cntr[1]};
                fixed_itm.translate(c);
                fixed_itm.markAsFixedInBin(0);
                i++;
            }
            size_t bins = libnest2d::nest(input, box, distance, cfg);
            PackGroup pgrp(bins);

            for (Item &itm : input) {
                if (itm.binId() >= 0) pgrp[size_t(itm.binId())].emplace_back(itm);
                //py::print("vertices: ", itm.vertexCount());
            }
            // we need to convert c++ type to python using py::cast
            py::object obj = py::cast(pgrp);
            return obj;
        },
        py::arg("input"),
        py::arg("fixed_input"),
        py::arg("fixed_centers"),
        py::arg("box"),
        py::arg("min_distance"),
        py::arg("placer_id"),
        "Nest and pack the input items into the box bin. No rotation angles."
        );

    py::class_<SVGWriter>(m, "SVGWriter", "SVGWriter tools to write pack_group to SVG.")
        .def(py::init([]() {
            // custom constructor
            SVGWriter::Config conf;
            conf.mm_in_coord_units = libnest2d::mm();
            return std::unique_ptr<SVGWriter>(new SVGWriter(conf));
        }))
        .def("write_packgroup", [](SVGWriter & sw, const PackGroup & pgrp) {
            sw.setSize(Box(libnest2d::mm(250), libnest2d::mm(210)));  // TODO make own call
            sw.writePackGroup(pgrp);
        })
        .def("save", [](SVGWriter & sw) {
            sw.save("out");
        })
        .def("__repr__",
             [](const SVGWriter &sw) {
                 std::string r("SVGWriter(");
                 r += ")";
                 return r;
             }
        );

}
