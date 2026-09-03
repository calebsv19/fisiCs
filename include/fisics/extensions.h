#ifndef FISICS_EXTENSIONS_H
#define FISICS_EXTENSIONS_H

/*
 * Portable authoring spellings for fisiCs physics-units attributes.
 *
 * fisiCs defines __FISICS__ before preprocessing. Other C compilers see
 * these annotations as empty macros, so the same source can participate in
 * strict control-compiler builds without unknown-attribute warnings.
 */
#ifdef __FISICS__
#define FISICS_DIM(value) [[fisics::dim(value)]]
#define FISICS_UNIT(value) [[fisics::unit(value)]]
#else
#define FISICS_DIM(value)
#define FISICS_UNIT(value)
#endif

#endif
