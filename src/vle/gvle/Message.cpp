/**
 * @file vle/gvle/Message.cpp
 * @author The VLE Development Team
 */

/*
 * VLE Environment - the multimodeling and simulation environment
 * This file is a part of the VLE environment (http://vle.univ-littoral.fr)
 * Copyright (C) 2003 - 2008 The VLE Development Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <gtkmm/messagedialog.h>
#include <gtkmmconfig.h>
#include <vle/gvle/Message.hpp>
#include <vle/utils/Tools.hpp>

namespace vle
{
namespace gvle {

void Debug(const Glib::ustring& debug)
{
    Gtk::MessageDialog* box;

    Glib::ustring::size_type pos = debug.find("----------\n");
    if (pos == Glib::ustring::npos) {
        box = new Gtk::MessageDialog(debug, false, Gtk::MESSAGE_WARNING,
                                     Gtk::BUTTONS_OK, true);
    } else {
        Glib::ustring left(debug, 0, pos);
        Glib::ustring right(debug, pos + 12);

        box = new Gtk::MessageDialog(left, false, Gtk::MESSAGE_WARNING,
                                     Gtk::BUTTONS_OK, true);

#if GTK_VERSION_GT(2,4)
        box->set_secondary_text(right);
#endif
    }

    box->run();
    delete box;
}

void Info(const Glib::ustring& information)
{
    Gtk::MessageDialog* box;

    Glib::ustring::size_type pos = information.find("----------\n");
    if (pos == Glib::ustring::npos) {
        box = new Gtk::MessageDialog(information, false, Gtk::MESSAGE_INFO,
                                     Gtk::BUTTONS_OK, true);
    } else {
        Glib::ustring left(information, 0, pos);
        Glib::ustring right(information, pos + 12);

        box = new Gtk::MessageDialog(left, false, Gtk::MESSAGE_INFO,
                                     Gtk::BUTTONS_OK, true);
#if GTK_VERSION_GT(2,4)
        box->set_secondary_text(right);
#endif
    }

    box->run();
    delete box;
}

void Error(const Glib::ustring& msg)
{
    Glib::ustring msg_first;
    Glib::ustring msg_second;

    Glib::ustring::size_type pos1 = msg.find("----------");
    if (pos1 == Glib::ustring::npos) {
        msg_first.assign(msg);
    } else {
        msg_first.assign(msg, 0, pos1);
        msg_second.assign(msg, pos1 + 10, Glib::ustring::npos);
        msg_second = Glib::Markup::escape_text(msg_second);

        Glib::ustring::size_type pos2 = msg_second.find("----------");
        if (pos2 == Glib::ustring::npos) {
            msg_second.insert(0, "<i>");
            msg_second.append("</i>");
        } else {
            msg_second.erase(pos2, 10);
            msg_second.insert(pos2, "</i>\n<tt>");
            msg_second.insert(0, "<i>");
            msg_second.append("</tt>");
        }
    }

    msg_first.insert(0, "<b>");
    msg_first.append("</b>\n");
    msg_first += msg_second;

    Gtk::MessageDialog* box = new Gtk::MessageDialog(
        msg_first, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

    box->run();
    delete box;
}

bool Question(const Glib::ustring& question)
{
    Gtk::MessageDialog* box = NULL;
    int r;

    box = new Gtk::MessageDialog(question, false, Gtk::MESSAGE_QUESTION,
                                 Gtk::BUTTONS_YES_NO, true);
    r = box->run();
    delete box;

    return (r == Gtk::RESPONSE_YES);
}

}
} // namespace vle gvle