/*
 * This file is part of VLE, a framework for multi-modeling, simulation
 * and analysis of complex dynamical systems.
 * http://www.vle-project.org
 *
 * Copyright (c) 2003-2016 Gauthier Quesnel <quesnel@users.sourceforge.net>
 * Copyright (c) 2003-2016 ULCO http://www.univ-littoral.fr
 * Copyright (c) 2007-2016 INRA http://www.inra.fr
 *
 * See the AUTHORS or Authors.txt file for copyright owners and
 * contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <vle/gvle/OutputPlugin.hpp>

namespace vle { namespace gvle {

OutputPlugin::OutputPlugin(const std::string& package,
                           const std::string& library)
    : m_package(package), m_library(library)
{
}

Glib::RefPtr < Gdk::Pixbuf > OutputPlugin::icon() const
{
    //if (not m_icon) {
        //std::string file = utils::Path::path().getOutputPixmapFile(m_name);

        //try {
            //m_icon = Gdk::Pixbuf::create_from_file(file);
        //} catch(const Glib::FileError& e) {
            //throw utils::FileError(fmt(_(
                    //"OutputPlugin '%1%': FileError, %2%")) % m_name % e.what());
        //} catch(const Gdk::PixbufError& e) {
            //throw utils::FileError(fmt(_(
                    //"OutputPlugin '%1%': PixbufError, %2%")) % m_name %
                //e.what());
        //} catch(...) {
            //throw utils::FileError(fmt(_(
                    //"OutputPlugin '%1%': Unknow error")) % m_name);
        //}
    //}
    return m_icon;
}

}} // namespace vle gvle
