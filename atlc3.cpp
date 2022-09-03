#include "atlc3.h"

//static const char *names[] = {"Vacuum", "Air", "PTFE", "duroid_5880", "Polyethelene", "Polystyrene", "PVC", "Epoxy_resin", "FR4 PCB", "Fibreglass_PCB", "duroid_6006", "duroid_6010", "one_hundred"};
static float Ers[] = {1.0, 1.0006, 2.1, 2.2, 2.33, 2.5, 3.3, 3.335, 3.7, 4.8, 6.15, 10.2, 100.0};
static std::uint32_t colours[]={0xffffff, 0xffcaca, 0x8235ef, 0x8e8e8e, 0xff00ff, 0xffff00, 0xefcc1a, 0xbc7f60, 0xdff788, 0x1aefb3, 0x696969, 0xdcdcdc, 0xd5a04d};

atlc3::atlc3()
    : _coupler(false)
    , _verbose_level(3)
{
    _r = 1.9;
    _cutoff = 0.0001;
    
    _write_binary_field_imagesQ = false;
    _write_bitmap_field_imagesQ = true;
    _inputfile_filename = "test.bmp";
    
    _er_list.push_back(std::pair<std::uint32_t, float>(0x0f0ed8, 3.800000));
    _er_list.push_back(std::pair<std::uint32_t, float>(0x0f1004, 4.100000));
    _er_list.push_back(std::pair<std::uint32_t, float>(0x0f1108, 4.360000));
}


atlc3::~atlc3()
{
}



bool atlc3::setup_arrays(const matrix_rgb & img)
{
    struct pixels pixels_found;
    bool conductor_found;
    int conductors = 0;
    
    
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint32_t colour_mixture;
    
    bool dielectric_found;
    //int new_colour_in_image;
    //data->dielectrics_in_bitmap=0;
    pixels_found.red = 0;
    pixels_found.green = 0;
    pixels_found.blue = 0;
    pixels_found.white = 0;
    pixels_found.other_colour = 0;
    
    
    _mat.create(img.rows(), img.cols());
    
    for (std::int32_t row = 0; row < img.rows(); row++)
    {
        for (std::int32_t col = 0; col < img.cols(); col++)
        {
            const rgb& pixel = img.at(row, col);
            atlc3_node& node = _mat.at(row, col);
            
            blue = pixel.b;
            green = pixel.g;
            red = pixel.r;
            
            dielectric_found = false;
            conductor_found = false;
            
            colour_mixture = 256 * 256 * red + 256 * green + blue;
            
            if (colour_mixture == 0xff0000) /* +1V red */
            {
                node.cell_type = CONDUCTOR_PLUS_ONE_V;
                node.v = 1.0;
                node.er = METAL_ER;
                
                conductor_found = true;
                pixels_found.red++;
            }
            else if (colour_mixture == 0x00ff00) /* 0v green */
            {
                node.cell_type = CONDUCTOR_ZERO_V;
                node.v = 0.0;
                node.er = METAL_ER;
                
                conductor_found = true;
                pixels_found.green++;
            }
            else if (colour_mixture == 0x0000ff) /* -1V blue */
            {
                node.cell_type = CONDUCTOR_MINUS_ONE_V;
                node.v = -1.0;
                node.er = METAL_ER;
                
                conductor_found = true;
                pixels_found.blue++;
                _coupler = true;
            }
            else /* A dielectric */
            {
                if (colour_mixture == 0xffffff) /* White */
                {
                    pixels_found.white++; /* Vacuum */
                }
                else
                {
                    pixels_found.other_colour++; /* Some other dielectric */
                }
                
                node.cell_type = DIELECTRIC;
                node.v = 0.0;
                
                for(std::uint32_t z = 0; z < NUMBER_OF_DIELECTRICS_DEFINED; ++z)
                {
                    /* Check to see if the colour is one of the 10 known
                    about, without any need to define on the command line
                    */
                    if (colour_mixture == colours[z])
                    {
                        node.er = Ers[z];
                        dielectric_found = true;
                        
                        if(z != 0)
                        {
                            //non_vacuum_found = true;
                            //data->found_this_dielectric=Ers[z];
                        }
                    }
                }
                
                for(std::uint32_t i = 0; i < _er_list.size(); ++i)
                {
                    if (_er_list[i].first == colour_mixture)
                    {
                        node.er = _er_list[i].second;
                        
                        dielectric_found = true;
                        //non_vacuum_found = true;
                        //data->found_this_dielectric = _er_list[i].second;
                    }
                }
            }
            
            
            if ((dielectric_found == false) && (conductor_found == false))
            {
                fprintf(stderr, "Error#7: The colour r=0x%x g=0x%x b=0x%x (0x%02x%02x%02x) exists at pixel %d,%d.\n", red, green, blue, red, green, blue, col, row);
                fprintf(stderr, "However, atlc does not know how to interpret this colour. This is not a\n");
                fprintf(stderr, "conductor (pure red, green or blue), nor is it one of the %d dielectrics that\n", NUMBER_OF_DIELECTRICS_DEFINED);
                fprintf(stderr, "are predefined in Erdata.h, nor is a corresponding dielectric constant defined\n");
                fprintf(stderr, "on the command line with the -d option. Sometimes this occurs when a\n");
                fprintf(stderr, "graphics package is used to make the bitmap, but it performs some form of\n");
                fprintf(stderr, "anti-aliasing or smooting. If this is the case, redraw the image turning such\n");
                fprintf(stderr, "options off. If this is not the case then re-run atlc adding the -d option\n");
                fprintf(stderr, "to define the relative permittivity of the dielectric\n\n");
                fprintf(stderr, "e.g. atlc -d %02x%02x%02x=1.9 xx.bmp \n\n", red, green, blue);
                fprintf(stderr, "if this colour has a permittivity of 1.9. If there are multiple undefined\n");
                fprintf(stderr, "dielectrics, then there will need to be multiple copies of the -d option given.\n");
                return false;
            }
            
            /* We need to keep a record of the number of dielectrics in the image,
            and determine if they are defined on the command line, or if they are
            defined on in the header file. */
            if (dielectric_found == true)
            {
                if (_er_bitmap.count(colour_mixture) == 0)
                {
                    pixels pix;
                    
                    pix.other_colour = colour_mixture;
                    pix.red = red;
                    pix.green = green;
                    pix.blue = blue;
                    pix.epsilon = node.er;
                    
                    _er_bitmap.emplace(colour_mixture, pix);
                }
            } /* end of if dielctric found */
        }
    }
    
    /* The following prints a lot of data that is generally not wanted
    but is when finding statistics of performance etc. */
    if (_verbose_level >= 3)
    {
        printf("Red (+1 V conductor) pixels found   =        %8d \n", pixels_found.red);
        printf("Green (0 V conductor) pixels found  =        %8d \n", pixels_found.green);
        printf("Blue  (-1 V conductor) pixels found =        %8d \n", pixels_found.blue);
        printf("White (vacuum dielectric) pixels found =     %8d \n", pixels_found.white);
        printf("Others (not vacuum dielectic) pixels found = %8d \n", pixels_found.other_colour);
        printf("Width  =                                     %8d \n", img.cols());
        printf("Height =                                     %8d \n", img.rows());
        printf("Pixels =                                     %8d \n", img.cols() * img.rows());
        printf("Number of Dielectrics found =                %8d \n", (std::int32_t)_er_bitmap.size());
        
        std::uint32_t non_metallic_pixels = img.cols() * img.rows() - pixels_found.red - pixels_found.green - pixels_found.blue;
        
        printf("Number of non-metallic pixels =              %8d \n", non_metallic_pixels);
        
        //printf("filename =             %30s \n", inputfile_name);
        
        if (pixels_found.red > 0)
        {
            conductors += 1;
        }
        
        if (pixels_found.green > 0 )
        {
            conductors += 1;
        }
        
        if (pixels_found.blue > 0 )
        {
            conductors += 1;
        }
        printf("Number of Conductors  = %d \n", conductors);
    }
    
    /* The following should not be necessary, but may be as a test */
    /* I'd like to Miguel Berg for noticcing a servere bug, where the
    indeces of w and h were transposed, leading to crashes on Windoze
    XP */
    
    
    for (std::int32_t row = 0; row < _mat.rows(); row++)
    {
        for (std::int32_t col = 0; col < _mat.cols(); col++)
        {
            atlc3_node& node = _mat.at(row, col);
            if((node.v > 1.0) || (node.v < -1.0))
            {
                fprintf(stderr,"Sorry, something is wrong Vij[%d][%d]=%f in %s %d\n", col, row, node.v, __FILE__,__LINE__);
            }
        }
    }
    
    /* Check two conductors and not next to each other, creating a short */
    //check_for_shorts();
    return true;
}


bool atlc3::set_oddity_value()
{

    /* Its easier to set the endge values first, as it
    reduces the amount of checking needed in the main body.
    There are only 11 cases here - 3 types of metal, 
    the four corners, and the four sides */

    for (std::int32_t row = 0; row < _mat.rows(); row++)
    {
        for (std::int32_t col = 0; col < _mat.cols(); col++)
        {
            atlc3_node& node = _mat.at(row, col);
            node.oddity = UNDEFINED_ODDITY;  /* Stick it to some underfined status  */
            std::uint8_t c = node.cell_type;   /* Cell type at point (i,j) */
            
            /* The 3 metal cases can be quickly checked and the 
            oddity value assigned to a fixed value depending on
            whether it's -1, 0 or +1 V */

            if (c <= CONDUCTOR_PLUS_ONE_V) /* a metal */
            {
                node.oddity = c;
            }
            /* Now do the 4 courners */
            else if ((col == 0) && (row == _mat.rows() - 1))
            {
                node.oddity = BOTTOM_LEFT_CORNER;
            }
            else if ((col == _mat.cols() - 1) && (row == _mat.rows() - 1))
            {
                node.oddity = BOTTOM_RIGHT_CORNER;
            }
            else if ((col == 0) && (row == 0))
            {
                node.oddity = TOP_LEFT_CORNER;
            }
            else if((col == _mat.cols() - 1) && (row == 0))
            {
                node.oddity = TOP_RIGHT_CORNER;
            }
            /* Now the four edges */ 
            else if (col == 0)
            {
                node.oddity = ORDINARY_POINT_LEFT_EDGE;
            }
            else if (row == 0)
            {
                node.oddity = ORDINARY_POINT_TOP_EDGE;
            }
            else if (row == _mat.rows() - 1)
            {
                node.oddity = ORDINARY_POINT_BOTTOM_EDGE;
            }
            else if (col == _mat.cols() - 1)
            {
                node.oddity = ORDINARY_POINT_RIGHT_EDGE;
            }
            else if ((col == 0 || col == _mat.cols() - 1 || row == 0 || row == _mat.rows() - 1) && (node.oddity == UNDEFINED_ODDITY))
            {
                fprintf(stderr,"Internal error: one of the edge points (%d,%d) is still undefined\n", col, row);
                fprintf(stderr, "ZZZZZZZZZZZZZ width=%d height=%d\n", _mat.cols(), _mat.rows());
                fprintf(stderr,"Error set_oddity_value.c\n");
                return false;
            }
        }
    }
    
  /* With the oddity values of all the edges now know, the centre
  values can be attempted */

    for (std::int32_t i = 1; i < _mat.cols() - 1; ++i)
    {
        for(std::int32_t j = 1; j < _mat.rows() - 1; ++j)
        {
            std::uint8_t c = _mat.at(j, i).cell_type;           /* Cell type at point (i,j) */
            std::uint8_t cl = _mat.at(j, i - 1).cell_type;      /* Cell type to left of point (i,j) */
            std::uint8_t cr = _mat.at(j, i + 1).cell_type;      /* Cell type to right of point (i,j) */
            std::uint8_t ca = _mat.at(j - 1, i).cell_type;      /* Cell type above point (i,j) */
            std::uint8_t cb = _mat.at(j + 1, i).cell_type;      /* Cell type below point (i,j) */

            float ERa = _mat.at(j - 1, i).er;
            float ERb = _mat.at(j + 1, i).er;
            float ERl = _mat.at(j, i - 1).er;
            float ERr = _mat.at(j, i + 1).er;
            //float er = _mat.at(j, i).er;


            atlc3_node& node = _mat.at(j, i);
            
            /* If the conductor is at a fixed v, it must stay there
            so there is nothing to do with it */

            if (c == CONDUCTOR_ZERO_V)
            {
                node.oddity = CONDUCTOR_ZERO_V;
            }
            else if (c== CONDUCTOR_PLUS_ONE_V)
            {
                node.oddity = CONDUCTOR_PLUS_ONE_V;
            }
            else if (c == CONDUCTOR_MINUS_ONE_V)
            {
                node.oddity = CONDUCTOR_MINUS_ONE_V; 
            }
            else if (cr <= CONDUCTOR_PLUS_ONE_V && cb <= CONDUCTOR_PLUS_ONE_V)
            {
                node.oddity = METAL_BELOW_AND_RIGHT;
            }
            else if (cr <= CONDUCTOR_PLUS_ONE_V && ca <= CONDUCTOR_PLUS_ONE_V)
            {
                node.oddity = METAL_ABOVE_AND_RIGHT;
            }
            else if (cl <= CONDUCTOR_PLUS_ONE_V && cb <= CONDUCTOR_PLUS_ONE_V)
            {
                node.oddity = METAL_BELOW_AND_LEFT;
            }
            else if (cl <= CONDUCTOR_PLUS_ONE_V && ca <= CONDUCTOR_PLUS_ONE_V)
            {
                node.oddity = METAL_ABOVE_AND_LEFT;
            }
            else if (ca <= CONDUCTOR_PLUS_ONE_V)
            {
                node.oddity = METAL_ABOVE;
            }
            else if (cb <= CONDUCTOR_PLUS_ONE_V)
            {
                node.oddity= METAL_BELOW;
            }
            else if (cl <= CONDUCTOR_PLUS_ONE_V)
            {
                node.oddity = METAL_LEFT;
            }
            else if ( cr <= CONDUCTOR_PLUS_ONE_V )
            {
                node.oddity = METAL_RIGHT;
            }
            else if (ERb != ERa)
            {
                node.oddity = DIFFERENT_DIELECTRIC_VERTICALLY; 
            }
            else if (ERl != ERr)
            {
                node.oddity= DIFFERENT_DIELECTRIC_HORIZONTALLY;      
            }
/*
      else if ( ERa != er  && ERr != er)
	node.oddity= DIFFERENT_DIELECTRIC_ABOVE_AND_RIGHT; 

      else if ( ERa != er  && ERl != er)
	node.oddity= DIFFERENT_DIELECTRIC_ABOVE_AND_LEFT; 

      else if ( ERb != er  && ERl != er)
	node.oddity= DIFFERENT_DIELECTRIC_BELOW_AND_LEFT; 

      else if ( ERa != er )
	node.oddity= DIFFERENT_DIELECTRIC_ABOVE; 

      else if ( ERb != er )
	node.oddity= DIFFERENT_DIELECTRIC_BELOW; 
      else if ( ERl != er )
	node.oddity= DIFFERENT_DIELECTRIC_LEFT; 

      else if ( ERr != er )
	node.oddity= DIFFERENT_DIELECTRIC_RIGHT; 
*/
            else
            {
                node.oddity = ORDINARY_INTERIOR_POINT;
            }
        }/* end of for i=0 to width-1 */
    } /* end of for j= 0 to height-1 */
    return true;
}


void atlc3::do_fd_calculation()
{
    float capacitance_old;
    float capacitance;
    float velocity_of_light_in_vacuum;
    std::uint32_t count = 0;
  
    float relative_permittivity;
    float C_non_vacuum = 0;
    float relative_permittivity_odd;
    float relative_permittivity_even;
    
    (void)relative_permittivity;
    (void)relative_permittivity_odd;
    (void)relative_permittivity_even;
    
    velocity_of_light_in_vacuum = 1.0/(sqrt(MU_0 * EPSILON_0)); /* around 3x10^8 m/s */
    
    /* The following 10 lines are for a single dielectric 2 conductor line */
    if (!_coupler)
    {
        if(_verbose_level >= 2)
        {
            printf("Solving assuming a vacuum dielectric\n");
        }
        
        capacitance = VERY_LARGE; /* Can be anything large */
        
        _dielectrics_to_consider_just_now = 1;
        //setup_mask(true);
        
        do /* Start a finite calculation */
        {
            capacitance_old = capacitance;

            capacitance = finite_difference_single_threaded();

            _C_vacuum = capacitance;
            _C = capacitance;
            _L_vacuum = MU_0 * EPSILON_0 / capacitance; /* Same as L in *ALL* cases */
            _Zo_vacuum = sqrt(_L_vacuum / _C_vacuum);  /* Standard formaul for Zo */
            _C = capacitance; 
            
            if (_er_bitmap.size() == 1) /* Just get C by simple scaling of Er */
            {
                _Er = _er_bitmap.begin()->second.epsilon;
                _C = capacitance * _Er;  /* Scaled by the single dielectric constant */
            }
            else
            {
                _Er = 1.0;
            }
            
            _Zo = sqrt(_L_vacuum / _C);  /* Standard formula for Zo */
            _Zodd = sqrt(_L_vacuum / _C);  /* Standard formula for Zo */
            _velocity = 1.0 / pow(_L_vacuum * _C, 0.5);
            _velocity_factor = _velocity / velocity_of_light_in_vacuum;
            relative_permittivity = sqrt(_velocity_factor); /* ??? XXXXXX */
            
            if (_verbose_level > 0) /* Only needed if intermediate results wanted.  */
            {
                print_data_for_two_conductor_lines();
            }
            count++;
        }
        while (fabs((capacitance_old - capacitance) / capacitance_old) > _cutoff); /* end of FD loop */
        
        
        if (_verbose_level >= 4)
        {
            printf("Total of %d iterations ( %d calls to finite_difference() )\n", ITERATIONS * count, count);
        }
        
        if ((_write_binary_field_imagesQ || _write_bitmap_field_imagesQ) && _er_bitmap.size() == 1)
        {
            write_fields();
        }
        
        if (_verbose_level == 0 && _er_bitmap.size() == 1)
        {
            print_data_for_two_conductor_lines();
        }
        
        if (_er_bitmap.size() > 1)
        {
            /* We know the capacitance and inductance for the air spaced line
            as we calculated it above. Howerver, whilst the inductance
            is independant of the dielectric, the capacitance is not, so this
            has to be recalculated, taking care not to alter the inductance
            at all */
            
            if(_verbose_level >= 2)
            {
                printf("Now taking into account the permittivities of the different dielectrics for 2 conductors.\n");
            }
            
            _dielectrics_to_consider_just_now = 2; /* Any number > 1 */
            capacitance = VERY_LARGE; /* Can be anything large */

            do /* Start a finite calculation */
            {
                capacitance_old = capacitance;
                capacitance = finite_difference_single_threaded();
                _C = capacitance;
                C_non_vacuum = capacitance;
                _Zo = sqrt(_L_vacuum / C_non_vacuum);  /* Standard formula for Zo */
                _velocity = 1.0 / pow(_L_vacuum * C_non_vacuum, 0.5);
                _velocity_factor = _velocity / velocity_of_light_in_vacuum;
                relative_permittivity = sqrt(_velocity_factor); /* ??? XXXXXX */
		        _Er = _C / _C_vacuum;
                if (_verbose_level > 0 ) /* Only needed if intermediate results wanted. */
                {
                    print_data_for_two_conductor_lines();
                }
            }
            while (fabs((capacitance_old - capacitance) / capacitance_old) > _cutoff); /* end of FD loop */

            /* We must print the results now, but only bother if the verbose level was 
            not not incremented on the command line, otherwide there will be two duplicate
            lines */

            if (_verbose_level == 0)
            {
                print_data_for_two_conductor_lines();
            }
        
            if (_write_binary_field_imagesQ || _write_bitmap_field_imagesQ)
            {
                write_fields();
            }
        }
    }
    
    else
    {
        /* The properties of a couplers will be computed in 2 or 4 stages
        1) Compute the odd-mode impedance, assuming a vacuum dielectric, or
        if there is just one dielectric, that one.

        2) Compute the odd-mode impedance, taking into account the effect of
        multiple dielectrics, IF NECESSARY

        at this point, the negative voltages will be turned into positive ones. 

        3) Compute the even-mode impedance, assuming a vacuum dielectric, or
        if there is just one dielectric, that one.

        4) Compute the even-mode impedance, taking into account the effect of
        multiple dielectrics, IF NECESSARY  */

        /* Stage 1 - compute the odd mode impedance assuming single dielectric */
        
        _display = Z_ODD_SINGLE_DIELECTRIC;
        _dielectrics_to_consider_just_now = 1;

        capacitance = VERY_LARGE; /* Can be anything large */
        if(_verbose_level >= 2)
        {
            printf("Solving assuming a vacuum dielectric to compute the odd-mode impedance\n");
        }

        do /* Start a finite difference calculation */
        {
            capacitance_old = capacitance;
            capacitance = finite_difference_single_threaded();
        
            _Codd_vacuum = capacitance;
            _Codd = capacitance;
            _Lodd_vacuum = MU_0 * EPSILON_0 / capacitance; /* Same as L in *ALL* cases */

            _Zodd_vacuum = sqrt(_Lodd_vacuum / _Codd_vacuum);  /* Standard formaul for Zodd */

            if (_er_bitmap.size() == 1) /* Just get C by simple scaling of Er */
            {
                _Codd *= _er_bitmap.begin()->second.epsilon;  /* Scaled by the single dielectric constant */
            }
            else
            {
                _Er = 1.0;
            }
            
            _Zodd = sqrt(_Lodd_vacuum / _Codd);  /* Standard formula for Zo */
            
            /* FPE trapdata->velocity_odd=1.0/pow(data->L_vacuum*data->Codd,0.5); */
            _velocity_odd = 1.0 / pow(_Lodd_vacuum * _Codd, 0.5);
            _velocity_factor_odd = _velocity_odd / velocity_of_light_in_vacuum;
            relative_permittivity_odd = sqrt(_velocity_factor_odd); /* ??? XXXXXX */
            _Er_odd = _Codd / _Codd_vacuum;
            _Zdiff = 2.0 * _Zodd;
            /* Print text if uses wants it */
            if(_verbose_level >= 1)
            {
                print_data_for_directional_couplers();
            }
        } while (fabs((capacitance_old - capacitance) / capacitance_old) > _cutoff); /* end of FD loop */

        /* display bitpamps/binary files if this is the last odd-mode computation */
        if ((_write_binary_field_imagesQ || _write_bitmap_field_imagesQ) && _er_bitmap.size() == 1)
        {
            write_fields("odd.");
        }
        
        /* Stage 2 - compute the odd-mode impedance taking into account other dielectrics IF NECESSARY */

        if (_er_bitmap.size() >1)
        {
            if (_verbose_level >= 2)
            {
                printf("Now taking into account the permittivities of the different dielectrics to compute Zodd.\n");
            }
            
            _display = Z_ODD_SINGLE_DIELECTRIC;
            capacitance = VERY_LARGE; /* Can be anything large */

            _dielectrics_to_consider_just_now = 2;
        
            do /* Start a finite calculation */
            {
                capacitance_old = capacitance;
                
                capacitance = finite_difference_single_threaded();
                _Codd = capacitance;
                _Zodd= sqrt(_Lodd_vacuum / _Codd);  /* Standard formula for Zo */
                
                _velocity_odd = 1.0 / pow(_L_vacuum * _C, 0.5);
                _velocity_factor_odd = _velocity / velocity_of_light_in_vacuum;
                relative_permittivity_odd = sqrt(_velocity_factor); /* ??? XXXXXX */
                _Er_odd = _Codd / _Codd_vacuum;
                _Zdiff = 2.0 * _Zodd;
                if(_verbose_level >= 1)
                {
                    print_data_for_directional_couplers();
                }
            } while (fabs((capacitance_old - capacitance) / capacitance_old) > _cutoff); /* end of FD loop */

            if ((_write_binary_field_imagesQ || _write_bitmap_field_imagesQ) && _er_bitmap.size() != 1)
            {
                write_fields("odd.");
            }
        } /* end of stage 2 for couplers */

        /* Stage 3 - compute the even-mode impedance assuming single dielectric */

        /* Since we want the even mode impedance now, we swap all the -1V
        metallic conductors for +1V */

        swap_conductor_voltages();

        _display = Z_EVEN_SINGLE_DIELECTRIC;
        _dielectrics_to_consider_just_now = 1;
        if(_verbose_level >= 2)
        {
            printf("Now assuming a vacuum dielectric to compute Zeven\n");
        }
        
        capacitance = VERY_LARGE; /* Can be anything large */


        do /* Start a finite difference calculation */
        {
            capacitance_old = capacitance;
            capacitance = finite_difference_single_threaded();

            _Ceven_vacuum = capacitance;
            _Ceven = capacitance;
            _Leven_vacuum = MU_0 * EPSILON_0 / capacitance; /* Same as L in *ALL* cases */

            _Zeven_vacuum = sqrt(_Leven_vacuum / _Ceven_vacuum);  /* Standard formaul for Zodd */

            if (_er_bitmap.size() == 1) /* Just get C by simple scaling of Er */
            {
                _Ceven*=_found_this_dielectric;  /* Scaled by the single dielectric constant */
                
            }
            else
            {
                _Er_even = 1.0;
            }
            _Zeven = sqrt(_Leven_vacuum / _Ceven);  /* Standard formula for Zo */
            _velocity_even = 1.0 / pow(_Leven_vacuum * _Ceven, 0.5);
            _velocity_factor_even = _velocity_even / velocity_of_light_in_vacuum;
            relative_permittivity_even = sqrt(_velocity_factor_even); /* ??? XXXXXX */
            _Er_even = _Ceven / _Ceven_vacuum;
            _Zcomm = _Zeven / 2.0;
            _Zo = sqrt(_Zodd * _Zeven);
            if (_verbose_level >= 1)
            {
                print_data_for_directional_couplers();
            }
            /* display bitpamps/binary files if this is the last even-mode computation */
        } while (fabs((capacitance_old - capacitance) / capacitance_old) > _cutoff); /* end of FD loop */

        if ((_write_binary_field_imagesQ || _write_bitmap_field_imagesQ) && _er_bitmap.size() == 1)
        {
            write_fields("even.", DONT_ZERO_ELEMENTS);
        }

        capacitance = VERY_LARGE; /* Can be anything large */
        /* Stage 4 - compute the even-mode impedance assuming multiple dielectrics IF NECESSARY */
        if (_er_bitmap.size() > 1)
        {
            _dielectrics_to_consider_just_now=2;
            if (_verbose_level >= 2)
            {
                printf("Now taking into account the permittivities of the different dielectrics to compute Zeven\n");
            }
            
            do /* Start a finite calculation */
            {
                capacitance_old = capacitance;
                capacitance = finite_difference_single_threaded();
                
                _Ceven = capacitance;
                _Zeven = sqrt(_Leven_vacuum / _Ceven);  /* Standard formula for Zo */
                _velocity_even = 1.0 / pow(_L_vacuum * _C, 0.5);
                _velocity_factor_even = _velocity / velocity_of_light_in_vacuum;
                relative_permittivity_even = sqrt(_velocity_factor); /* ??? XXXXXX */
                _Er_even = _Ceven / _Ceven_vacuum;
                _Zdiff = 2.0 * _Zodd;
                _Zcomm = _Zeven / 2.0; 
                _Zo = sqrt(_Zeven * _Zodd);
                if (_verbose_level >= 1)
                {
                    print_data_for_directional_couplers();
                }
            } while (fabs((capacitance_old - capacitance) / capacitance_old) > _cutoff); /* end of FD loop */

            if (_write_binary_field_imagesQ || _write_bitmap_field_imagesQ)
            {
                write_fields("even.", DONT_ZERO_ELEMENTS);
            }
        } /* end of stage 4 */
        
        /* Print the results if the verbose level was 0 (no -v flag(s) ). */
        if (_verbose_level == 0)
        {
            /* We need to print the data. The next function will only print if 
            the verbose_level is 1 or more, so I'll fix it at one. Then we print
            the final results and exit. */
            _verbose_level = 1;
            _display = Z_EVEN_SINGLE_DIELECTRIC;
            print_data_for_directional_couplers();
        }
    } /* end of if couplers */
}


float atlc3::finite_difference_single_threaded()
{
    int number_of_iterations = 25;
    float capacitance_per_metre;
    float energy_per_metre;

    /* The following might not look very neat, with a whole load of code being 
    written twice, when it would be posible to make it easier to read if the 
    'if(dielectrics_in_bitmap > 1)' was in an inner loop. However, the 
    following is almost certainly more efficient. It is not a good idea to 
    have any more than necessary in the inner loop. 

    The option to avoid the faster convergence algorithm has been didtched
    too, as this was in an inner loop. The faster covergence method seems
    to work fine, so there is no need to avoid using it */
 

    /* Note, that while the number of intterations requested is set in the first
    parameter to update_voltage_array, the actual number done is 4x higher, as 
    each computation id done in 4 directions */

    //update_voltage_array(number_of_iterations, 0, _mat.cols() - 1, 0, _mat.rows() - 1);
    update_voltage_array_fast(number_of_iterations);
    //update_voltage_array_fast(number_of_iterations, 1, _mat.cols() - 2, 1, _mat.rows() - 2);
    
    //matrix_rgb img_er;
    //cvt_rgb_er(img_er);
    //cv::Mat cvimger(img_er.rows(), img_er.cols(), CV_8UC3, img_er.data());
    //cv::imshow("imger", cvimger);
    
    //matrix_rgb img;
    //cvt_rgb(img);
    //cv::Mat cvimg(img.rows(), img.cols(), CV_8UC3, img.data());
    //cv::imshow("img", cvimg);
    //cv::waitKey(10);
    
    /* Once the v distribution is found, the energy in the field may be 
    found. This can be shown to be Energy = 0.5 * integral(E.D) dV, when 
    integrated over a volume V, and D.E is the vector dot product of E and
    D. 
  
    Energy per metre is 0.5 * D.E or (0.5*Epsilon)* E.E. Now E.E is given
    by Ex^2 + Ey^2 (by definition of a dot product. */

    energy_per_metre = 0.0;
    for(std::int32_t i = 1; i < _mat.cols() - 1; ++i)
    {
        for(std::int32_t j = 1; j < _mat.rows() - 1; ++j)
        {
            energy_per_metre += find_energy_per_metre(i, j);
        }
    }
    
    if(_coupler == false)
    {
        capacitance_per_metre = 2 * energy_per_metre;
    }
    else
    {
        capacitance_per_metre = energy_per_metre;
    }
    return (capacitance_per_metre);
}



/* The following function updates the v on the matrix V_to given data about the 
oddity of the location i,j and the voltages in the matrix V_from. It does this for n interations
between rows jmin and jmax inclusive and between columns imain and imax inclusive */

void atlc3::update_voltage_array(int nmax, int imin, int imax, int jmin, int jmax)
{
    int k, i, j, n;  
    unsigned char oddity_value;
    float Va, Vb, Vl, Vr, ERa, ERb, ERl, ERr;
    float Vnew, g;
    
    float r = 1.9;
    
    std::uint32_t width = _mat.cols();
    std::uint32_t height = _mat.rows();
    
    if (_dielectrics_to_consider_just_now == 1)
    {
        g = r;
    }
    else
    {
        g = 1.0;
    }
    
    for(n = 0; n  < nmax; ++n)
    {
        for(k = 0; k < 4; ++k)
        {
            for (j = (k == 0 || k == 3) ? jmin : jmax; (k ==0 || k == 3)  ? j <= jmax : j >= jmin ; (k == 0 || k ==3) ?  j++ : j--)
            {
                for (i = k&1 ? imax : imin;   k&1 ? i >=imin : i <= imax ;  k&1 ? i-- : i++)
                {
                    atlc3_node& node = _mat.at(j, i);
                    oddity_value = node.oddity; //oddity[i][j];
                        

                    if (oddity_value == CONDUCTOR_ZERO_V)
                    {
                        node.v = 0.0;
                    }
                    else if (oddity_value == CONDUCTOR_PLUS_ONE_V)
                    {
                        node.v = 1.0;
                    }
                    else if (oddity_value == CONDUCTOR_MINUS_ONE_V)
                    {
                        node.v = -1.0;
                    }
                    else if( oddity_value == TOP_LEFT_CORNER )
                    {  /* top left */
                        //Vnew = 0.5 * (V_from[1][0] + V_from[0][1]);
                        Vnew = 0.5 * (_mat.at(0, 1).v + _mat.at(1, 0).v);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == TOP_RIGHT_CORNER)
                    {
                        //Vnew = 0.5 * (V_from[width-2][0] + V_from[width-1][1]);         /* top right */
                        Vnew = 0.5 * (_mat.at(0, width - 2).v + _mat.at(1, width - 1).v);   /* top right */
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == BOTTOM_LEFT_CORNER)
                    {
                        //Vnew=0.5*(V_from[0][height-2]+V_from[1][height-1]);       /* bottom left */
                        Vnew = 0.5 * (_mat.at(height - 2, 0).v + _mat.at(height - 1, 1).v); /* bottom left */
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == BOTTOM_RIGHT_CORNER)
                    {   
                        //Vnew=0.5*(V_from[width-2][height-1]+V_from[width-1][height-2]); /* bottom right */
                        Vnew = 0.5 * (_mat.at(height - 1, width - 2).v + _mat.at(height - 2, width - 1).v); /* bottom right */
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    /* Now the sides */
                    else if (oddity_value == ORDINARY_POINT_LEFT_EDGE)
                    {  /* left hand side  */
                    
                        //Vnew=0.25*(V_from[0][j-1]+V_from[0][j+1] + 2*V_from[1][j]);
                        Vnew = 0.25 * (_mat.at(j - 1, 0).v + _mat.at(j + 1, 0).v + 2 * _mat.at(j, 1).v);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == ORDINARY_POINT_RIGHT_EDGE)
                    {   /* right hand side */
                        //Vnew=0.25*(V_from[width-1][j+1]+V_from[width-1][j-1]+2*V_from[width-2][j]);
                        Vnew = 0.25 * (_mat.at(j + 1, width - 1).v + _mat.at(j - 1, width - 1).v + 2 * _mat.at(j, width - 2).v);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == ORDINARY_POINT_TOP_EDGE)
                    { /* top row */ 
                        //Vnew=0.25*(V_from[i-1][0]+V_from[i+1][0]+2*V_from[i][1]);
                        Vnew = 0.25 * (_mat.at(0, i - 1).v + _mat.at(0, i + 1).v + 2 * _mat.at(1, i).v);
                        node.v = g * Vnew+(1 - g) * node.v;
                    }
                    else if (oddity_value == ORDINARY_POINT_BOTTOM_EDGE)
                    {   /* bottom row */ 
                        //Vnew=0.25*(V_from[i-1][height-1]+V_from[i+1][height-1]+2*V_from[i][height-2]);
                        Vnew = 0.25 * (_mat.at(height - 1, i - 1).v + _mat.at(height - 1, i + 1).v + 2 * _mat.at(height - 2, i).v);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == ORDINARY_INTERIOR_POINT
                        || (oddity_value >= DIFFERENT_DIELECTRIC_ABOVE_AND_RIGHT
                            && oddity_value < UNDEFINED_ODDITY
                            && _dielectrics_to_consider_just_now == 1))
                    {
                        //Va = V_from[i][j-1]; 
                        //Vb = V_from[i][j+1];
                        //Vl = V_from[i-1][j];
                        //Vr = V_from[i+1][j];
                        
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;

                        Vnew = (Va + Vb + Vl + Vr) / 4.0;
                        node.v = g * Vnew + (1 - g) * node.v;
                        //node.v = Vnew;
                    }

                    /* I'm not sure the following equations, which compute the v  
                    where there is a metal around are okay. One line of thought would 
                    say that the same equations as normal  i.e.
                        v_new=(v(i+1,j_+v(i-1,j)+v(i,j-1)+v(i,j+1))/4 should be used
                    but then since the electric field across the metal surface is zero,
                    the equation that was used to derrive  that equation is not valid.

                    Another thought of mine is that v near a metal will be more affected
                    by the metal than the dielectric, since the nearest part of the metal is at
                    at the same v as the node, whereas for a dielectric is less so. Hence
                    the following seems a sensible solution. Since the metal will have twice 
                    the effect of a dielectric, the v at i,j should be weighted such
                    that its effect is more strongly affected by the metal. This seems to 
                    produce reasonably accurate results, but whether this is chance or not
                    I don't know. */

                    else if (oddity_value == METAL_ABOVE)
                    {
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];

                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;
                        
                        Vnew = 0.25 * (4 * Va / 3 + 2 * Vb / 3 + Vl + Vr);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == METAL_BELOW)
                    {   
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;
        
                        Vnew = 0.25 * (4 * Vb / 3 + 2 * Va / 3 + Vl + Vr);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == METAL_LEFT)
                    {
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;

                        Vnew = 0.25 * (4 * Vl / 3 + 2 * Vr / 3 + Va + Vb);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == METAL_RIGHT)
                    {
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;

                        Vnew = 0.25 * (4 * Vr / 3 + 2 * Vl / 3 + Va + Vb);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == METAL_ABOVE_AND_RIGHT)
                    {
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;

                        Vnew = 0.25 * (4 * Vr / 3 + 4 * Va / 3 + 2 * Vl / 3 + 2 * Vb / 3);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == METAL_ABOVE_AND_LEFT)
                    {
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;

                        Vnew = 0.25 * (4 * Vl / 3 + 4 * Va / 3 + 2 * Vr / 3 + 2 * Vb / 3);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == METAL_BELOW_AND_LEFT)
                    {
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;

                        Vnew = 0.25 * (4 * Vl / 3 + 4 * Vb / 3 + 2 * Vr / 3 + 2 * Va / 3);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }
                    else if (oddity_value == METAL_BELOW_AND_RIGHT)
                    {
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;

                        Vnew = 0.25 * (4 * Vb / 3 + 4 * Vr / 3 + 2 * Va / 3 + 2 * Vl / 3);
                        node.v = g * Vnew + (1 - g) * node.v;
                    }

                    /* Again, when there is a change of permittivity, my equations may
                    (probably are wrong). My logic is that if there's and RF field,
                    the impedance is inversly proportional to Er. So if the material
                    above a node is of a higher permittivity, then the 
                    v will be closer to that of the node above, becuase of this.
                    The same applies for other directions of change in Er. */


                    else if (_dielectrics_to_consider_just_now > 1)
                    {
                        //Va=V_from[i][j-1]; 
                        //Vb=V_from[i][j+1];
                        //Vl=V_from[i-1][j];
                        //Vr=V_from[i+1][j];
                        Va = _mat.at(j - 1, i).v;
                        Vb = _mat.at(j + 1, i).v;
                        Vl = _mat.at(j, i - 1).v;
                        Vr = _mat.at(j, i + 1).v;

                        //ERa = Er[i][j-1]; 
                        //ERb = Er[i][j+1];
                        //ERl = Er[i-1][j];
                        //ERr = Er[i+1][j];
                        ERa = _mat.at(j - 1, i).er;
                        ERb = _mat.at(j + 1, i).er;
                        ERl = _mat.at(j, i - 1).er;
                        ERr = _mat.at(j, i + 1).er;

                        Vnew = (Va * ERa + Vb * ERb + Vl * ERl + Vr * ERr) / (ERa + ERb + ERl + ERr);
                        node.v = g * Vnew + (1 - g) * node.v;

                    }
                    else if ((_dielectrics_to_consider_just_now == 1 && oddity_value == UNDEFINED_ODDITY) 
                            || (_dielectrics_to_consider_just_now > 1))
                    {
                        fprintf(stderr,"Internal error in update_voltage_array.c\n");
                        fprintf(stderr,"i=%d j=%d oddity[%d][%d]=%d\n", i, j, i,j, oddity_value);
                        //exit(INTERNAL_ERROR);
                        return;
                    } /* end if if an internal error */
                } /* end of j loop */
            }
        }
    }
}



void atlc3::update_voltage_array_fast(int nmax)
{
    std::uint8_t oddity_value;
    float Va, Vb, Vl, Vr, ERa, ERb, ERl, ERr;
    float Vnew, g;
    
    
    std::int32_t width = _mat.cols();
    std::int32_t height = _mat.rows();
    
    if (_dielectrics_to_consider_just_now == 1)
    {
        g = _r;
    }
    else
    {
        g = 1.0;
    }
    
    
    for(std::int32_t n = 0; n  < nmax; ++n)
    {
        for (std::int32_t j = 0; j < height; j++)
        {
            for (std::int32_t i = 0; i < width; i++)
            {
                atlc3_node& node = _mat.at(j, i);
                oddity_value = node.oddity;
                    

                if (oddity_value == CONDUCTOR_ZERO_V)
                {
                    node.v = 0.0;
                }
                else if (oddity_value == CONDUCTOR_PLUS_ONE_V)
                {
                    node.v = 1.0;
                }
                else if (oddity_value == CONDUCTOR_MINUS_ONE_V)
                {
                    node.v = -1.0;
                }
                else if( oddity_value == TOP_LEFT_CORNER )
                {  /* top left */
                    Vnew = 0.5 * (_mat.at(0, 1).v + _mat.at(1, 0).v);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == TOP_RIGHT_CORNER)
                {
                    Vnew = 0.5 * (_mat.at(0, width - 2).v + _mat.at(1, width - 1).v);   /* top right */
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == BOTTOM_LEFT_CORNER)
                {
                    Vnew = 0.5 * (_mat.at(height - 2, 0).v + _mat.at(height - 1, 1).v); /* bottom left */
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == BOTTOM_RIGHT_CORNER)
                {   
                    Vnew = 0.5 * (_mat.at(height - 1, width - 2).v + _mat.at(height - 2, width - 1).v); /* bottom right */
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                /* Now the sides */
                else if (oddity_value == ORDINARY_POINT_LEFT_EDGE)
                {  /* left hand side  */
                    Vnew = 0.25 * (_mat.at(j - 1, 0).v + _mat.at(j + 1, 0).v + 2 * _mat.at(j, 1).v);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == ORDINARY_POINT_RIGHT_EDGE)
                {   /* right hand side */
                    Vnew = 0.25 * (_mat.at(j + 1, width - 1).v + _mat.at(j - 1, width - 1).v + 2 * _mat.at(j, width - 2).v);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == ORDINARY_POINT_TOP_EDGE)
                { /* top row */ 
                    Vnew = 0.25 * (_mat.at(0, i - 1).v + _mat.at(0, i + 1).v + 2 * _mat.at(1, i).v);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == ORDINARY_POINT_BOTTOM_EDGE)
                {   /* bottom row */ 
                    Vnew = 0.25 * (_mat.at(height - 1, i - 1).v + _mat.at(height - 1, i + 1).v + 2 * _mat.at(height - 2, i).v);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == ORDINARY_INTERIOR_POINT
                    || (oddity_value >= DIFFERENT_DIELECTRIC_ABOVE_AND_RIGHT
                        && oddity_value < UNDEFINED_ODDITY
                        && _dielectrics_to_consider_just_now == 1))
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;

                    Vnew = (Va + Vb + Vl + Vr) / 4.0;
                    node.v = g * Vnew + ((float)1 - g) * node.v;
                }

                /* I'm not sure the following equations, which compute the v  
                where there is a metal around are okay. One line of thought would 
                say that the same equations as normal  i.e.
                    v_new=(v(i+1,j_+v(i-1,j)+v(i,j-1)+v(i,j+1))/4 should be used
                but then since the electric field across the metal surface is zero,
                the equation that was used to derrive  that equation is not valid.

                Another thought of mine is that v near a metal will be more affected
                by the metal than the dielectric, since the nearest part of the metal is at
                at the same v as the node, whereas for a dielectric is less so. Hence
                the following seems a sensible solution. Since the metal will have twice 
                the effect of a dielectric, the v at i,j should be weighted such
                that its effect is more strongly affected by the metal. This seems to 
                produce reasonably accurate results, but whether this is chance or not
                I don't know. */

                else if (oddity_value == METAL_ABOVE)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;
                    
                    Vnew = 0.25 * (4 * Va / 3 + 2 * Vb / 3 + Vl + Vr);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == METAL_BELOW)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;
        
                    Vnew = 0.25 * (4 * Vb / 3 + 2 * Va / 3 + Vl + Vr);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == METAL_LEFT)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;

                    Vnew = 0.25 * (4 * Vl / 3 + 2 * Vr / 3 + Va + Vb);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == METAL_RIGHT)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;

                    Vnew = 0.25 * (4 * Vr / 3 + 2 * Vl / 3 + Va + Vb);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == METAL_ABOVE_AND_RIGHT)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;

                    Vnew = 0.25 * (4 * Vr / 3 + 4 * Va / 3 + 2 * Vl / 3 + 2 * Vb / 3);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == METAL_ABOVE_AND_LEFT)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;

                    Vnew = 0.25 * (4 * Vl / 3 + 4 * Va / 3 + 2 * Vr / 3 + 2 * Vb / 3);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == METAL_BELOW_AND_LEFT)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;

                    Vnew = 0.25 * (4 * Vl / 3 + 4 * Vb / 3 + 2 * Vr / 3 + 2 * Va / 3);
                    node.v = g * Vnew + (1 - g) * node.v;
                }
                else if (oddity_value == METAL_BELOW_AND_RIGHT)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;

                    Vnew = 0.25 * (4 * Vb / 3 + 4 * Vr / 3 + 2 * Va / 3 + 2 * Vl / 3);
                    node.v = g * Vnew + (1 - g) * node.v;
                }

                /* Again, when there is a change of permittivity, my equations may
                (probably are wrong). My logic is that if there's and RF field,
                the impedance is inversly proportional to Er. So if the material
                above a node is of a higher permittivity, then the 
                v will be closer to that of the node above, becuase of this.
                The same applies for other directions of change in Er. */


                else if (_dielectrics_to_consider_just_now > 1)
                {
                    Va = _mat.at(j - 1, i).v;
                    Vb = _mat.at(j + 1, i).v;
                    Vl = _mat.at(j, i - 1).v;
                    Vr = _mat.at(j, i + 1).v;
                    
                    ERa = _mat.at(j - 1, i).er;
                    ERb = _mat.at(j + 1, i).er;
                    ERl = _mat.at(j, i - 1).er;
                    ERr = _mat.at(j, i + 1).er;

                    Vnew = (Va * ERa + Vb * ERb + Vl * ERl + Vr * ERr) / (ERa + ERb + ERl + ERr);
                    node.v = g * Vnew + (1 - g) * node.v;

                }
                else if ((_dielectrics_to_consider_just_now == 1 && oddity_value == UNDEFINED_ODDITY) 
                        || (_dielectrics_to_consider_just_now > 1))
                {
                    fprintf(stderr,"Internal error in update_voltage_array.c\n");
                    fprintf(stderr,"i=%d j=%d oddity[%d][%d]=%d\n", i, j, i,j, oddity_value);
                    //exit(INTERNAL_ERROR);
                    return;
                } /* end if if an internal error */
            } /* end of j loop */
        }
    }
}


void atlc3::swap_conductor_voltages()
{
    
    for (std::int32_t row = 0; row < _mat.rows(); row++)
    {
        for (std::int32_t col = 0; col < _mat.cols(); col++)
        {
            atlc3_node& node = _mat.at(row, col);
            
            if (node.oddity == CONDUCTOR_MINUS_ONE_V)
            {
                node.v = 1.0;
                node.oddity = CONDUCTOR_PLUS_ONE_V;
            }
            else if (node.oddity == CONDUCTOR_ZERO_V)
            {
            }
            else if (node.oddity == CONDUCTOR_PLUS_ONE_V)
            {
            }
            else
            {
                node.v = 0.0;
            }
        }
    }
}


float atlc3::find_energy_per_metre(int w, int h)
{
    float energy_per_metre;
    float Ex;
    float Ey;

    Ex = find_Ex(w, h);
    Ey = find_Ey(w, h);
    energy_per_metre = (float)(0.5 * EPSILON_0) * (Ex * Ex + Ey * Ey);
    if (_dielectrics_to_consider_just_now > 1)
    {
        energy_per_metre *= _mat.at(h, w).er; //Er[w][h]; /* second run, energy proportional to Er */
    }
    return (energy_per_metre);
}


float atlc3::find_Ex(int i/*col*/, int j) 
{
    float Ex = 0.0;
    std::uint8_t odd = _mat.at(j, i).oddity; //oddity[i][j];
    std::uint32_t width = _mat.cols();
    
    if (odd > CONDUCTOR_FLOATING)
    {
        if (odd == TOP_LEFT_CORNER || odd == BOTTOM_LEFT_CORNER)
        {
            //Ex = Vij[0][j] - Vij[1][j];
            Ex = _mat.at(j, 0).v - _mat.at(j, 1).v;
        }
        else if (odd == TOP_RIGHT_CORNER || odd == BOTTOM_RIGHT_CORNER) 
        {
            //Ex = Vij[width-2][0]-Vij[width-1][0];
            Ex = _mat.at(0, width - 2).v - _mat.at(0, width - 1).v;
        }
        else if (odd == ORDINARY_POINT_TOP_EDGE || odd == ORDINARY_POINT_BOTTOM_EDGE) 
        {
            //Ex = 0.5*(Vij[i-1][j]-Vij[i+1][j]);
            Ex = 0.5 * (_mat.at(j, i - 1).v - _mat.at(j, i + 1).v);
        }
        else if (odd == ORDINARY_POINT_LEFT_EDGE) 
        {
            //Ex = (Vij[i][j]-Vij[i+1][j]);
            Ex = _mat.at(j, i).v - _mat.at(j, i + 1).v;
        }
        else if (odd == ORDINARY_POINT_RIGHT_EDGE) 
        {
            //Ex = (Vij[width-2][j]-Vij[width-1][j]);
            Ex = _mat.at(j, width - 2).v - _mat.at(j, width - 1).v;
        }
        else if (odd == METAL_LEFT || odd == METAL_BELOW_AND_LEFT || odd == METAL_ABOVE_AND_LEFT)
        {
            //Ex = Vij[i][j]-Vij[i+1][j];
            Ex = _mat.at(j, i).v - _mat.at(j, i + 1).v;
        }
        else if (odd == METAL_RIGHT || odd == METAL_ABOVE_AND_RIGHT || odd ==METAL_BELOW_AND_RIGHT)
        {
            //Ex = Vij[i-1][j]-Vij[i][j];
            Ex = _mat.at(j, i - 1).v - _mat.at(j, i).v;
        }
        else if (odd == ORDINARY_INTERIOR_POINT || odd == METAL_ABOVE || odd == METAL_BELOW)
        {
            //Ex = 0.5*(Vij[i-1][j]-Vij[i+1][j]);
            Ex = 0.5 * (_mat.at(j, i - 1).v - _mat.at(j, i + 1).v);
        }
        else if (odd >= DIFFERENT_DIELECTRIC_ABOVE_AND_RIGHT && odd < UNDEFINED_ODDITY  )
        {
            //Ex = 0.5*(Vij[i-1][j]-Vij[i+1][j]);
            Ex = 0.5 * (_mat.at(j, i - 1).v - _mat.at(j, i + 1).v);
        }
        else
        {
            fprintf(stderr,"oddity[%d][%d]=%d\n", i, j, odd);
            //exit_with_msg_and_exit_code("Internal error in find_Ex",INTERNAL_ERROR);
        }
    }
    return (Ex);
}

float atlc3::find_Ey(int i, int j)
{
    float Ey = 0.0;
    std::uint8_t odd = _mat.at(j, i).oddity; //oddity[i][j];
    std::int32_t height = _mat.rows();
    
    if (odd > CONDUCTOR_FLOATING)
    {

        if (odd == TOP_LEFT_CORNER || odd == TOP_RIGHT_CORNER) 
        {
            //Ey=Vij[i][1]-Vij[i][0];
            Ey = _mat.at(1, i).v - _mat.at(0, i).v;
        }

        else if (odd == BOTTOM_LEFT_CORNER || odd == BOTTOM_RIGHT_CORNER) 
        {
            //Ey=Vij[i][height-1]-Vij[i][height-2];
            Ey = _mat.at(height - 1, i).v - _mat.at(height - 2, i).v;
        }

        else if (odd == ORDINARY_POINT_LEFT_EDGE || odd == ORDINARY_POINT_RIGHT_EDGE) 
        {
            //Ey=0.5*(Vij[i][j+1]-Vij[i][j-1]);
            Ey = 0.5 * (_mat.at(j + 1, i).v - _mat.at(j - 1, i).v);
        }

        else if (odd == ORDINARY_POINT_BOTTOM_EDGE) 
        {
            //Ey=Vij[i][j+1]-Vij[i][j];
            Ey = _mat.at(j + 1, i).v - _mat.at(j, i).v;
            //Ey = 0 - _mat.at(j, i).v;
        }

        else if (odd == ORDINARY_POINT_TOP_EDGE)
        {
            //Ey=Vij[i][j]-Vij[i][j-1];
            Ey = _mat.at(j, i).v - _mat.at(j - 1, i).v;
            //Ey = _mat.at(j, i).v;
        }

        else if (odd == METAL_ABOVE || odd == METAL_ABOVE_AND_LEFT || odd == METAL_ABOVE_AND_RIGHT)
        {
            //Ey=Vij[i][j+1]-Vij[i][j];
            Ey = _mat.at(j + 1, i).v - _mat.at(j, i).v;
        }

        else if (odd == METAL_BELOW || odd == METAL_BELOW_AND_LEFT || odd == METAL_BELOW_AND_RIGHT)
        {
            //Ey=Vij[i][j]-Vij[i][j-1];
            Ey = _mat.at(j, i).v - _mat.at(j - 1, i).v;
        }

        else if(odd >= DIFFERENT_DIELECTRIC_LOCALLY || odd == ORDINARY_INTERIOR_POINT || odd == METAL_RIGHT || odd == METAL_LEFT)
        {
            //Ey=0.5*(Vij[i][j+1]-Vij[i][j-1]);
            Ey = 0.5 * (_mat.at(j + 1, i).v - _mat.at(j - 1, i).v);
        }
        else
        {
            fprintf(stderr,"oddity[%d][%d]=%d\n", i, j, odd);
            //exit_with_msg_and_exit_code("Internal error in find_Ey",INTERNAL_ERROR);
        }
    }
    return (Ey);
}

float atlc3::find_E(int w, int h)
{
    float Ex, Ey, E;
    Ex = find_Ex(w, h);
    Ey = find_Ey(w, h);
    E = sqrt(Ex * Ex + Ey * Ey);
    return(E);
}



void atlc3::cvt_rgb(matrix_rgb& img)
{
    img.create(_mat.rows(), _mat.cols());
    
    for (std::int32_t row = 0; row < _mat.rows(); row++)
    {
        for (std::int32_t col = 0; col < _mat.cols(); col++)
        {
            img.at(row, col).r = _mat.at(row, col).v * 255.;
            img.at(row, col).g = _mat.at(row, col).v * 255.;
            img.at(row, col).b = _mat.at(row, col).v * 255.;
        }
    }
}

void atlc3::cvt_rgb_er(matrix_rgb& img)
{
    img.create(_mat.rows(), _mat.cols());
    
    for (std::int32_t row = 0; row < _mat.rows(); row++)
    {
        for (std::int32_t col = 0; col < _mat.cols(); col++)
        {
            img.at(row, col).r = _mat.at(row, col).er * 255.;
            img.at(row, col).g = _mat.at(row, col).er * 255.;
            img.at(row, col).b = _mat.at(row, col).er * 255.;
        }
    }
}


/* The following simple function just prints data into a file, or if
fp-stout, to the screen. Depending on whether the dielectric is mixed or
not, it is or is not possible to quote a value for Er. If Er is passed
as a mumber < 0, this function interprets that as meaning that the
dielectric is mixed, and says 'Er= MIXED' */

void atlc3::print_data_for_two_conductor_lines()
{
    if (_verbose_level < 2)
    {
        printf("%s 2 Er= %5.2f Zo= %7.3f Ohms C= %6.1f pF/m L= %6.1f nH/m v= %.4g m/s v_f= %.3f VERSION= %s\n", 
            _inputfile_filename.c_str(), _Er, _Zo, _C * 1e12, _L_vacuum * 1e9, _velocity, _velocity_factor, PACKAGE_VERSION);
    }
    else
    {
        printf("%s 2 Er= %16.13f Zo= %16.13f Ohms C= %16.13f pF/m L= %16.13f nH/m v= %16.13g m/s v_f= %16.13f VERSION= %s\n",
            _inputfile_filename.c_str(), _Er, _Zo, _C * 1e12, _L_vacuum * 1e9, _velocity, _velocity_factor,PACKAGE_VERSION);
    }
}



/* The following simple function just prints data into a file, or if
fp-stout, to the screen. Depending on whether the dielectric is mixed or
not, it is or is not possible to quote a value for Er. If Er is passed
as a mumber < 0, this function interprets that as meaning that the
dielectric is mixed, and says 'Er= MIXED' */

void atlc3::print_data_for_directional_couplers()
{
    if (_display == Z_ODD_SINGLE_DIELECTRIC)
    {
        if(_verbose_level == 1)
        {
            printf("%s 3 Er_odd= %6.2f Er_even= %s Zodd= %7.3f Zeven= %s Zo= %s Zdiff= %6.2f Zcomm= %s Ohms VERSION=%s\n",
                _inputfile_filename.c_str(), _Er_odd, "??????", _Zodd, "??????","??????", _Zdiff, "??????", PACKAGE_VERSION);
        }
        else if (_verbose_level >= 2)
        {
            printf("%s 3 Er_odd= %6.2f Er_even= %s Zodd= %7.3f Zeven= %s Zo= %s Zdiff= %6.2f Zcomm= %s Ohms VERSION=%s\n",
                _inputfile_filename.c_str(), _Er_odd, "??????", _Zodd, "??????","??????", _Zdiff, "??????", PACKAGE_VERSION);
        }
    }
    else if (_display == Z_EVEN_SINGLE_DIELECTRIC)
    {
        if(_verbose_level == 1)
        {
            printf("%s 3 Er_odd= %7.3f Er_even= %7.3f Zodd= %7.3f Zeven= %7.3f Zo= %7.3f Zdiff= %7.3f Zcomm= %7.3f Ohms "
                                "Lodd= %7.3f nH/m Leven= %7.3f nH/m Codd= %7.3f pF/m  Ceven= %7.3f pF/m  VERSION=%s\n",
                                _inputfile_filename.c_str(), _Er_odd, _Er_even, _Zodd, _Zeven, _Zo, _Zdiff, _Zcomm,
                                _Lodd_vacuum * 1e9, _Leven_vacuum * 1e9, _Codd * 1e12, _Ceven * 1e12, PACKAGE_VERSION);
        }
        else if (_verbose_level >= 2)
        {
            printf("%s 3 Er_odd= %7.3f Er_even= %7.3f Zodd= %7.3f Zeven= %7.3f Zo= %7.3f Zdiff= %7.3f Zcomm= %7.3f Ohms VERSION=%s\n",
                _inputfile_filename.c_str(), _Er_odd, _Er_even, _Zodd, _Zeven, _Zo, _Zdiff, _Zcomm, PACKAGE_VERSION);
        }
    }
}



/* Write the following files, assuming an input of example.bmp 
where example.bmp is a 2 conductor transmission lines. For
3 conductor transmission lines (couplers) the function


example.E.bmp   Grayscale Bitmap of |E-field|, normallised to 1,
but corrected for Gamma
example.Ex.bmp  Colour Bitmap of x-directed E-field, normallised to 1, 
but corrected for Gamma
example.Ey.bmp  Colour Bitmap of y-directed E-field, normallised to 1, 
but corrected for Gamma
example.V.bmp   Colour Bitmap of Voltage field, normallised to 1, but 
corrected for Gamma
eexample.U.bmp  Grayscale bitmap, with just the energy (U=CV^2).

example.Ex.bin  binary file, with just the x-directed E-field 
(in volts/pixel) as doubles 
example.Ey.bin  binary file, with just the y-directed E-field 
(in volts/pixel) as doubles 
example.E.bin   binary file, with just the E-field {sqrt(Ex^2+Ey^2)} 
(in volts/pixel) as doubles 
example.V.bin   binary file, with just the Voltage as doubles 
eexample.U.bin  binary file, with just the energy (U=CV^2).

*/

extern float image_fiddle_factor;


void atlc3::write_fields(std::string name, std::int32_t zero_elementsQ)
{
    FILE *Ex_bin_fp = NULL, *Ey_bin_fp = NULL;
    FILE *E_bin_fp = NULL, *V_bin_fp, *U_bin_fp = NULL;
    
#ifdef WRITE_ODDITY_DATA
    FILE *oddity_bmp_fp=NULL;
    float odd;
#endif
    unsigned char r, g, b;
  
    FILE *permittivity_bin_fp = NULL;
  
    int w, h;
    float V, E, Ex, Ey, U;

    matrix_rgb image_data_Ex; 
    matrix_rgb image_data_Ey;
    matrix_rgb image_data_E;
    matrix_rgb image_data_U; 
    matrix_rgb image_data_V;
    matrix_rgb image_data_Er;
#ifdef WRITE_ODDITY_DATA
    matrix_rgb image_data_oddity;
#endif 


    if(_write_binary_field_imagesQ)
    {
        Ex_bin_fp = fopen((_inputfile_filename + ".Ex." + name + "bin").c_str(), "wb");
        Ey_bin_fp = fopen((_inputfile_filename + ".Ey." + name + "bin").c_str(), "wb");
        E_bin_fp = fopen((_inputfile_filename + ".E." + name + "bin").c_str(), "wb");
        V_bin_fp = fopen((_inputfile_filename + ".V." + name + "bin").c_str(), "wb");
        U_bin_fp = fopen((_inputfile_filename + ".U." + name + "bin").c_str(), "wb");
        permittivity_bin_fp = fopen((_inputfile_filename + ".Er.bin").c_str(), "wb");
        
        for (std::int32_t row = _mat.rows() - 1; row >= 0; row++)
        {
            for (std::int32_t col = 0; col < _mat.cols(); col++)
            {
                V = _mat.at(row, col).v;
                
                Ex = find_Ex(col, row);
                Ey = find_Ey(col, row);
                E = find_E(col, row); 
                U = find_energy_per_metre(col, row);
                if (Ex_bin_fp)
                {
                    if (fwrite((void *)&Ex, sizeof(float), 1, Ex_bin_fp) != 1)
                    {
                        printf("Error#1: Failed to write binary file in atlc3.c");
                    }
                }
                
                if (Ey_bin_fp)
                {
                    if (fwrite((void *)&Ey, sizeof(float), 1, Ey_bin_fp) != 1)
                    {
                        printf("Error#2: Failed to write binary file in atlc3.c");
                    }
                }
                
                if (E_bin_fp)
                {
                    if (fwrite((void *)&E, sizeof(float), 1, E_bin_fp) != 1)
                    {
                        printf("Error#3: Failed to write binary file in atlc3.c");
                    }
                }
                
                if (V_bin_fp)
                {
                    if (fwrite((void *)&V, sizeof(float), 1, V_bin_fp) != 1)
                    {
                        printf("Error#4: Failed to write binary file in atlc3.c");
                    }
                }
                
                if (U_bin_fp)
                {
                    if (fwrite((void *)&U, sizeof(float), 1, U_bin_fp) != 1)
                    {
                        printf("Error#5: Failed to write binary file in atlc3.c");
                    }
                }
                
                if (permittivity_bin_fp)
                {
                    float Er = _mat.at(row, col).er;
                    if (fwrite((void *)&Er, sizeof(float), 1, permittivity_bin_fp) != 1)
                    {
                        printf("Error#6: Failed to write binary file in atlc3.c");
                    }
                }
            }
        }
        
        if (Ex_bin_fp)
        {
            fclose(Ex_bin_fp);
        }
        if (Ey_bin_fp)
        {
            fclose(Ey_bin_fp);
        }
        if (E_bin_fp)
        {
            fclose(E_bin_fp);
        }
        if (V_bin_fp)
        {
            fclose(V_bin_fp);
        }
        if (U_bin_fp)
        {
            fclose(U_bin_fp);
        }
        if (permittivity_bin_fp)
        {
            fclose(permittivity_bin_fp);
        }
        
    } /* end of writing binary files for 2 conductor lines */

    if (_write_bitmap_field_imagesQ)
    {
        find_maximum_values(&_maximum_values, zero_elementsQ); /* sets stucture maximum_values */

        /* Allocate ram to store the bitmaps before they are written to disk */
        image_data_Ex.create(_mat.rows(), _mat.cols());
        image_data_Ey.create(_mat.rows(), _mat.cols());
        image_data_E.create(_mat.rows(), _mat.cols());
        image_data_V.create(_mat.rows(), _mat.cols());
        image_data_Er.create(_mat.rows(), _mat.cols());
        image_data_U.create(_mat.rows(), _mat.cols());
#ifdef WRITE_ODDITY_DATA
        image_data_oddity.create(_mat.rows(), _mat.cols());
#endif


        for(h = 0; h < _mat.rows(); h++)
        {
            for(w = 0; w < _mat.cols(); ++w)
            {
                Ex = find_Ex(w, h);
                Ey = find_Ey(w, h);
                E = find_E(w, h); 
                U = find_energy_per_metre(w, h);
                calculate_colour_data(Ex, _maximum_values.Ex_or_Ey_max, w, h, COLOUR, &r, &g, &b, IMAGE_FIDDLE_FACTOR);
                image_data_Ex.at(h, w).r = r;  image_data_Ex.at(h, w).g = g; image_data_Ex.at(h, w).b = b;
                
                calculate_colour_data(Ey, _maximum_values.Ex_or_Ey_max, w, h, COLOUR, &r, &g, &b, IMAGE_FIDDLE_FACTOR);
                image_data_Ey.at(h, w).r = r;  image_data_Ey.at(h, w).g = g; image_data_Ey.at(h, w).b = b;
                
                calculate_colour_data(E, _maximum_values.E_max, w, h, MONOCHROME, &r, &g, &b, IMAGE_FIDDLE_FACTOR);
                image_data_E.at(h, w).r = r;  image_data_E.at(h, w).g = g; image_data_E.at(h, w).b = b;
                
                calculate_colour_data(U, _maximum_values.U_max, w, h, MONOCHROME, &r, &g, &b, IMAGE_FIDDLE_FACTOR);
                image_data_U.at(h, w).r = r;  image_data_U.at(h, w).g = g; image_data_U.at(h, w).b = b;
                
                calculate_colour_data(_mat.at(h, w).v, _maximum_values.V_max, w, h, COLOUR, &r, &g, &b, IMAGE_FIDDLE_FACTOR);
                image_data_V.at(h, w).r = r;  image_data_V.at(h, w).g = g; image_data_V.at(h, w).b = b;
                
                calculate_colour_data(_mat.at(h, w).er, MAX_ER, w, h, MIXED, &r, &g, &b, IMAGE_FIDDLE_FACTOR);
                image_data_Er.at(h, w).r = r;  image_data_Er.at(h, w).g = g; image_data_Er.at(h, w).b = b;
                
    #ifdef WRITE_ODDITY_DATA
                odd=(float)_mat.at(h, w).oddity;
                calculate_colour_data(odd, 255 , w, h, MONOCHROME, &r, &g, &b, 1.0);
                image_data_oddity.at(h, w).r = r;  image_data_oddity.at(h, w).g = g; image_data_oddity.at(h, w).b = b;
    #endif


            }
        }

        bitmap_write((_inputfile_filename + ".Ex." + name + "bmp").c_str(), image_data_Ex);
        bitmap_write((_inputfile_filename + ".Ey." + name + "bmp").c_str(), image_data_Ey);
        bitmap_write((_inputfile_filename + ".E." + name + "bmp").c_str(), image_data_E);
        bitmap_write((_inputfile_filename + ".V." + name + "bmp").c_str(), image_data_V);
        bitmap_write((_inputfile_filename + ".U." + name + "bmp").c_str(), image_data_U);
        bitmap_write((_inputfile_filename + ".Er." + name + "bmp").c_str(), image_data_Er);

#ifdef WRITE_ODDITY_DATA
        bitmap_write((_inputfile_filename + ".oddity.bmp").c_str(), image_data_oddity);
#endif
    }
}

void atlc3::find_maximum_values(struct max_values *maximum_values, int zero_elementsQ)
{
    float U, V, Ex, Ey, E, permittivity;
    int i, j;

    /* It makes sense to draw the even and odd mode images on the same
    scale, so if its a coupler, they elements are not zeroed if the
    function is called when doing the even mode, which is done
    after the odd mode */

    if(zero_elementsQ == ZERO_ELEMENTS_FIRST)
    {
        maximum_values->E_max = 0.0;
        maximum_values->Ex_or_Ey_max = 0.0;
        maximum_values->V_max = 0.0;
        maximum_values->U_max = 0.0;
        maximum_values->permittivity_max = 0.0;
    }
    
    std::int32_t width = _mat.cols();
    std::int32_t height = _mat.rows();
    for(i = 0; i < _mat.cols(); ++i)
    {
        for(j = 0;j < _mat.rows(); ++j)
        {
            V = _mat.at(j, i).v;
            U = find_energy_per_metre(i, j);
            if(1)
            {
                if(i == 0)
                {
                    //Ex=2*Er[i+1][j]*(Vij[i][j]-Vij[i+1][j])/(Er[i+1][j]+Er[i][j]);
                    Ex = 2 * _mat.at(j, i + 1).er * (_mat.at(j, i).v - _mat.at(j, i + 1).v) / 
                            (_mat.at(j, i + 1).er + _mat.at(j, i).er);
                }
                else if (i == width - 1) 
                {
                    //Ex=2*Er[i-1][j]*(Vij[i-1][j]-Vij[i][j])/(Er[i-1][j]+Er[i][j]);
                    Ex = 2 * _mat.at(j, i - 1).er * (_mat.at(j, i - 1).v - _mat.at(j, i).v) / 
                            (_mat.at(j, i - 1).er + _mat.at(j, i).er);
                }
                else /* This is the best estimate, but can't be done on boundary */
                {
                    //Ex = Er[i-1][j]*(Vij[i-1][j]-Vij[i][j])/(Er[i-1][j]+Er[i][j]);
                    
                    Ex = _mat.at(j, i - 1).er * (_mat.at(j, i - 1).v - _mat.at(j, i).v) / 
                            (_mat.at(j, i - 1).er + _mat.at(j, i).er);
                            
                    //Ex += Er[i+1][j]*(Vij[i][j]-Vij[i+1][j])/(Er[i+1][j]+Er[i][j]);
                    Ex += _mat.at(j, i + 1).er * (_mat.at(j, i).v - _mat.at(j, i + 1).v) / 
                            (_mat.at(j, i + 1).er + _mat.at(j, i).er);
                }
                
                if(j == 0)
                {
                    //Ey=2*Er[i][j+1]*(Vij[i][j]-Vij[i][j+1])/(Er[i][j+1]+Er[i][j]);
                    Ey = 2 * _mat.at(j + 1, i).er * (_mat.at(j, i).v - _mat.at(j + 1, i).v) / 
                            (_mat.at(j + 1, i).er + _mat.at(j, i).er);
                }
                else if (j == height - 1)
                {
                    //Ey=2*Er[i][j-1]*(Vij[i][j-1]-Vij[i][j])/(Er[i][j-1]+Er[i][j]);
                    Ey = 2 * _mat.at(j - 1, i).er * (_mat.at(j - 1, i).v - _mat.at(j, i).v) / 
                            (_mat.at(j - 1, i).er + _mat.at(j, i).er);
                }
                else
                {
                    //Ey=Er[i][j-1]*(Vij[i][j-1]-Vij[i][j])/(Er[i][j-1]+Er[i][j]);
                    Ey = _mat.at(j - 1, i).er * (_mat.at(j - 1, i).v - _mat.at(j, i).v) / 
                            (_mat.at(j - 1, i).er + _mat.at(j, i).er);
                            
                    //Ey+=Er[i][j+1]*(Vij[i][j]-Vij[i][j+1])/(Er[i][j+1]+Er[i][j]);
                    Ey += _mat.at(j + 1, i).er * (_mat.at(j, i).v - _mat.at(j + 1, i).v) / 
                            (_mat.at(j + 1, i).er + _mat.at(j, i).er);
                }
                E = sqrt(Ex * Ex + Ey * Ey);
                permittivity = _mat.at(j, i).er;
            }
            else
            {
                Ex=0.0;
                Ey=0.0;
                E=0.0;
                permittivity = 0.0;
            }
            
            if (U > 1.0)
            {             
                printf("U=%f v=%f Er=%f at %d %d\n", U, V, _mat.at(j, i).er, i, j);
            }
            
            if (E > maximum_values->E_max)
            {
                maximum_values->E_max = E;
            }

            if (fabs(Ex) > maximum_values->Ex_or_Ey_max)
            {
                maximum_values->Ex_or_Ey_max = fabs(Ex);
            }
            
            if (fabs(Ey) > maximum_values->Ex_or_Ey_max)
            {
                maximum_values->Ex_or_Ey_max = fabs(Ey);
            }
            
            if (fabs(E) > maximum_values->E_max)
            {
                maximum_values->E_max = fabs(E);
            }
            
            if (fabs(V) > maximum_values->V_max)
            {
                maximum_values->V_max = fabs(V); 
            }
            
            if (U > maximum_values->U_max)
            {
                maximum_values->U_max = U; 
            }
            
            if (permittivity > maximum_values->permittivity_max)
            {
                maximum_values->permittivity_max = permittivity; 
            }
        /* printf("Ex_or_Ey_max=%g E_max=%g V_max=%g U_max=%g Er_max=%g\n",maximum_values->Ex_or_Ey_max, maximum_values->E_max, maximum_values->V_max, maximum_values->U_max, maximum_values->permittivity_max);  */
        }
    }
}


void atlc3::calculate_colour_data(float x, float xmax, int w, int h, int image_type,
unsigned char *red, unsigned char *green, unsigned char *blue, float image_fiddle_factor)
{
    if(image_type == COLOUR) /*Ex, Ey, V */
    {
        if(fabs(x) < 1e-9) /* This is a bit of a fiddle I admit, but */
        {                  /* Is needed to prevent problems at 'small' values */
            *red = 0;
            *green = 0;
            *blue = 0;
        }
        else if (x > 0.0)
        {
            *red = 255 *pow(x / xmax, 1.0 / image_fiddle_factor);
            *green = 0; 
            *blue = 0; 
        }
        else 
        {
            *red = 0; 
            *green = 0; 
            *blue = 255 * pow(-x / xmax, 1.0 / image_fiddle_factor); 
        }
    }
    else if (image_type == MONOCHROME) /* E, energy, permittivity */
    {
        if (x > xmax)
        {
            *red=0; *green=0; *blue=0;
        }
        else
        {
            *red = 255 * pow(fabs(x / xmax), 1.0 / image_fiddle_factor);
            *green = 255 * pow(fabs(x / xmax), 1.0 / image_fiddle_factor);
            *blue = 255 * pow(fabs(x / xmax), 1.0 / image_fiddle_factor);
        }
    }
    else if (image_type == MIXED) /* Only for permittivity*/
    {
        if (_mat.at(h, w).oddity == CONDUCTOR_ZERO_V)
        {
            *red = 0; *green = 255; *blue = 0;
        }
        else if (_mat.at(h, w).oddity == CONDUCTOR_PLUS_ONE_V)
        {
            *red = 255; *green = 0; *blue = 0;
        }
        else if (_mat.at(h, w).oddity == CONDUCTOR_MINUS_ONE_V)
        {
            *red = 0; *green = 0; *blue = 255;
        }
        else
        {
            *red = 255 * pow(fabs(x / xmax), 1.0 / image_fiddle_factor);
            *green = 255 * pow(fabs(x / xmax), 1.0 / image_fiddle_factor);
            *blue = 255 * pow(fabs(x / xmax), 1.0 / image_fiddle_factor);
        }
    }
}
