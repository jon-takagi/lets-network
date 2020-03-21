// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

//[debug_settings_includes
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <set>
#include <exception>
#include <iostream>
namespace pt = boost::property_tree;
//]
//[debug_settings_data
struct debug_settings
{
    std::string m_file;               // log filename
    int m_level;                      // debug level
    std::set<std::string> m_modules;  // modules where logging is enabled
    void load(const std::string &filename);
    void load(const std::string file, const int level, const std::set<std::string> modules);
    void save(const std::string &filename);
};
//]
//[debug_settings_load
void debug_settings::load(const std::string file, const int level, const std::set<std::string> modules) {
    m_file = file;
    m_level = level;
    m_modules = modules;


}
//]
//[debug_settings_save
void debug_settings::save(const std::string &filename)
{
    // Create empty property tree object
    pt::ptree tree;

    tree.put("debug.filename", m_file);
    tree.put("debug.level", m_level);
    BOOST_FOREACH(const std::string &name, m_modules)
        tree.add("debug.modules.module", name);
    pt::write_json(filename, tree);
}
//]

int main()
{
    try
    {
        debug_settings ds;
        std::set<std::string> modules;
        modules.insert("Finance");
        modules.insert("Admin");
        modules.insert("HR");
        ds.load("example.json", 2, modules);
        ds.save("output.json");
        std::cout << "Success\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
