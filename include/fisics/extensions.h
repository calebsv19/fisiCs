#ifndef FISICS_EXTENSIONS_H
#define FISICS_EXTENSIONS_H

/*
 * Portable authoring spellings for fisiCs extension attributes.
 *
 * fisiCs defines __FISICS__ before preprocessing. Other C compilers see
 * these annotations as empty macros, so the same source can participate in
 * strict control-compiler builds without unknown-attribute warnings.
 */
#ifdef __FISICS__
#define FISICS_ENERGY_FORM(value) [[fisics::energy_form(value)]]
#define FISICS_ENERGY_TRANSFER(value) [[fisics::energy_transfer(value)]]
#define FISICS_FRAME(value) [[fisics::frame(value)]]
#define FISICS_COMPONENT_AXIS(value) [[fisics::component_axis(value)]]
#define FISICS_FRAME_TRANSFORM(value) [[fisics::frame_transform(value)]]
#define FISICS_POINT_ID(value) [[fisics::point_id(value)]]
#define FISICS_MOMENT_REFERENCE(value) [[fisics::moment_reference(value)]]
#define FISICS_APPLIED_AT(value) [[fisics::applied_at(value)]]
#define FISICS_BETWEEN_POINTS(from,to) [[fisics::between_points(from,to)]]
#define FISICS_QUANTITY_KIND(value) [[fisics::quantity_kind(value)]]
#define FISICS_PHYSICAL_ORIGIN(value) [[fisics::physical_origin(value)]]
#define FISICS_REPRESENTATION(value) [[fisics::representation(value)]]
#define FISICS_PHYSICAL_OPERATION(value) [[fisics::physical_operation(value)]]
#define FISICS_DIM(value) [[fisics::dim(value)]]
#define FISICS_UNIT(value) [[fisics::unit(value)]]
#define FISICS_ID(value) [[fisics::id(value)]]
#define FISICS_DOMAIN(value) [[fisics::domain(value)]]
#define FISICS_ROLE(value) [[fisics::role(value)]]
#define FISICS_PRODUCES(value) [[fisics::produces(value)]]
#define FISICS_CONSUMES(value) [[fisics::consumes(value)]]
#else
#define FISICS_ENERGY_FORM(value)
#define FISICS_ENERGY_TRANSFER(value)
#define FISICS_FRAME(value)
#define FISICS_COMPONENT_AXIS(value)
#define FISICS_FRAME_TRANSFORM(value)
#define FISICS_POINT_ID(value)
#define FISICS_MOMENT_REFERENCE(value)
#define FISICS_APPLIED_AT(value)
#define FISICS_BETWEEN_POINTS(from,to)
#define FISICS_QUANTITY_KIND(value)
#define FISICS_PHYSICAL_ORIGIN(value)
#define FISICS_REPRESENTATION(value)
#define FISICS_PHYSICAL_OPERATION(value)
#define FISICS_DIM(value)
#define FISICS_UNIT(value)
#define FISICS_ID(value)
#define FISICS_DOMAIN(value)
#define FISICS_ROLE(value)
#define FISICS_PRODUCES(value)
#define FISICS_CONSUMES(value)
#endif

#endif
