#ifndef __ATLC3_H__
#define __ATLC3_H__
#include <vector>
#include <map>
#include <math.h>
#include "matrix.h"
#include "bitmap.h"

#define    CONDUCTOR_MINUS_ONE_V                                               5 
#define    CONDUCTOR_ZERO_V                                                   10 
#define    CONDUCTOR_PLUS_ONE_V                                               15 

#define    CONDUCTOR_FLOATING                                                 20

#define    METAL_LEFT                                                         25
#define    METAL_RIGHT                                                        30
#define    METAL_ABOVE                                                        35
#define    METAL_BELOW                                                        40
#define    METAL_BELOW_AND_LEFT                                               45
#define    METAL_BELOW_AND_RIGHT                                              50
#define    METAL_ABOVE_AND_LEFT                                               55 
#define    METAL_ABOVE_AND_RIGHT                                              60

#define    DIELECTRIC                                                         65 /* not for oddity */
#define    ORDINARY_INTERIOR_POINT                                            70

#define    TOP_LEFT_CORNER                                                    75
#define    BOTTOM_RIGHT_CORNER                                                80
#define    TOP_RIGHT_CORNER                                                   85
#define    BOTTOM_LEFT_CORNER                                                 90
#define    ORDINARY_POINT_BOTTOM_EDGE                                         95
#define    ORDINARY_POINT_TOP_EDGE                                           100
#define    ORDINARY_POINT_LEFT_EDGE                                          105
#define    ORDINARY_POINT_RIGHT_EDGE                                         110
#define    DIFFERENT_DIELECTRIC_LOCALLY                                      115

#define    DIFFERENT_DIELECTRIC_ABOVE_AND_RIGHT                              120
#define    DIFFERENT_DIELECTRIC_BELOW_AND_LEFT                               125
#define    DIFFERENT_DIELECTRIC_BELOW_AND_RIGHT                              135
#define    DIFFERENT_DIELECTRIC_VERTICALLY                                   140
#define    DIFFERENT_DIELECTRIC_HORIZONTALLY                                 145
#define    DIFFERENT_DIELECTRIC_BELOW                                        150
#define    DIFFERENT_DIELECTRIC_LEFT                                         155
#define    DIFFERENT_DIELECTRIC_RIGHT                                        160
#define    DIFFERENT_DIELECTRIC_ABOVE_AND_LEFT                               165

#define    UNDEFINED_ODDITY                                                  255


#define METAL_ER  1e9 

#define NUMBER_OF_DIELECTRICS_DEFINED 13

#define VERY_LARGE 1e15

#define EPSILON_0 8.854187817e-12
#define MU_0 M_PI * 4e-7

#define ITERATIONS                      100

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.1.0"

#define DONT_ZERO_ELEMENTS  0       
#define ZERO_ELEMENTS_FIRST 1   

#define COLOUR 0
#define MONOCHROME 1
#define MIXED      2
#define Z0                              1

#define IMAGE_FIDDLE_FACTOR 2.0

#define MAX_ER 12.0

#define Z_ODD_SINGLE_DIELECTRIC         1
#define Z_EVEN_SINGLE_DIELECTRIC        2
#define Z_ODD_MULTIPLE_DIELECTRIC       3
#define Z_EVEN_MULTIPLE_DIELECTRIC      4

#define DONT_CARE                          0
#define ODD                                1
#define EVEN                               2

class atlc3
{
private:
    struct atlc3_node
    {
        atlc3_node()
            : oddity(0)
            , cell_type(0)
            , er(0)
            , v(0)
        {
        }
        
        std::uint8_t oddity;
        std::uint8_t cell_type;
        float er;
        float v;
    };
    
    struct pixels
    {
        int red;          /* +1 V */
        int green;        /* 0 V */
        int blue;         /* -1 V */
        int white;        /* Vacuum */
        int other_colour;  /* mix of red, green and blue  */
        float epsilon;
    };
    
    struct max_values
    {
        float Ex_or_Ey_max, E_max, V_max, U_max, permittivity_max;
    };
    
    typedef matrix<atlc3_node> matrix_atlc;
    
public:
    atlc3();
    ~atlc3();
    
public:
    bool setup_arrays(const matrix_rgb& img);
    bool set_oddity_value();
    void do_fd_calculation();
    
    void set_inputfile_filename(const char *filename)
    {
        _inputfile_filename = filename;
    }
    void set_verbose_level(std::int32_t level)
    {
        _verbose_level = level;
    }
    
    void add_er(std::uint32_t color, float er)
    {
        _er_list.push_back(std::pair<std::uint32_t, float>(color, er));
    }
    
    void set_cutoff(float cutoff)
    {
        _cutoff = cutoff;
    }
    
    void set_rate_multiplier(float rate_multiplier)
    {
        _r = rate_multiplier;
    }
    
    void enable_binary_imageQ(bool b)
    {
        _write_binary_field_imagesQ = b;
    }
    
    void enable_bitmap_imageQ(bool b)
    {
        _write_bitmap_field_imagesQ = b;
    }
    
    void setup_mask(bool ignore_dielectric);
    float finite_difference_single_threaded();
    void update_voltage_array(int nmax, int imin, int imax, int jmin, int jmax);
    void update_voltage_array_fast(int nmax);
    
    void swap_conductor_voltages();
    
    inline float find_energy_per_metre(int w, int h);
    inline float find_Ex(int i/*col*/, int j);
    inline float find_Ey(int i, int j);
    float find_E(int w, int h);
    
    void cvt_rgb(matrix_rgb& img);
    void cvt_rgb_er(matrix_rgb& img);
    
    void print_data_for_two_conductor_lines();
    void print_data_for_directional_couplers();
    void write_fields(std::string name = "", std::int32_t zero_elementsQ = ZERO_ELEMENTS_FIRST);
    void find_maximum_values(struct max_values *maximum_values, int zero_elementsQ);
    void calculate_colour_data(float x, float xmax, int w, int h, int image_type,
                unsigned char *red, unsigned char *green, unsigned char *blue, float image_fiddle_factor);
    
private:
    matrix_atlc _mat;
    bool _coupler;
    std::uint32_t _dielectrics_to_consider_just_now;
    
    std::vector<std::pair<std::uint32_t, float> > _er_list; //命令行用户输入的电介质常熟
    std::map<std::uint32_t, pixels> _er_bitmap;
    
    std::string _inputfile_filename;
    std::uint8_t _verbose_level;
    
    struct max_values _maximum_values;
    bool _write_binary_field_imagesQ;
    bool _write_bitmap_field_imagesQ;
    std::uint32_t _display;
    
    float _r;
    float _cutoff;
    float _C_vacuum;
    float _C;
    float _L_vacuum; /* Same as L in *ALL* cases */
    float _Zo_vacuum;  /* Standard formaul for Zo */
    float _Er;
    float _Zo;
    float _Zodd;
    float _Codd_vacuum;
    float _Codd;
    float _Lodd_vacuum; /* Same as L in *ALL* cases */
    float _Zodd_vacuum;  /* Standard formaul for Zodd */
    float _Er_odd;
    
    float _Ceven;
    float _Ceven_vacuum;
    float _Leven_vacuum;
    float _Er_even;
    float _Zeven;
    float _Zcomm;
    
    float _Zdiff;
    float _Zeven_vacuum;
    
    float _velocity;
    float _velocity_factor;
    
    float _velocity_odd;
    float _velocity_factor_odd;
    
    float _velocity_even;
    float _velocity_factor_even;
};

#endif