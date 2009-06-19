/**
 * @file vle/gvle/NewProjectBox.cpp
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

#include <vle/gvle/Message.hpp>
#include <vle/gvle/NewProjectBox.hpp>
#include <vle/gvle/Modeling.hpp>
#include <vle/utils/Debug.hpp>
#include <vle/utils/Package.hpp>
#include <boost/filesystem.hpp>

namespace vle
{
namespace gvle {

NewProjectBox::NewProjectBox(Glib::RefPtr<Gnome::Glade::Xml> xml,
			     Modeling* m) :
    mXml(xml),
    mModeling(m)
{
    xml->get_widget("DialogNewProject", mDialog);
    xml->get_widget("EntryNameProject", mEntryName);

    xml->get_widget("ButtonNewProjectApply", mButtonApply);
    mButtonApply->signal_clicked().connect(
	sigc::mem_fun(*this, &NewProjectBox::onApply));

    xml->get_widget("ButtonNewProjectCancel", mButtonCancel);
    mButtonCancel->signal_clicked().connect(
	sigc::mem_fun(*this, &NewProjectBox::onCancel));
}

NewProjectBox::~NewProjectBox()
{
    delete mButtonApply;
    delete mButtonCancel;
    delete mEntryName;
    delete mDialog;
}

void NewProjectBox::show()
{
    mEntryName->set_text("");
    mDialog->set_title("New Project");
    mDialog->show_all();
    mDialog->run();
}

void NewProjectBox::onApply()
{
    if (not mEntryName->get_text().empty()) {
	if (not exist(mEntryName->get_text())) {
	    std::string out;
	    std::string err;
	    utils::Path::path().setPackage(mEntryName->get_text());
	    vle::utils::CMakePackage::create(out, err);
	    mModeling->getGVLE()->buildPackageHierarchy();
	    mDialog->hide_all();
	} else {
	    Error(_("The Project ") +
		  static_cast<std::string>(mEntryName->get_text()) +
		  (_(" already exist")));
	}
    }
}

void NewProjectBox::onCancel()
{
    mDialog->hide_all();
}

bool NewProjectBox::exist(std::string name)
{
    utils::PathList list = utils::CMakePackage::getInstalledPackages();
    utils::PathList::const_iterator it = list.begin();
    while (it != list.end()) {
	if (boost::filesystem::basename(*it) == name) {
	    return true;
	}
	++it;
    }
    return false;
}

}
}



