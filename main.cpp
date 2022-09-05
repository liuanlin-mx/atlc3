#include <stdio.h>
#include "matrix.h"
#include "bitmap.h"
#include "atlc3.h"

static std::vector<std::string> _string_split(std::string str, const std::string& key)
{
    std::vector<std::string> out;
    std::string::size_type begin = 0;
    std::string::size_type end = 0;
    while ((end = str.find(key, begin)) != str.npos)
    {
        out.push_back(str.substr(begin, end - begin));
        begin = end + key.size();
    }
    
    if (begin < str.size())
    {
        out.push_back(str.substr(begin, end - begin));
    }
    
    return out;
}

int main(int argc, char **argv)
{
    matrix_rgb img;
    atlc3 atlc;
    if (argc < 2)
    {
        return 0;
    }
    
    const char *filename = argv[1];
    
    for (std::int32_t i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        const char *arg_next = argv[i + 1];
        
        if (std::string(arg) == "-S")
        {
            atlc.enable_binary_imageQ(false);
        }
        else if (std::string(arg) == "-s")
        {
            atlc.enable_bitmap_imageQ(false);
        }
        else if (std::string(arg) == "-vv")
        {
            atlc.set_verbose_level(2);
        }
        else if (std::string(arg) == "-vvv")
        {
            atlc.set_verbose_level(3);
        }
        else if (std::string(arg) == "-vvvv")
        {
            atlc.set_verbose_level(4);
        }
        else if (std::string(arg) == "-c" && i < argc)
        {
            atlc.set_cutoff(atof(arg_next));
        }
        else if (std::string(arg) == "-r" && i < argc)
        {
            atlc.set_rate_multiplier(atof(arg_next));
        }
        else if (std::string(arg) == "-d" && i < argc)
        {
            std::vector<std::string> v_str = _string_split(arg_next, "=");
            if (v_str.size() == 2)
            {
                std::uint32_t color = strtol(v_str[0].c_str(), NULL, 16);
                std::uint32_t er = atof(v_str[1].c_str());
                atlc.add_er(color, er);
            }
        }
        else
        {
            filename = arg;
        }
    }
    
    if (std::string(filename) == "-")
    {
        bitmap_read(NULL, img);
    }
    else
    {
        bitmap_read(filename, img);
        atlc.set_inputfile_filename(filename);
    }
    
    if (img.rows() == 0 || img.cols() == 0)
    {
        return 0;
    }
    
    atlc.setup_arrays(img);
    atlc.set_oddity_value();
    atlc.do_fd_calculation();
    
    return 0;
}
