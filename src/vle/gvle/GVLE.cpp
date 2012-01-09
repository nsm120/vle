/*
 * @file vle/gvle/GVLE.cpp
 *
 * This file is part of VLE, a framework for multi-modeling, simulation
 * and analysis of complex dynamical systems
 * http://www.vle-project.org
 *
 * Copyright (c) 2003-2007 Gauthier Quesnel <quesnel@users.sourceforge.net>
 * Copyright (c) 2003-2011 ULCO http://www.univ-littoral.fr
 * Copyright (c) 2007-2011 INRA http://www.inra.fr
 *
 * See the AUTHORS or Authors.txt file for copyright owners and contributors
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


#include <vle/gvle/GVLE.hpp>
#include <vle/gvle/About.hpp>
#include <vle/gvle/Message.hpp>
#include <vle/gvle/AtomicModelBox.hpp>
#include <vle/gvle/ImportModelBox.hpp>
#include <vle/gvle/ImportClassesBox.hpp>
#include <vle/gvle/CoupledModelBox.hpp>
#include <vle/gvle/DynamicsBox.hpp>
#include <vle/gvle/Editor.hpp>
#include <vle/gvle/FileTreeView.hpp>
#include <vle/gvle/ExperimentBox.hpp>
#include <vle/gvle/Modeling.hpp>
#include <vle/gvle/HelpBox.hpp>
#include <vle/gvle/HostsBox.hpp>
#include <vle/gvle/GVLEMenuAndToolbar.hpp>
#include <vle/gvle/PreferencesBox.hpp>
#include <vle/gvle/ViewDrawingArea.hpp>
#include <vle/gvle/ViewOutputBox.hpp>
#include <vle/gvle/View.hpp>
#include <vle/gvle/LaunchSimulationBox.hpp>
#include <vle/gvle/OpenPackageBox.hpp>
#include <vle/gvle/NewProjectBox.hpp>
#include <vle/gvle/Settings.hpp>
#include <vle/utils/Exception.hpp>
#include <vle/utils/Package.hpp>
#include <vle/vpz/Vpz.hpp>
#include <vle/version.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sstream>
#include <gtkmm/filechooserdialog.h>
#include <glibmm/spawn.h>
#include <glibmm/miscutils.h>
#include <gtkmm/stock.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace vle { namespace gvle {

/*  - - - - - - - - - - - - - --ooOoo-- - - - - - - - - - - -  */

GVLE::GVLE(BaseObjectType* cobject,
           const Glib::RefPtr<Gnome::Glade::Xml> xml):
    Gtk::Window(cobject),
    mModeling(new Modeling()),
    mCurrentButton(VLE_GVLE_POINTER),
    mCutCopyPaste(this),
    mCurrentTab(-1)
{
    mRefXML = xml;
    mModeling->setGlade(mRefXML);

    mModeling->signalModified().connect(
        sigc::mem_fun(*this, &GVLE::onSignalModified));

    Settings::settings().load();

    mGlobalVpzPrevDirPath = "";

    mConditionsBox = new ConditionsBox(mRefXML, this);
    mPreferencesBox = new PreferencesBox(mRefXML);
    mAtomicBox = new AtomicModelBox(mRefXML, mModeling, this);
    mImportModelBox = new ImportModelBox(mRefXML, mModeling);
    mCoupledBox = new CoupledModelBox(xml, mModeling, this);
    mImportClassesBox = new ImportClassesBox(xml, mModeling, this);
    mOpenVpzBox = new OpenVpzBox(mRefXML, mModeling, this);
    mSaveVpzBox = new SaveVpzBox(mRefXML, mModeling, this);
    mQuitBox = new QuitBox(mRefXML, this);

    mRefXML->get_widget("MenuAndToolbarVbox", mMenuAndToolbarVbox);
    mRefXML->get_widget("StatusBarPackageBrowser", mStatusbar);
    mRefXML->get_widget("TextViewLogPackageBrowser", mLog);
    mRefXML->get_widget_derived("FileTreeViewPackageBrowser", mFileTreeView);
    mFileTreeView->setParent(this);
    mRefXML->get_widget_derived("NotebookPackageBrowser", mEditor);
    mEditor->setParent(this);
    mRefXML->get_widget_derived("TreeViewModel", mModelTreeBox);
    mModelTreeBox->setModelingGVLE(mModeling, this);
    mRefXML->get_widget_derived("TreeViewClass", mModelClassBox);
    mModelClassBox->createNewModelBox(mModeling, this);
    mModelTreeBox->set_sensitive(false);
    mModelClassBox->set_sensitive(false);
    mFileTreeView->set_sensitive(false);

    mMenuAndToolbar = new GVLEMenuAndToolbar(this);
    mMenuAndToolbarVbox->pack_start(*mMenuAndToolbar->getMenuBar());
    mMenuAndToolbarVbox->pack_start(*mMenuAndToolbar->getToolbar());
    mMenuAndToolbar->getToolbar()->set_toolbar_style(Gtk::TOOLBAR_BOTH);

    if (mModeling->vpz().project().model().model() != 0) {
        setTitle(mModeling->getFileName());
    }

    Settings::settings(); /** Initialize the GVLE settings singleton to read
                            colors, fonts, commands from the configuration file
                            `vle.conf'. */

    Glib::signal_timeout().connect(sigc::mem_fun(*this, &GVLE::on_timeout),
                                   1000 );

    set_title(windowTitle());
    resize(900, 550);
    show();
}

std::string GVLE::windowTitle()
{
    std::string result("GVLE ");
    result += VLE_VERSION;

    std::string extra(VLE_EXTRA_VERSION);
    if (not extra.empty()) {
        result += '-';
        result += VLE_EXTRA_VERSION;
    }

    return result;
}

GVLE::~GVLE()
{
    delViews();
    delete mModeling;
    delete mAtomicBox;
    delete mImportModelBox;
    delete mCoupledBox;
    delete mImportClassesBox;
    delete mConditionsBox;
    delete mPreferencesBox;
    delete mOpenVpzBox;
    delete mSaveVpzBox;
    delete mQuitBox;
    delete mMenuAndToolbar;
}

void GVLE::setGlade(Glib::RefPtr < Gnome::Glade::Xml > xml)
{
    mRefXML = xml;
    mAtomicBox = new AtomicModelBox(xml, mModeling, this);
    mImportModelBox = new ImportModelBox(xml, mModeling);
    mCoupledBox = new CoupledModelBox(xml, mModeling, this);
    mModeling->setGlade(xml);
}

bool GVLE::on_timeout()
{
    mConnectionTimeout.disconnect();

    mStatusbar->push("");
    return false;
}

void GVLE::onSignalModified()
{
    if (mModeling->isModified()) {
        setModifiedTitle(mModeling->getFileName());
        getMenu()->showSave();
    } else {
        getMenu()->hideSave();
    }
}

void GVLE::show()
{
    buildPackageHierarchy();
    show_all();
}

void GVLE::showMessage(const std::string& message)
{
    mConnectionTimeout.disconnect();
    mConnectionTimeout = Glib::signal_timeout().
        connect(sigc::mem_fun(*this, &GVLE::on_timeout), 30000);

    mStatusbar->push(message);
}

void GVLE::setModifiedTitle(const std::string& name)
{
    if (not name.empty() and utils::Path::extension(name) == ".vpz") {
        Editor::Documents::const_iterator it =
            mEditor->getDocuments().find(name);
        if (it != mEditor->getDocuments().end()) {
            it->second->setTitle(name ,getModeling()->getTopModel(), true);
        }
    }
}

void GVLE::buildPackageHierarchy()
{
    mPackage = vle::utils::Path::path().getPackageDir();
    mFileTreeView->clear();
    mFileTreeView->setPackage(mPackage);
    mFileTreeView->build();
}

void GVLE::refreshPackageHierarchy()
{
    mFileTreeView->refresh();
}

void GVLE::refreshEditor(const std::string& oldName, const std::string& newName)
{
    if (newName.empty()) { // the file is removed
        mEditor->closeTab(Glib::build_filename(mPackage, oldName));
    } else {
        std::string filePath = utils::Path::buildFilename(mPackage, oldName);
        std::string newFilePath = utils::Path::buildFilename(mPackage,
                                                             newName);
        mEditor->changeFile(filePath, newFilePath);
        mModeling->setFileName(newFilePath);
    }
}

void GVLE::setFileName(std::string name)
{
    if (not name.empty() and utils::Path::extension(name) == ".vpz") {
        parseXML(name);
        getEditor()->openTabVpz(mModeling->getFileName(),
                                mModeling->getTopModel());
        if (mModeling->getTopModel()) {
            mMenuAndToolbar->showEditMenu();
            mMenuAndToolbar->showSimulationMenu();
            redrawModelTreeBox();
            redrawModelClassBox();
        }
    }
    mModeling->setModified(false);
}

void GVLE::modifications(std::string filepath, bool isModif)
{
    if (utils::Path::extension(filepath) == ".vpz") {
        mMenuAndToolbar->onOpenVpz();
        mModelTreeBox->set_sensitive(true);
        mModelClassBox->set_sensitive(true);
    } else {
        mMenuAndToolbar->onOpenFile();
        mModelTreeBox->set_sensitive(false);
        mModelClassBox->set_sensitive(false);
    }
    if (isModif) {
        mMenuAndToolbar->showSave();
    } else {
        mMenuAndToolbar->hideSave();
    }
    setTitle(utils::Path::basename(filepath) +
                    utils::Path::extension(filepath));
}

void GVLE::focusRow(std::string filepath)
{
    mFileTreeView->showRow(filepath);
}

void GVLE::addView(graph::Model* model)
{
    if (model) {
        if (model->isCoupled()) {
            graph::CoupledModel* m = graph::Model::toCoupled(model);
            addView(m);
        } else if (model->isAtomic()) {
            try {
                mAtomicBox->show((graph::AtomicModel*)model);
            } catch (utils::SaxParserError& /*e*/) {
                parse_model(mModeling->vpz().project().model().atomicModels());
            }
        }
    }
    refreshViews();
}

void GVLE::addView(graph::CoupledModel* model)
{
    const size_t szView = mListView.size();

    View* search = findView(model);
    if (search != NULL) {
        search->selectedWindow();
    } else {
        mListView.push_back(new View(mModeling, model, szView, this));
    }
    getEditor()->openTabVpz(mModeling->getFileName(), model);
}

void GVLE::addViewClass(graph::Model* model, std::string name)
{
    assert(model);
    if (model->isCoupled()) {
        graph::CoupledModel* m = (graph::CoupledModel*)(model);
        addViewClass(m, name);
    } else if (model->isAtomic()) {
        try {
            mAtomicBox->show((graph::AtomicModel*)model, name);
        } catch (utils::SaxParserError& E) {
            parse_model(mModeling->vpz().project().classes().
                        get(mCurrentClass).atomicModels());
        }
    }
    refreshViews();
}

void GVLE::addViewClass(graph::CoupledModel* model, std::string name)
{
    const size_t szView = mListView.size();

    View* search = findView(model);
    if (search != NULL) {
        search->selectedWindow();
    } else {
        View* v = new View(mModeling, model, szView, this);
        v->setCurrentClass(name);
        mListView.push_back(v);
    }
    getEditor()->openTabVpz(getModeling()->getFileName(), model);
}

void GVLE::insertLog(const std::string& text)
{
    if (not text.empty()) {
        Glib::RefPtr < Gtk::TextBuffer > ref = mLog->get_buffer();
        if (ref) {
            ref->insert(ref->end(), text);
        }
    }
}

void GVLE::scrollLogToLastLine()
{
    Glib::RefPtr < Gtk::TextBuffer > ref = mLog->get_buffer();
    if (ref) {
        Glib::RefPtr < Gtk::TextMark > mark = ref->get_mark("end");
        if (mark) {
            ref->move_mark(mark, ref->end());
        } else {
            mark = ref->create_mark("end", ref->end());
        }

        mLog->scroll_to(mark, 0.0, 0.0, 1.0);
    }
}

void GVLE::start()
{
    mModeling->clearModeling();
    mModeling->delNames();
    mModeling->setTopModel(mModeling->newCoupledModel(0,
                                                      "Top model", "", 0, 0));
    setTitle(mModeling->getFileName());
    mModeling->vpz().project().model().setModel(mModeling->getTopModel());
    redrawModelTreeBox();
    redrawModelClassBox();
    if (utils::Package::package().name().empty()) {
        mModeling->setFileName("noname.vpz");
    } else {
        mModeling->setFileName(Glib::build_filename(
                                   utils::Path::path().getPackageExpDir(),
                                   "noname.vpz"));
    }
    mModeling->setSaved(false);
    mModeling->setModified(true);
    addView(mModeling->getTopModel());
    getEditor()->openTabVpz(mModeling->getFileName(), mModeling->getTopModel());
    setModifiedTitle(mModeling->getFileName());
}

void GVLE::start(const std::string& path, const std::string& fileName)
{
    mModeling->clearModeling();
    mModeling->delNames();
    mModeling->setTopModel(mModeling->newCoupledModel(0,
                                                      "Top model", "", 0, 0));
    setTitle(mModeling->getFileName());
    mModeling->vpz().project().model().setModel(mModeling->getTopModel());
    redrawModelTreeBox();
    redrawModelClassBox();
    mModeling->setFileName(path + "/" + fileName);
    mModeling->setSaved(false);
    mModeling->setModified(true);
    addView(mModeling->getTopModel());
    getEditor()->openTabVpz(mModeling->getFileName(), mModeling->getTopModel());
    setModifiedTitle(mModeling->getFileName());
}

void GVLE::parseXML(const std::string& filename)
{
    getEditor()->closeVpzTab();
    mModeling->parseXML(filename);
    mModeling->setFileName(filename);
    delViews();
    addView(mModeling->getTopModel());
    mModeling->setSaved(true);
    mModeling->setModified(false);
    setTitle(mModeling->getFileName());
}

bool GVLE::existView(graph::CoupledModel* model)
{
    assert(model);
    ListView::const_iterator it = mListView.begin();
    while (it != mListView.end()) {
        if ((*it) && (*it)->getGCoupledModel() == model)
            return true;
        ++it;
    }
    return false;
}

View* GVLE::findView(graph::CoupledModel* model)
{
    assert(model);
    ListView::const_iterator it = mListView.begin();
    while (it != mListView.end()) {
        if ((*it) && (*it)->getGCoupledModel() == model) {
            return *it;
        }
        ++it;
    }
    return NULL;
}

void GVLE::delViewIndex(size_t index)
{
    assert(index < mListView.size());
    delete mListView[index];
    mListView[index] = NULL;
}

void GVLE::delViewOnModel(const graph::CoupledModel* cm)
{
    assert(cm);
    const size_t sz = mListView.size();
    for (size_t i = 0; i < sz; ++i) {
        if (mListView[i] != 0 and mListView[i]->getGCoupledModel() == cm)
            delViewIndex(i);
    }
}

void GVLE::delViews()
{
    const size_t sz = mListView.size();
    for (size_t i = 0; i < sz; ++i)
        delViewIndex(i);
}

void GVLE::refreshViews()
{
    ListView::iterator it = mListView.begin();
    while (it != mListView.end()) {
        if (*it) {
            (*it)->redraw();
        }
        ++it;
    }
}

void GVLE::redrawView()
{
    View* currentView = dynamic_cast<DocumentDrawingArea*>(
        mEditor->get_nth_page(mCurrentTab))->getView();
    currentView->redraw();
}

void GVLE::redrawModelTreeBox()
{
    assert(mModeling->getTopModel());
    mModelTreeBox->parseModel(mModeling->getTopModel());
}

void GVLE::redrawModelClassBox()
{
    mModelClassBox->parseClass();
}

void GVLE::clearModelTreeBox()
{
    mModelTreeBox->clear();
}

void GVLE::clearModelClassBox()
{
    mModelClassBox->clear();
}

void GVLE::showRowTreeBox(const std::string& name)
{
    mModelTreeBox->showRow(name);
}

void GVLE::showRowModelClassBox(const std::string& name)
{
    mModelClassBox->showRow(name);
}

bool GVLE::on_delete_event(GdkEventAny* event)
{
    if (event->type == GDK_DELETE) {
        onQuit();
        return true;
    }
    return false;
}

void GVLE::onArrow()
{
    mCurrentButton = VLE_GVLE_POINTER;
    mEditor->getDocumentDrawingArea()->updateCursor();
    showMessage(_("Selection"));
}

void GVLE::onAddModels()
{
    mCurrentButton = VLE_GVLE_ADDMODEL;
    mEditor->getDocumentDrawingArea()->updateCursor();
    showMessage(_("Add models"));
}

void GVLE::onAddLinks()
{
    mCurrentButton = VLE_GVLE_ADDLINK;
    mEditor->getDocumentDrawingArea()->updateCursor();
    showMessage(_("Add links"));
}

void GVLE::onDelete()
{
    mCurrentButton = VLE_GVLE_DELETE;
    mEditor->getDocumentDrawingArea()->updateCursor();
    showMessage(_("Delete object"));
}

void GVLE::onAddCoupled()
{
    mCurrentButton = VLE_GVLE_ADDCOUPLED;
    mEditor->getDocumentDrawingArea()->updateCursor();
    showMessage(_("Coupled Model"));
}

void GVLE::onZoom()
{
    mCurrentButton = VLE_GVLE_ZOOM;
    mEditor->getDocumentDrawingArea()->updateCursor();
    showMessage(_("Zoom"));
}

void GVLE::onQuestion()
{
    mCurrentButton = VLE_GVLE_QUESTION;
    mEditor->getDocumentDrawingArea()->updateCursor();
    showMessage(_("Question"));
}

void GVLE::onNewFile()
{
    mEditor->createBlankNewFile();
    mMenuAndToolbar->onOpenFile();
    mMenuAndToolbar->showSave();
}

void GVLE::onNewFile(const std::string& path, const std::string& fileName)
{
    mEditor->createBlankNewFile(path, fileName);
    mMenuAndToolbar->onOpenFile();
    mMenuAndToolbar->showSave();
}

void GVLE::onNewVpz()
{
    if (not mModeling->isModified() or mModeling->getFileName().empty() or
        (mModeling->isModified() and
         gvle::Question(_("Do you really want load a new Model ?\nCurrent "
                          "model will be destroy and not save")))) {
        getEditor()->closeVpzTab();
        start();
        redrawModelTreeBox();
        redrawModelClassBox();
        mMenuAndToolbar->onOpenVpz();
        mMenuAndToolbar->showSave();
        mEditor->getDocumentDrawingArea()->updateCursor();
        mModelTreeBox->set_sensitive(true);
        mModelClassBox->set_sensitive(true);
        if (mCurrentButton == VLE_GVLE_POINTER){
            showMessage(_("Selection"));
        }
    }
    onExperimentsBox();
}

void GVLE::onNewNamedVpz(const std::string& path, const std::string& filename)
{
    if (not mModeling->isModified() or mModeling->getFileName().empty() or
        (mModeling->isModified() and
         gvle::Question(_("Do you really want load a new Model ?\nCurrent "
                          "model will be destroy and not save")))) {
        getEditor()->closeVpzTab();
        start(path.c_str(), filename.c_str());
        redrawModelTreeBox();
        redrawModelClassBox();
        mMenuAndToolbar->onOpenVpz();
        mMenuAndToolbar->showSave();
        mEditor->getDocumentDrawingArea()->updateCursor();
        mModelTreeBox->set_sensitive(true);
        mModelClassBox->set_sensitive(true);
        if (mCurrentButton == VLE_GVLE_POINTER){
            showMessage(_("Selection"));
        }
    }
    onExperimentsBox();
}

void GVLE::onNewProject()
{
    NewProjectBox box(mRefXML, mModeling, this);
    box.show();
    mMenuAndToolbar->onOpenProject();
    clearModelTreeBox();
    clearModelClassBox();
    mFileTreeView->set_sensitive(true);
    onTrouble();
}

void GVLE::onOpenFile()
{
    Gtk::FileChooserDialog file(_("Choose a file"),
                                Gtk::FILE_CHOOSER_ACTION_OPEN);
    file.set_transient_for(*this);
    file.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    file.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

    if (file.run() == Gtk::RESPONSE_OK) {
        std::string selected_file = file.get_filename();
        mEditor->openTab(selected_file);
        mMenuAndToolbar->onOpenFile();
    }
}

void GVLE::onOpenProject()
{
    OpenPackageBox box(mRefXML);

    if (box.run()) {
        onCloseProject();
        utils::Package::package().select(box.name());
        mPluginFactory.update();
        buildPackageHierarchy();
        mMenuAndToolbar->onOpenProject();
        setTitle("");
        mFileTreeView->set_sensitive(true);
        onTrouble();
    } else if (not utils::Package::package().existsPackage(
            utils::Package::package().name())) {
        onCloseProject();
    }
}

void GVLE::onOpenVpz()
{
    if (not mModeling->isModified() or mModeling->getFileName().empty() or
        (mModeling->isModified() and
         gvle::Question(_("Do you really want load a new Model ?\nCurrent "
                          "model will be destroy and not save")))) {
        try {
            if (mOpenVpzBox->run() == Gtk::RESPONSE_OK) {
                redrawModelTreeBox();
                redrawModelClassBox();
                mMenuAndToolbar->onOpenVpz();
                mModelTreeBox->set_sensitive(true);
                mModelClassBox->set_sensitive(true);
                mEditor->getDocumentDrawingArea()->updateCursor();
                if (mCurrentButton == VLE_GVLE_POINTER){
                    showMessage(_("Selection"));
                }
            }
        } catch(utils::InternalError) {
            Error((fmt(_("No experiments in the package '%1%'")) %
                   utils::Package::package().name()).str());
        }
    }
}

void GVLE::onOpenGlobalVpz()
{
    if (not mModeling->isModified() or mModeling->getFileName().empty() or
        (mModeling->isModified() and
         gvle::Question(_("Do you really want load a new Model ?\nCurrent "
                          "model will be destroy and not save")))) {
        Gtk::FileChooserDialog file("VPZ file", Gtk::FILE_CHOOSER_ACTION_OPEN);
        file.set_transient_for(*this);
        file.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        file.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
        Gtk::FileFilter filter;
        filter.set_name(_("Vle Project gZipped"));
        filter.add_pattern("*.vpz");
        file.add_filter(filter);
        if (mGlobalVpzPrevDirPath != "") {
            file.set_current_folder(mGlobalVpzPrevDirPath);
        }

        if (file.run() == Gtk::RESPONSE_OK) {
            mGlobalVpzPrevDirPath = file.get_current_folder();
            mEditor->closeAllTab();
            utils::Package::package().select("");
            mPluginFactory.update();
            mPackage = "";
            parseXML(file.get_filename());
            getEditor()->openTabVpz(mModeling->getFileName(),
                                    mModeling->getTopModel());
            if (mModeling->getTopModel()) {
                redrawModelTreeBox();
                redrawModelClassBox();
                mModelTreeBox->set_sensitive(true);
                mModelClassBox->set_sensitive(true);
                mMenuAndToolbar->onOpenVpz();
                mMenuAndToolbar->hideCloseProject();
                mFileTreeView->clear();
            }
        }
    }

}

void GVLE::onRefresh()
{
    mFileTreeView->refresh();
}

void GVLE::onShowCompleteView()
{
    DocumentDrawingArea* tab = dynamic_cast<DocumentDrawingArea*>(
        mEditor->get_nth_page(mCurrentTab));
    graph::CoupledModel* currentModel;
    if (tab-> isComplete()) {
        currentModel = tab->getCompleteDrawingArea()->getModel();
    } else {
        currentModel = tab->getSimpleDrawingArea()->getModel();
    }
    mEditor->showCompleteView(mModeling->getFileName(),currentModel);
}

void GVLE::onShowSimpleView()
{
    DocumentDrawingArea* tab = dynamic_cast<DocumentDrawingArea*>(
        mEditor->get_nth_page(mCurrentTab));
    graph::CoupledModel* currentModel;
    if (tab-> isComplete()) {
        currentModel = tab->getCompleteDrawingArea()->getModel();
    } else {
        currentModel = tab->getSimpleDrawingArea()->getModel();
    }
    mEditor->showSimpleView(mModeling->getFileName(), currentModel);
}

bool GVLE::checkVpz()
{
    std::vector<std::string> vec;
    mModeling->vpz_is_correct(vec);

    if (vec.size() != 0) {
        std::string error = _("Vpz incorrect :\n");
        std::vector<std::string>::const_iterator it = vec.begin();

        while (it != vec.end()) {
            error += *it + "\n";
            ++it;
        }
        Error(error);
        return false;
    }
    return true;
}

void GVLE::onSave()
{
    int page = mEditor->get_current_page();

    if (page != -1) {
        if (dynamic_cast < Document* >(mEditor->get_nth_page(page))
            ->isDrawingArea()) {
            saveVpz();
        } else {
            DocumentText* doc = dynamic_cast < DocumentText* >(
                mEditor->get_nth_page(page));

            if (not doc->isNew() || doc->hasFullName()) {
                doc->save();
            } else {
                Gtk::FileChooserDialog file(_("Text file"),
                                            Gtk::FILE_CHOOSER_ACTION_SAVE);

                file.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
                file.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
                file.set_current_folder(utils::Path::path().getPackageDir());
                if (file.run() == Gtk::RESPONSE_OK) {
                    std::string filename(file.get_filename());

                    doc->saveAs(filename);
                    mModeling->setFileName(filename);
                }
            }
            refreshPackageHierarchy();
        }
    }
}

void GVLE::onSaveAs()
{
    int page = mEditor->get_current_page();

    Glib::ustring title;

    bool isVPZ = dynamic_cast < Document* >(mEditor->get_nth_page(page))
        ->isDrawingArea();

    if (page != -1) {
        if (!checkVpz()) {
            return;
        }
        if (isVPZ) {
            title = _("VPZ file");
        } else {
            title = _("Text file");
        }

        Gtk::FileChooserDialog file(title,
                                    Gtk::FILE_CHOOSER_ACTION_SAVE);

        file.set_transient_for(*this);
        file.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        file.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

        if (isVPZ) {
            Gtk::FileFilter filter;
            filter.set_name(_("Vle Project gZipped"));
            filter.add_pattern("*.vpz");
            file.add_filter(filter);
        }

        // to provide a default filename
        // but also a default location
        file.set_filename(dynamic_cast < Document* >(mEditor->
                                                     get_nth_page(page))
                          ->filepath());

        if (file.run() == Gtk::RESPONSE_OK) {
            std::string filename(file.get_filename());

            if (isVPZ) {
                vpz::Vpz::fixExtension(filename);

                Editor::Documents::const_iterator it =
                    mEditor->getDocuments().find(mModeling->getFileName());

                mModeling->saveXML(filename);
                setTitle(mModeling->getFileName());

                if (it != mEditor->getDocuments().end()) {
                    it->second->setTitle(filename,
                                         mModeling->getTopModel(), false);
                }
            } else {
                DocumentText* doc = dynamic_cast < DocumentText* >(
                    mEditor->get_nth_page(page));

                doc->saveAs(filename);
            }
            refreshPackageHierarchy();
        }
    }
}

void GVLE::fixSave()
{
    if (not mEditor->getDocuments().empty()) {
        int page = mEditor->get_current_page();
        Gtk::Widget* tab = mEditor->get_nth_page(page);
        bool found = false;
        Editor::Documents::const_iterator it =
            mEditor->getDocuments().begin();

        while (not found and it != mEditor->getDocuments().end()) {
            if (it->second == tab) {
                found = true;
            } else {
                ++it;
            }
        }
        if (it->second->isModified()) {
            mMenuAndToolbar->showSave();
        } else {
            mMenuAndToolbar->hideSave();
        }
    }
}

bool GVLE::closeTab(const std::string& filepath)
{
    bool vpz = false;
    bool close = false;
    Editor::Documents::const_iterator it =
        mEditor->getDocuments().find(filepath);

    if (it != mEditor->getDocuments().end()) {
        if (not it->second->isModified() or
            gvle::Question(_("The current tab is not saved\n"
                             "Do you really want to close this file ?"))) {
            if (it->second->isDrawingArea()) {
                mEditor->closeVpzTab();
                mModeling->clearModeling();
                setTitle(mModeling->getFileName());
                clearModelTreeBox();
                clearModelClassBox();
                mModelTreeBox->set_sensitive(false);
                mModelClassBox->set_sensitive(false);
                vpz = true;
                close = true;
            } else {
                mEditor->closeTab(it->first);
            }
            mMenuAndToolbar->onCloseTab(vpz, mEditor->getDocuments().empty());
            fixSave();
            updateTitle();
        }
    }
    return close;
}

void GVLE::onCloseTab()
{
    bool vpz = false;
    int page = mEditor->get_current_page();
    Gtk::Widget* tab = mEditor->get_nth_page(page);
    bool found = false;
    Editor::Documents::const_iterator it = mEditor->getDocuments().begin();

    while (not found and it != mEditor->getDocuments().end()) {
        if (it->second == tab) {
            found = true;
        } else {
            ++it;
        }
    }
    if (found) {
        if (not it->second->isModified() or
            gvle::Question(_("The current tab is not saved\n"
                             "Do you really want to close this file ?"))) {
            if (it->second->isDrawingArea()) {
                mModeling->clearModeling();
                getEditor()->closeVpzTab();
                setTitle(mModeling->getFileName());
                clearModelTreeBox();
                clearModelClassBox();
                mModelTreeBox->set_sensitive(false);
                mModelClassBox->set_sensitive(false);
                vpz = true;
            }
            mEditor->closeTab(it->first);
            mMenuAndToolbar->onCloseTab(vpz, mEditor->getDocuments().empty());
            fixSave();
            updateTitle();
        }
    }
}

void GVLE::onCloseProject()
{
    mEditor->closeAllTab();
    mModeling->clearModeling();
    setTitle(mModeling->getFileName());
    clearModelTreeBox();
    clearModelClassBox();
    mModelTreeBox->set_sensitive(false);
    mModelClassBox->set_sensitive(false);
    utils::Package::package().select("");
    mPluginFactory.update();
    buildPackageHierarchy();
    mMenuAndToolbar->showMinimalMenu();
    setTitle("");
    mFileTreeView->set_sensitive(false);

}

void GVLE::onQuit()
{
    mQuitBox->show();
}

void GVLE::onPreferences()
{
    if (mPreferencesBox->run() == Gtk::RESPONSE_OK) {
        refreshViews();
        mEditor->refreshViews();
    }
}

void GVLE::onSimulationBox()
{
    if (mModeling->isSaved()) {
        LaunchSimulationBox box(mRefXML, ((const Modeling*)mModeling)->vpz());
        box.run();
        refreshPackageHierarchy();
    } else {
        gvle::Error(_("Save or load a project before simulation"));
    }
}

void GVLE::onParameterExecutionBox()
{
    ParameterExecutionBox box(mModeling);
    box.run();
}

void GVLE::onExperimentsBox()
{
    ExperimentBox box(mRefXML, mModeling);
    box.run();
}

void GVLE::onConditionsBox()
{
    const Modeling* modeling = (const Modeling*)mModeling;

    if (runConditionsBox(modeling->conditions()) == 1) {
        applyRemoved();

        renameList tmpRename= applyConditionsBox(mModeling->conditions());
        {
            renameList::const_iterator it = tmpRename.begin();

            while (it != tmpRename.end()) {
                vpz::AtomicModelList& atomlist(
                    mModeling->vpz().project().model().atomicModels());
                atomlist.updateCondition(it->first, it->second);

                vpz::ClassList::iterator itc = mModeling->
                    vpz().project().classes().begin();

                while (itc != mModeling->vpz().project().classes().end()) {
                    vpz::AtomicModelList& atomlist(
                        itc->second.atomicModels());
                    atomlist.updateCondition(it->first, it->second);
                    itc++;
                }
                ++it;
            }
        }
    }
}

void GVLE::applyRemoved()
{
    vpz::AtomicModelList& list =
        mModeling->vpz().project().model().atomicModels();
    vpz::AtomicModelList::iterator it = list.begin();

    while (it != list.end()) {
        std::vector < std::string > mdlConditions =
            it->second.conditions();
        std::vector < std::string >::const_iterator its =
            mdlConditions.begin();

        while (its != mdlConditions.end()) {
            if (not mModeling->conditions().exist(*its)) {
                it->second.delCondition(*its);
            }
            ++its;
        }
        ++it;
    }

    vpz::ClassList::iterator itc = mModeling->vpz().project().classes().begin();

    while (itc != mModeling->vpz().project().classes().end()) {
        vpz::AtomicModelList& atomlist( itc->second.atomicModels() );
        vpz::AtomicModelList::iterator itl = atomlist.begin();

        while (itl != atomlist.end()) {
            std::vector < std::string > mdlConditions =
                itl->second.conditions();
            std::vector < std::string >::const_iterator its =
                mdlConditions.begin();
            while (its != mdlConditions.end()) {
                if (not mModeling->conditions().exist(*its)) {
                    itl->second.delCondition(*its);
                }
                ++its;
            }
            ++itl;
        }
        ++itc;
    }
}

int GVLE::runConditionsBox(const vpz::Conditions& conditions)
{
    return mConditionsBox->run(conditions);
}

renameList GVLE::applyConditionsBox(vpz::Conditions& conditions)
{
    return mConditionsBox->apply(conditions);
}

void GVLE::onHostsBox()
{
    HostsBox box(mRefXML);
    box.run();
}

void GVLE::onHelpBox()
{
}

void GVLE::onViewOutputBox()
{
    const Modeling* modeling((const Modeling*)mModeling);
    vpz::Views views(modeling->views());
    ViewOutputBox box(*mModeling, this, mRefXML, views);
    box.run();
}

void GVLE::onDynamicsBox()
{
    const Modeling* modeling((const Modeling*)mModeling);
    vpz::Dynamics dynamics(modeling->dynamics());
    DynamicsBox box(*mModeling, mRefXML, dynamics);
    box.run();
}

void GVLE::onShowAbout()
{
    About box(mRefXML);
    box.run();
}

void GVLE::saveVpz()
{
    if (!checkVpz()){
        return;
    }
    if (mModeling->isSaved()) {
        Editor::Documents::const_iterator it =
            mEditor->getDocuments().find(mModeling->getFileName());

        mModeling->saveXML(mModeling->getFileName());
        mModeling->setModified(false);
        setTitle(mModeling->getFileName());
        if (it != mEditor->getDocuments().end()) {
            it->second->setTitle(mModeling->getFileName(),
                                 mModeling->getTopModel(), false);
        }
    } else {
        saveFirstVpz();
        refreshPackageHierarchy();
    }
}

void GVLE::saveFirstVpz()
{
    if (not utils::Package::package().name().empty()) {
        mSaveVpzBox->show();
    } else {
        Gtk::FileChooserDialog file(_("VPZ file"),
                                    Gtk::FILE_CHOOSER_ACTION_SAVE);
        file.set_transient_for(*this);
        file.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        file.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
        Gtk::FileFilter filter;
        filter.set_name(_("Vle Project gZipped"));
        filter.add_pattern("*.vpz");
        file.add_filter(filter);

        if (file.run() == Gtk::RESPONSE_OK) {
            std::string filename(file.get_filename());
            vpz::Vpz::fixExtension(filename);
            Editor::Documents::const_iterator it =
                mEditor->getDocuments().find(mModeling->getFileName());
            mModeling->saveXML(filename);
            setTitle(mModeling->getFileName());
            if (it != mEditor->getDocuments().end()) {
                it->second->setTitle(filename,
                                     mModeling->getTopModel(), false);
            }
        }
    }
}

void GVLE::setTitle(const Glib::ustring& name)
{
    Glib::ustring title(windowTitle());

    if (utils::Package::package().selected()) {
        title += " - " + utils::Package::package().name();
    }

    if (not name.empty()) {
        title += " - " + Glib::path_get_basename(name);
    }
    set_title(title);
}

void GVLE::updateTitle()
{
    if (not mEditor->getDocuments().empty()) {
        int page = mEditor->get_current_page();
        Gtk::Widget* tab = mEditor->get_nth_page(page);
        bool found = false;
        Editor::Documents::const_iterator it =
            mEditor->getDocuments().begin();

        while (not found and it != mEditor->getDocuments().end()) {
            if (it->second == tab) { found = true; } else { ++it; }
        }
        if (found) {
            setTitle(utils::Path::basename(it->second->filename()) +
                     utils::Path::extension(it->second->filename()));
        }
    }
}

std::string valuetype_to_string(value::Value::type type)
{
    switch (type) {
    case(value::Value::BOOLEAN):
        return "boolean";
        break;
    case(value::Value::INTEGER):
        return "integer";
        break;
    case(value::Value::DOUBLE):
        return "double";
        break;
    case(value::Value::STRING):
        return "string";
        break;
    case(value::Value::SET):
        return "set";
        break;
    case(value::Value::MAP):
        return "map";
        break;
    case(value::Value::TUPLE):
        return "tuple";
        break;
    case(value::Value::TABLE):
        return "table";
        break;
    case(value::Value::XMLTYPE):
        return "xml";
        break;
    case(value::Value::NIL):
        return "null";
        break;
    case(value::Value::MATRIX):
        return "matrix";
        break;
    default:
        return "(no value)";
        break;
    }
}

bool GVLE::packageAllTimer()
{
    std::string o, e;
    utils::Package::package().getOutputAndClear(o);
    utils::Package::package().getErrorAndClear(e);
    Glib::RefPtr < Gtk::TextBuffer > ref = mLog->get_buffer();

    insertLog(o);
    insertLog(e);
    scrollLogToLastLine();

    if (utils::Package::package().isFinish()) {
        ++itDependencies;
        if (itDependencies != mDependencies.end()) {
            utils::Package::package().select(*itDependencies);
            buildAllProject();
        } else {
            utils::Package::package().select(utils::Path::filename(mPackage));
            insertLog("package " +
                      utils::Package::package().name() +
                      " & first level dependencies built\n");
            getMenu()->showProjectMenu();
        }
        scrollLogToLastLine();
        return false;
    } else {
        return true;
    }
}

bool GVLE::packageTimer()
{
    std::string o, e;
    utils::Package::package().getOutputAndClear(o);
    utils::Package::package().getErrorAndClear(e);
    Glib::RefPtr < Gtk::TextBuffer > ref = mLog->get_buffer();

    insertLog(o);
    insertLog(e);
    scrollLogToLastLine();

    if (utils::Package::package().isFinish()) {
        getMenu()->showProjectMenu();
        return false;
    } else {
        return true;
    }
}

bool GVLE::packageConfigureAllTimer()
{
    std::string o, e;
    utils::Package::package().getOutputAndClear(o);
    utils::Package::package().getErrorAndClear(e);

    insertLog(o);
    insertLog(e);
    scrollLogToLastLine();

    if (utils::Package::package().isFinish()) {
        if (utils::Package::package().isSuccess()) {
            buildAllProject();
        } else {
            getMenu()->showProjectMenu();
        }
        scrollLogToLastLine();
        return false;
    } else {
        return true;
    }
}

bool GVLE::packageBuildAllTimer()
{
    std::string o, e;
    utils::Package::package().getOutputAndClear(o);
    utils::Package::package().getErrorAndClear(e);

    insertLog(o);
    insertLog(e);
    scrollLogToLastLine();

    if (utils::Package::package().isFinish()) {
        if (utils::Package::package().isSuccess()) {
            installAllProject();
        } else {
            getMenu()->showProjectMenu();
        }
        scrollLogToLastLine();
        return false;
    } else {
        return true;
    }
}

bool GVLE::packageBuildTimer()
{
    std::string o, e;
    utils::Package::package().getOutputAndClear(o);
    utils::Package::package().getErrorAndClear(e);

    insertLog(o);
    insertLog(e);
    scrollLogToLastLine();

    if (utils::Package::package().isFinish()) {
        if (utils::Package::package().isSuccess()) {
            installProject();
        } else {
            getMenu()->showProjectMenu();
        }
        scrollLogToLastLine();
        return false;
    } else {
        return true;
    }
}

void GVLE::configureAllProject()
{
    insertLog("configure package " + utils::Package::package().name() + "\n");
    getMenu()->hideProjectMenu();
    try {
        utils::Package::package().configure();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageConfigureAllTimer), 250);
}

void GVLE::configureProject()
{
    insertLog("configure package\n");
    getMenu()->hideProjectMenu();
    try {
        utils::Package::package().configure();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageTimer), 250);
}

void GVLE::buildAllProject()
{
    insertLog("build package " + utils::Package::package().name() + "\n");
    try {
        utils::Package::package().build();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageBuildAllTimer), 250);
}

void GVLE::buildProject()
{
    insertLog("build package\n");
    getMenu()->hideProjectMenu();
    try {
        utils::Package::package().build();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageBuildTimer), 250);
}

std::map < std::string, Depends > GVLE::depends()
{
    std::map < std::string, Depends > result;

    utils::PathList vpz(utils::Path::path().getInstalledExperiments());
    std::sort(vpz.begin(), vpz.end());

    for (utils::PathList::iterator it = vpz.begin(); it != vpz.end(); ++it) {
        std::set < std::string > depends;
        try {
            vpz::Vpz vpz(*it);
            depends = vpz.depends();
        } catch (const std::exception& /*e*/) {
        }
        result[*it] = depends;
    }

    return result;
}

void GVLE::makeAllProject()
{
    AllDepends deps = depends();

    insertLog("\nbuild package "  +
              utils::Package::package().name() +
              " & first level of dependencies\n");

    getMenu()->hideProjectMenu();

    mDependencies.clear();

    for (AllDepends::const_iterator it = deps.begin(); it != deps.end(); ++it) {
        for (Depends::const_iterator jt = it->second.begin(); jt !=
             it->second.end(); ++jt) {
            mDependencies.insert(*jt);
        }
    }

    mDependencies.insert(utils::Package::package().name());

    using utils::Path;
    using utils::Package;

    itDependencies = mDependencies.begin();

    if (itDependencies != mDependencies.end()) {

        Package::package().select(*itDependencies);

        insertLog("configure package " +
                  utils::Package::package().name() + "\n");

        Package::package().configure();

        Glib::signal_timeout().connect(
            sigc::mem_fun(*this, &GVLE::packageConfigureAllTimer), 250);
    }
}

void GVLE::displayDependencies()
{
    std::string dependsbuffer;

    AllDepends deps = depends();

    for (AllDepends::const_iterator it = deps.begin(); it != deps.end(); ++it) {
        if (it->second.empty()) {
            dependsbuffer += "<b>" + utils::Path::basename(it->first) +
                "</b> : -\n";
        } else {
            dependsbuffer += "<b>" + utils::Path::basename(it->first) +
                "</b> : ";

            Depends::const_iterator jt = it->second.begin();
            while (jt != it->second.end()) {
                Depends::const_iterator kt = jt++;
                dependsbuffer += *kt;
                if (jt != it->second.end()) {
                    dependsbuffer += ", ";
                } else {
                    dependsbuffer += '\n';
                }
            }
        }
    }

    const std::string title =
        utils::Path::filename(mPackage) +
        _(" - Package Dependencies");

    Gtk::MessageDialog* box;

    box = new Gtk::MessageDialog(dependsbuffer, true, Gtk::MESSAGE_INFO,
                                 Gtk::BUTTONS_OK, true);
    box->set_title(title);

    box->run();
    delete box;
}

void GVLE::testProject()
{
    insertLog("test package\n");
    getMenu()->hideProjectMenu();
    try {
        utils::Package::package().test();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageTimer), 250);
}


void GVLE::installAllProject()
{
    insertLog("install package : " +
              utils::Package::package().name() +
              "\n");
    try {
        utils::Package::package().install();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageAllTimer), 250);
}


void GVLE::installProject()
{
    insertLog("install package\n");
    getMenu()->hideProjectMenu();
    try {
        utils::Package::package().install();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageTimer), 250);
}

void GVLE::cleanProject()
{
    insertLog("clean package\n");
    getMenu()->hideProjectMenu();
    try {
        utils::Package::package().clean();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageTimer), 250);
}

void GVLE::packageProject()
{
    insertLog("make source and binary packages\n");
    getMenu()->hideProjectMenu();
    try {
        utils::Package::package().pack();
    } catch (const std::exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    } catch (const Glib::Exception& e) {
        getMenu()->showProjectMenu();
        gvle::Error(e.what());
        return;
    }
    Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &GVLE::packageTimer), 250);
}

void parse_model(vpz::AtomicModelList& list)
{
    vpz::AtomicModelList::iterator it = list.begin();
    while (it != list.end()) {
        if (it->first)
            std::cout << "\t" << it->first << " : " <<
                it->first->getName() << "\n";
        else
            std::cout << "\tNULL\n";

        ++it;
    }
}

void GVLE::onCutModel()
{
    if (mCurrentTab >= 0) {
        if (dynamic_cast<Document*>(mEditor->get_nth_page(mCurrentTab))
            ->isDrawingArea()) {
            View* currentView = dynamic_cast<DocumentDrawingArea*>(
                mEditor->get_nth_page(mCurrentTab))->getView();

            if (currentView) {
                currentView->onCutModel();
                mMenuAndToolbar->showPaste();
            }
        } else {
            DocumentText* doc = dynamic_cast<DocumentText*>(
                mEditor->get_nth_page(mCurrentTab));

            if (doc) {
                doc->cut();
            }
            mMenuAndToolbar->showPaste();
        }
    }
}

void GVLE::onCopyModel()
{
    if (mCurrentTab >= 0) {
        if (dynamic_cast<Document*>(mEditor->get_nth_page(mCurrentTab))
            ->isDrawingArea()) {
            View* currentView = dynamic_cast<DocumentDrawingArea*>(
                mEditor->get_nth_page(mCurrentTab))->getView();

            if (currentView) {
                currentView->onCopyModel();
                mMenuAndToolbar->showPaste();
            }
        } else {
            DocumentText* doc = dynamic_cast<DocumentText*>(
                mEditor->get_nth_page(mCurrentTab));

            if (doc) {
                doc->copy();
            }
            mMenuAndToolbar->showPaste();
        }
    }
}

void GVLE::onPasteModel()
{
    if (mCurrentTab >= 0) {
        if (dynamic_cast<Document*>(mEditor->get_nth_page(mCurrentTab))
            ->isDrawingArea()) {
            View* currentView = dynamic_cast<DocumentDrawingArea*>(
                mEditor->get_nth_page(mCurrentTab))->getView();

            if (currentView) {
                currentView->onPasteModel();
            }
        } else {
            DocumentText* doc = dynamic_cast<DocumentText*>(
                mEditor->get_nth_page(mCurrentTab));

            if (doc) {
                doc->paste();
            }
        }
    }
}

void GVLE::onSelectAll()
{
    if (mCurrentTab >= 0) {
        if (dynamic_cast<Document*>(mEditor->get_nth_page(mCurrentTab))
            ->isDrawingArea()) {
            View* currentView = dynamic_cast<DocumentDrawingArea*>(
                mEditor->get_nth_page(mCurrentTab))->getView();

            if (currentView) {
                graph::CoupledModel* cModel = currentView->getGCoupledModel();
                currentView->onSelectAll(cModel);
                mMenuAndToolbar->showCopyCut();
            }
        } else {
            DocumentText* doc = dynamic_cast<DocumentText*>(
                mEditor->get_nth_page(mCurrentTab));

            if (doc) {
                doc->selectAll();
                mMenuAndToolbar->showCopyCut();
            }
        }
    }
}

void GVLE::cut(graph::ModelList& lst, graph::CoupledModel* gc,
                   std::string className)
{
    if (className.empty()) {
        mCutCopyPaste.cut(lst, gc, mModeling->vpz().
                          project().model().atomicModels());
    } else {
        mCutCopyPaste.cut(lst, gc, mModeling->vpz().project().
                          classes().get(className).atomicModels());
    }
}

void GVLE::copy(graph::ModelList& lst, graph::CoupledModel* gc,
                    std::string className)
{
    // the current view is not a class
    if (className.empty()) {
        // no model is selected in current view and a class is selected
        // -> class instantiation
        if (lst.empty() and not mModeling->getSelectedClass().empty()) {
            vpz::Class& currentClass = mModeling->vpz().project().classes()
                .get(mModeling->getSelectedClass());
            graph::Model* model = currentClass.model();
            graph::ModelList lst2;

            lst2[model->getName()] = model;
            mCutCopyPaste.copy(lst2, gc,
                               mModeling->vpz().project().classes()
                               .get(mModeling->getSelectedClass())
                               .atomicModels(), true);
        } else {
            mCutCopyPaste.copy(lst, gc,
                               mModeling->vpz().project().model().
                               atomicModels(),
                               false);
        }
    } else {
        mCutCopyPaste.copy(lst, gc,
                           mModeling->vpz().project().classes().get(className)
                           .atomicModels(), false);
    }
}

void GVLE::paste(graph::CoupledModel* gc, std::string className)
{
    if (className.empty()) {
        mCutCopyPaste.paste(gc, mModeling->vpz().project().model().
                            atomicModels());
    } else {
        mCutCopyPaste.paste(gc, mModeling->vpz().project().classes().
                            get(className).atomicModels());
    }
}

void GVLE::selectAll(graph::ModelList& lst, graph::CoupledModel* gc)
{
    lst = gc->getModelList();
}

bool GVLE::paste_is_empty() {
    return mCutCopyPaste.paste_is_empty();
}

void GVLE::setModified(bool modified)
{
    mModeling->setModified(modified);
}

void GVLE::clearCurrentModel()
{
    if (mCurrentTab >= 0) {
        View* currentView = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getView();

        if (currentView) {
            currentView->clearCurrentModel();
        }
    }
}

void GVLE::delModel(graph::Model* model, std::string className)
{
    if (model->isAtomic()) {
        vpz::AtomicModelList& list = mModeling
            ->getAtomicModelClass(className);
        list.del(model);
        setModified(true);
    } else {
        graph::ModelList& graphlist =
            graph::Model::toCoupled(model)->getModelList();
        vpz::AtomicModelList& vpzlist = mModeling
            ->getAtomicModelClass(className);
        graph::ModelList::iterator it;
        for (it = graphlist.begin(); it!= graphlist.end(); ++it) {
            if (it->second->isCoupled())
                delModel(it->second, className);
            else
                vpzlist.del(it->second);
        }
        setModified(true);
    }
}

void GVLE::importModel()
{
    Gtk::FileChooserDialog file(_("VPZ file"), Gtk::FILE_CHOOSER_ACTION_OPEN);
    file.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    file.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    Gtk::FileFilter filter;
    filter.set_name(_("Vle Project gZipped"));
    filter.add_pattern("*.vpz");
    file.add_filter(filter);

    if (file.run() == Gtk::RESPONSE_OK) {
        std::string project_file = file.get_filename();
        try {
            using namespace vpz;

            View* currentView = dynamic_cast<DocumentDrawingArea*>(
                mEditor->get_nth_page(mCurrentTab))->getView();
            vpz::Vpz* src = new vpz::Vpz(project_file);

            assert(currentView->getGCoupledModel());
            assert(src);

            if (mImportModelBox) {
                mImportModelBox->setGCoupled(currentView->getGCoupledModel());
                if (mImportModelBox->show(src)) {
                    graph::Model* import = src->project().model().model();
                    currentView->getGCoupledModel()->addModel(import);
                    mModeling->importModel(import, src);
                    redrawModelTreeBox();
                    refreshViews();
                }
            }
            delete src;
        } catch (std::exception& E) {
            Error(E.what());
        }
    }
}

void GVLE::importModelToClass(vpz::Vpz* src, std::string& className)
{
    using namespace vpz;
    assert(src);
    boost::trim(className);

    if (mImportModelBox) {
        if (mImportModelBox->show(src)) {
            mModeling->importModelToClass(src, className);
            redrawModelClassBox();
            refreshViews();
        }
    }
}

void GVLE::importClasses(vpz::Vpz* src)
{
    using namespace vpz;
    assert(src);

    if (mImportClassesBox) {
        mImportClassesBox->show(src);
    }
}

void GVLE::exportCurrentModel()
{
    View* currentView = dynamic_cast<DocumentDrawingArea*>(
        mEditor->get_nth_page(mCurrentTab))->getView();
    currentView->exportCurrentModel();
}

void GVLE::EditCoupledModel(graph::CoupledModel* model)
{
    assert(model);
    mCoupledBox->show(model);
}

void GVLE::exportGraphic()
{
    ViewDrawingArea* tab;
    if ( dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->isComplete()) {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getCompleteDrawingArea();
    } else {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getSimpleDrawingArea();
    }


    const vpz::Experiment& experiment = ((const Modeling*)mModeling)
        ->vpz().project().experiment();
    if (experiment.name().empty() || experiment.duration() == 0) {
        Error(_("Fix a Value to the name and the duration "	\
                "of the experiment before exportation."));
        return;
    }

    Gtk::FileChooserDialog file(_("Image file"), Gtk::FILE_CHOOSER_ACTION_SAVE);
    file.set_transient_for(*this);
    file.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    file.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    Gtk::FileFilter filterAuto;
    Gtk::FileFilter filterPng;
    Gtk::FileFilter filterPdf;
    Gtk::FileFilter filterSvg;
    filterAuto.set_name(_("Guess type from file name"));
    filterAuto.add_pattern("*");
    filterPng.set_name(_("Portable Newtork Graphics (.png)"));
    filterPng.add_pattern("*.png");
    filterPdf.set_name(_("Portable Format Document (.pdf)"));
    filterPdf.add_pattern("*.pdf");
    filterSvg.set_name(_("Scalable Vector Graphics (.svg)"));
    filterSvg.add_pattern("*.svg");
    file.add_filter(filterAuto);
    file.add_filter(filterPng);
    file.add_filter(filterPdf);
    file.add_filter(filterSvg);


    if (file.run() == Gtk::RESPONSE_OK) {
        std::string filename(file.get_filename());
        std::string extension(file.get_filter()->get_name());

        if (extension == _("Guess type from file name")) {
            size_t ext_pos = filename.find_last_of('.');
            if (ext_pos != std::string::npos) {
                std::string type(filename, ext_pos+1);
                filename.resize(ext_pos);
                if (type == "png")
                    tab->exportPng(filename);
                else if (type == "pdf")
                    tab->exportPdf(filename);
                else if (type == "svg")
                    tab->exportSvg(filename);
                else
                    Error(_("Unsupported file format"));
            }
        }
        else if (extension == _("Portable Newtork Graphics (.png)"))
            tab->exportPng(filename);
        else if (extension == _("Portable Format Document (.pdf)"))
            tab->exportPdf(filename);
        else if (extension == _("Scalable Vector Graphics (.svg)"))
            tab->exportSvg(filename);
    }
}

void GVLE::addCoefZoom()
{
    ViewDrawingArea* tab;
    if ( dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->isComplete()) {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getCompleteDrawingArea();
    } else {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getSimpleDrawingArea();
    }
    tab->addCoefZoom();
}

void GVLE::delCoefZoom()
{
    ViewDrawingArea* tab;
    if ( dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->isComplete()) {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getCompleteDrawingArea();
    } else {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getSimpleDrawingArea();
    }
    tab->delCoefZoom();
}

void GVLE::setCoefZoom(double coef)
{
    ViewDrawingArea* tab;
    if ( dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->isComplete()) {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getCompleteDrawingArea();
    } else {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getSimpleDrawingArea();
    }
    tab->setCoefZoom(coef);
}

void  GVLE::updateAdjustment(double h, double v)
{
    DocumentDrawingArea* tab = dynamic_cast<DocumentDrawingArea*>(
        mEditor->get_nth_page(mCurrentTab));
    tab->setHadjustment(h);
    tab->setVadjustment(v);
}

void GVLE::onOrder()
{
    ViewDrawingArea* tab;
    if ( dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->isComplete()) {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getCompleteDrawingArea();
    } else {
        tab = dynamic_cast<DocumentDrawingArea*>(
            mEditor->get_nth_page(mCurrentTab))->getSimpleDrawingArea();
    }
    tab->onOrder();
    mModeling->setModified(true);
}

void GVLE::onTrouble()
{
    if (utils::Package::package().selected()) {
        std::string package = utils::Package::package().name();
        boost::algorithm::to_lower(package);

        if (package == "canabis" and not mTroubleTimemout.connected()) {
            mTroubleTimemout = Glib::signal_timeout().connect(
                sigc::mem_fun(*this, &GVLE::signalTroubleTimer), 50);
        }
    }
}

bool GVLE::signalTroubleTimer()
{
    static int nb = 0;

    if (not utils::Package::package().selected()) {
        return false;
    }

    std::string package = utils::Package::package().name();
    if (package.empty()) {
        return false;
    }

    boost::algorithm::to_lower(package);
    if (package == "canabis") {
        int x, y;

        get_position(x, y);

        switch (nb % 4) {
        case 0:
            x = x - 5;
            break;
        case 1:
            y = y + 5;
            break;
        case 2:
            x = x + 5;
            break;
        default:
            y = y - 5;
            break;
        }

        move(x, y);
        nb++;
        return true;
    }
    return false;
}

}} // namespace vle gvle
