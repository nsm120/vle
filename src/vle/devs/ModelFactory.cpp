/**
 * @file devs/ModelFactory.cpp
 * @author The VLE Development Team.
 * @brief Read simulations plugin from models directories and manage models
 * classes.
 */

/*
 * Copyright (C) 2004, 05, 06 - The vle Development Team
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <vle/devs/ModelFactory.hpp>
#include <vle/devs/Simulator.hpp>
#include <vle/devs/Dynamics.hpp>
#include <vle/graph/Model.hpp>
#include <vle/graph/AtomicModel.hpp>
#include <vle/graph/CoupledModel.hpp>
#include <vle/utils/XML.hpp>
#include <vle/utils/Debug.hpp>
#include <vle/utils/Tools.hpp>
#include <vle/utils/Path.hpp>
#include <vle/utils/Trace.hpp>

namespace vle { namespace devs {

ModelFactory::ModelFactory(Coordinator& coordinator,
                           const vpz::Dynamics& dyn,
                           const vpz::Classes& cls) :
    mCoordinator(coordinator),
    mDynamics(dyn),
    mClasses(cls)
{
}

devs::SimulatorList ModelFactory::createModels(
                const graph::AtomicModelVector& lst,
                SimulatorMap& result)
{
    return createModelsFromDynamics(lst, result, mDynamics);
}

graph::Model* ModelFactory::createModels(const std::string& classname,
                                         devs::SimulatorList& lst,
                                         SimulatorMap& result)
{
    const vpz::Class& cl(mClasses.get(classname));
    graph::Model* clone = cl.model().model();

    graph::AtomicModelVector vt;
    graph::CoupledModel::getAtomicModelList(clone, vt);

    lst = createModelsFromDynamics(vt, result, cl.dynamics());
    return clone;
}

Glib::Module* ModelFactory::buildPlugin(const vpz::Dynamic& dyn)
{
    std::string file1(Glib::Module::build_path(
        utils::Path::path().getDefaultModelDir(), dyn.library()));
    
    Glib::Module* module = new Glib::Module(file1);
    if (not (*module)) {
        std::string err1(Glib::Module::get_last_error());
        delete module;

        std::string file2(Glib::Module::build_path(
            utils::Path::path().getUserModelDir(), dyn.library()));
        
        module = new Glib::Module(file2);
        if (not (*module)) {
            std::string err2(Glib::Module::get_last_error());
            delete module;

            Glib::ustring er((boost::format(
                        "error opening plugin '%1%' or '%2%' library "
                        "'%3%'\nwith error\n") % file1 % file2 %
                    dyn.library()).str());
            er += err1;
            er += " ";
            er += err2;
            Throw(utils::InternalError, er);
        }
    }
    module->make_resident();
    return module;
}

devs::SimulatorList ModelFactory::createModelsFromDynamics(
                const graph::AtomicModelVector& lst,
                SimulatorMap& result,
                const vpz::Dynamics& dyn)
{
    devs::SimulatorList newsatom;
    graph::AtomicModelVector::const_iterator it;
    for (it = lst.begin(); it != lst.end(); ++it) {
        Simulator* a = new Simulator((graph::AtomicModel*)(*it));

        const vpz::Dynamic& d(dyn.get((*it)->getName()));
        switch(d.type()) {
        case vpz::Dynamic::LOCAL:
            attachDynamics(a, d, getPlugin((*it)->getName()));
            break;
        case vpz::Dynamic::DISTANT:
            Throw(utils::InternalError, "Distant dynamics not yet implemented.");
            break;
        }

        Assert(utils::ParseError,
               result.find((graph::AtomicModel*)(*it)) == result.end(),
               boost::format("Already existing Simulator '%1%' name '%2%'\n") %
               *it % (*it)->getName());

        newsatom.push_back(a);
        result[(graph::AtomicModel*)(*it)] = a;
    }
    return newsatom;
}


Glib::Module* ModelFactory::getPlugin(const std::string& name)
{
    if (mDynamics.exist(name)) {
        return buildPlugin(mDynamics.get(name));
    } else {
        vpz::Classes::const_iterator clit;
        for (clit = mClasses.begin(); clit != mClasses.end(); ++clit) {
            if ((*clit).second.dynamics().exist(name)) {
                return buildPlugin((*clit).second.dynamics().get(name));
            }
        }
    }

    Throw(utils::ArgError, boost::format(
            "Dynamics plugin '%1%' not found in model factory\n") % name);
}

void ModelFactory::attachDynamics(devs::Simulator* atom,
                                  const vpz::Dynamic& /* dyn */,
                                  Glib::Module* module)
{
    devs::Dynamics* call = 0;
    void* makeNewDynamics = 0;

    bool getSymbol = module->get_symbol("makeNewDynamics", makeNewDynamics);
    Assert(utils::ParseError, getSymbol, boost::format(
            "Error in '%1%', function 'makeNewDynamics' not found '%2%'\n") %
        module->get_name() % Glib::Module::get_last_error());

    call = ((Dynamics*(*)(Simulator*))(makeNewDynamics))(atom);
    Assert(utils::ParseError, call, boost::format(
            "Error in '%1%', function 'makeNewDynamics':"
            "problem allocation a new Dynamics '%2%'\n") %
        module->get_name() % Glib::Module::get_last_error());
    
    atom->addDynamics(call);
    delete module;
}

}} // namespace vle devs
